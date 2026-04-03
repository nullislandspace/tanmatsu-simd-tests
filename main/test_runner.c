/*
 * SIMD test runner: PIE instruction verification, memcpy and memset benchmarks.
 *
 * The H.264 function tests have been removed — the h264bsd decoder
 * is being rewritten. Only the PIE instruction tests and SIMD memcpy
 * benchmark remain. See simd_findings.md for the full optimization
 * results from the H.264 function testing.
 */

#include "test_runner.h"
#include "pie_tests.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "pax_fonts.h"
#include "pax_text.h"
#include "bsp/device.h"
#include "bsp/input.h"
#include "esp_async_memcpy.h"
#include "freertos/semphr.h"
#include <stdlib.h>
#include <string.h>

static const char TAG[] = "test";

#define LINE_HEIGHT  18
#define FONT_SIZE    16
#define SCREEN_LINES 26
#define LINE_BUF_LEN 120

/* -------------------------------------------------------------------
 * Display / logging helper
 * ------------------------------------------------------------------- */

static char screen[SCREEN_LINES][LINE_BUF_LEN];

static void screen_clear(void) {
    for (int i = 0; i < SCREEN_LINES; i++)
        screen[i][0] = '\0';
}

static void screen_redraw(pax_buf_t *fb, void (*blit)(void)) {
    pax_background(fb, 0xFFFFFFFF);
    for (int i = 0; i < SCREEN_LINES; i++) {
        if (screen[i][0])
            pax_draw_text(fb, 0xFF000000, pax_font_sky_mono, FONT_SIZE, 4, i * LINE_HEIGHT, screen[i]);
    }
    blit();
}

void report(pax_buf_t *fb, void (*blit)(void), int line, const char *msg) {
    (void)line;
    /* Scroll up: discard first line, shift the rest */
    for (int i = 0; i < SCREEN_LINES - 1; i++)
        memcpy(screen[i], screen[i + 1], LINE_BUF_LEN);
    /* Write new line into last slot */
    strncpy(screen[SCREEN_LINES - 1], msg, LINE_BUF_LEN - 1);
    screen[SCREEN_LINES - 1][LINE_BUF_LEN - 1] = '\0';

    screen_redraw(fb, blit);
    ESP_LOGI(TAG, "%s", msg);
}

/* -------------------------------------------------------------------
 * SIMD memcpy tests
 * ------------------------------------------------------------------- */

/* C reference: plain byte-by-byte copy (not libc memcpy, to avoid
 * any platform-specific optimizations that would skew the comparison) */
static void memcpy_c_ref(void *dst, const void *src, size_t n) {
    const uint8_t *s = (const uint8_t *)src;
    uint8_t *d = (uint8_t *)dst;
    for (size_t i = 0; i < n; i++)
        d[i] = s[i];
}

/*
 * SIMD memcpy: 128 bytes per loop iteration (8 x 16-byte registers).
 *
 * Based on Espressif's PIE introduction blog post. Uses all 8 vector
 * registers (q0-q7) to load 128 bytes, then stores all 8. This
 * maximizes data throughput by minimizing loop overhead relative to
 * data moved.
 *
 * When src and dst are 16-byte aligned, uses SIMD for the bulk copy
 * and byte-by-byte for the remainder. Falls back to byte-by-byte for
 * the entire copy when alignment requirements are not met.
 *
 * Reference: https://developer.espressif.com/blog/2024/12/pie-introduction/
 * The blog reports 74.3% faster than standard library, 97.2% faster
 * than ANSI C, on ESP32-P4 at 360 MHz.
 */
static void memcpy_simd(void *dst, const void *src, size_t n) {
    int aligned = (((uintptr_t)src | (uintptr_t)dst) & 15) == 0;
    size_t bulk = aligned ? (n & ~(size_t)127) : 0;
    if (bulk > 0) {
        register const uint8_t *s asm("a0") = (const uint8_t *)src;
        register uint8_t *d asm("a1") = (uint8_t *)dst;
        register size_t remaining asm("a2") = bulk;

        asm volatile(
            "1:\n"
            "esp.vld.128.ip q0, %[s], 16\n"
            "esp.vld.128.ip q1, %[s], 16\n"
            "esp.vld.128.ip q2, %[s], 16\n"
            "esp.vld.128.ip q3, %[s], 16\n"
            "esp.vld.128.ip q4, %[s], 16\n"
            "esp.vld.128.ip q5, %[s], 16\n"
            "esp.vld.128.ip q6, %[s], 16\n"
            "esp.vld.128.ip q7, %[s], 16\n"
            "esp.vst.128.ip q0, %[d], 16\n"
            "esp.vst.128.ip q1, %[d], 16\n"
            "esp.vst.128.ip q2, %[d], 16\n"
            "esp.vst.128.ip q3, %[d], 16\n"
            "esp.vst.128.ip q4, %[d], 16\n"
            "esp.vst.128.ip q5, %[d], 16\n"
            "esp.vst.128.ip q6, %[d], 16\n"
            "esp.vst.128.ip q7, %[d], 16\n"
            "addi %[rem], %[rem], -128\n"
            "bnez %[rem], 1b\n"
            : [s] "+r"(s), [d] "+r"(d), [rem] "+r"(remaining)
            :: "memory"
        );
    }
    const uint8_t *s = (const uint8_t *)src + bulk;
    uint8_t *d = (uint8_t *)dst + bulk;
    for (size_t i = 0; i < n - bulk; i++) {
        d[i] = s[i];
    }
}

/* -------------------------------------------------------------------
 * SIMD memset tests
 * ------------------------------------------------------------------- */

/* C reference: plain byte-by-byte set */
static void memset_c_ref(void *dst, int c, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    uint8_t val = (uint8_t)c;
    for (size_t i = 0; i < n; i++)
        d[i] = val;
}

/*
 * SIMD memset: 128 bytes per loop iteration (store q0 eight times).
 *
 * Fills a 16-byte aligned scratch buffer with the fill byte, loads it
 * into q0, then blasts q0 to dst eight times per iteration.  Only one
 * register is needed because every store writes the same value.
 *
 * Falls back to byte-by-byte when dst is not 16-byte aligned.
 */
static void memset_simd(void *dst, int c, size_t n) {
    int aligned = ((uintptr_t)dst & 15) == 0;
    size_t bulk = aligned ? (n & ~(size_t)127) : 0;
    if (bulk > 0) {
        /* Build a 16-byte pattern buffer and load it into q0 */
        uint8_t pattern[16] __attribute__((aligned(16)));
        memset(pattern, c, 16);
        asm volatile("esp.vld.128.ip q0, %0, 0" :: "r"(pattern) : "memory");

        register uint8_t *d asm("a0") = (uint8_t *)dst;
        register size_t remaining asm("a1") = bulk;

        asm volatile(
            "1:\n"
            "esp.vst.128.ip q0, %[d], 16\n"
            "esp.vst.128.ip q0, %[d], 16\n"
            "esp.vst.128.ip q0, %[d], 16\n"
            "esp.vst.128.ip q0, %[d], 16\n"
            "esp.vst.128.ip q0, %[d], 16\n"
            "esp.vst.128.ip q0, %[d], 16\n"
            "esp.vst.128.ip q0, %[d], 16\n"
            "esp.vst.128.ip q0, %[d], 16\n"
            "addi %[rem], %[rem], -128\n"
            "bnez %[rem], 1b\n"
            : [d] "+r"(d), [rem] "+r"(remaining)
            :: "memory"
        );
    }
    uint8_t *d = (uint8_t *)dst + bulk;
    uint8_t val = (uint8_t)c;
    for (size_t i = 0; i < n - bulk; i++)
        d[i] = val;
}

#define BENCH_LOOPS(sz) ((sz) >= 1048576 ? 5 : \
                         (sz) >= 32768  ? 1000 : 100000)

static void test_and_bench_memset(pax_buf_t *fb, void (*blit)(void), int *line, size_t size) {
    char buf[120];
    int loops = BENCH_LOOPS(size);

    uint8_t *dst_c = aligned_alloc(16, size);
    uint8_t *dst_m = aligned_alloc(16, size);
    uint8_t *dst_s = aligned_alloc(16, size);

    uint8_t fill = 0xA5;  /* Arbitrary non-zero fill pattern */

    /* Correctness test */
    memset(dst_c, 0, size);
    memset(dst_m, 0, size);
    memset(dst_s, 0, size);
    memset_c_ref(dst_c, fill, size);
    memset(dst_m, fill, size);
    memset_simd(dst_s, fill, size);

    /* Verify every byte */
    int ok_c = 1, ok_m = 1, ok_s = 1;
    for (size_t i = 0; i < size; i++) {
        if (dst_c[i] != fill) { ok_c = 0; break; }
    }
    for (size_t i = 0; i < size; i++) {
        if (dst_m[i] != fill) { ok_m = 0; break; }
    }
    for (size_t i = 0; i < size; i++) {
        if (dst_s[i] != fill) { ok_s = 0; break; }
    }
    int correct = ok_c && ok_m && ok_s;

    /* Use function pointer indirection for libc memset to prevent
     * GCC from recognizing it as a builtin and optimizing it away. */
    void *(*memset_libc)(void *, int, size_t) = memset;
    asm volatile("" : "+r"(memset_libc));

    /* Benchmark C byte-loop */
    int64_t start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        memset_c_ref(dst_c, fill, size);
    int64_t c_ns = (esp_timer_get_time() - start) * 1000 / loops;

    /* Benchmark libc memset (via function pointer to prevent elimination) */
    start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        memset_libc(dst_m, fill, size);
    int64_t m_ns = (esp_timer_get_time() - start) * 1000 / loops;

    /* Benchmark SIMD */
    start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        memset_simd(dst_s, fill, size);
    int64_t s_ns = (esp_timer_get_time() - start) * 1000 / loops;

    snprintf(buf, sizeof(buf), " %7dB: %s  byte:%lld libc:%lld simd:%lldns/call",
             (int)size, correct ? "OK" : "FAIL",
             (long long)c_ns, (long long)m_ns, (long long)s_ns);
    report(fb, blit, (*line)++, buf);
    snprintf(buf, sizeof(buf), "           simd vs byte:%.2fx  simd vs libc:%.2fx",
             s_ns > 0 ? (double)c_ns / (double)s_ns : 0.0,
             s_ns > 0 ? (double)m_ns / (double)s_ns : 0.0);
    report(fb, blit, (*line)++, buf);

    free(dst_c);
    free(dst_m);
    free(dst_s);
}

static void test_and_bench_memcpy(pax_buf_t *fb, void (*blit)(void), int *line, size_t size) {
    char buf[120];
    int loops = BENCH_LOOPS(size);

    uint8_t *src   = aligned_alloc(16, size);
    uint8_t *dst_c = aligned_alloc(16, size);
    uint8_t *dst_m = aligned_alloc(16, size);
    uint8_t *dst_s = aligned_alloc(16, size);

    /* Fill source with pattern */
    for (size_t i = 0; i < size; i++)
        src[i] = (uint8_t)(i & 0xFF);

    /* Correctness test */
    memset(dst_c, 0, size);
    memset(dst_m, 0, size);
    memset(dst_s, 0, size);
    memcpy_c_ref(dst_c, src, size);
    memcpy(dst_m, src, size);
    memcpy_simd(dst_s, src, size);
    int ok_c = (memcmp(dst_c, src, size) == 0);
    int ok_m = (memcmp(dst_m, src, size) == 0);
    int ok_s = (memcmp(dst_s, src, size) == 0);
    int correct = ok_c && ok_m && ok_s;

    /* Use a function pointer indirection for libc memcpy to prevent
     * GCC from recognizing it as a builtin and optimizing it away.
     * The asm("") barrier prevents the compiler from resolving the
     * pointer back to memcpy at compile time. */
    void *(*memcpy_libc)(void *, const void *, size_t) = memcpy;
    asm volatile("" : "+r"(memcpy_libc));

    /* Benchmark C byte-loop */
    int64_t start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        memcpy_c_ref(dst_c, src, size);
    int64_t c_ns = (esp_timer_get_time() - start) * 1000 / loops;

    /* Benchmark libc memcpy (via function pointer to prevent elimination) */
    start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        memcpy_libc(dst_m, src, size);
    int64_t m_ns = (esp_timer_get_time() - start) * 1000 / loops;

    /* Benchmark SIMD */
    start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        memcpy_simd(dst_s, src, size);
    int64_t s_ns = (esp_timer_get_time() - start) * 1000 / loops;

    snprintf(buf, sizeof(buf), " %7dB: %s  byte:%lld libc:%lld simd:%lldns/call",
             (int)size, correct ? "OK" : "FAIL",
             (long long)c_ns, (long long)m_ns, (long long)s_ns);
    report(fb, blit, (*line)++, buf);
    snprintf(buf, sizeof(buf), "           simd vs byte:%.2fx  simd vs libc:%.2fx",
             s_ns > 0 ? (double)c_ns / (double)s_ns : 0.0,
             s_ns > 0 ? (double)m_ns / (double)s_ns : 0.0);
    report(fb, blit, (*line)++, buf);

    free(src);
    free(dst_c);
    free(dst_m);
    free(dst_s);
}

/* -------------------------------------------------------------------
 * DMA vs SIMD memcpy comparison
 * ------------------------------------------------------------------- */

static SemaphoreHandle_t dma_done_sem;

static IRAM_ATTR bool dma_cb(async_memcpy_handle_t hdl, async_memcpy_event_t *event, void *args) {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(dma_done_sem, &woken);
    return woken == pdTRUE;
}

static void test_dma_vs_simd(pax_buf_t *fb, void (*blit)(void), int *line,
                             async_memcpy_handle_t mcp, size_t size) {
    char buf[120];
    int loops = BENCH_LOOPS(size);

    uint8_t *src   = aligned_alloc(16, size);
    uint8_t *dst_s = aligned_alloc(16, size);
    uint8_t *dst_d = aligned_alloc(16, size);

    /* Fill source with pattern */
    for (size_t i = 0; i < size; i++)
        src[i] = (uint8_t)(i & 0xFF);

    /* Correctness test */
    memset(dst_s, 0, size);
    memset(dst_d, 0, size);
    memcpy_simd(dst_s, src, size);
    esp_async_memcpy(mcp, dst_d, src, size, dma_cb, NULL);
    xSemaphoreTake(dma_done_sem, portMAX_DELAY);
    int ok_s = (memcmp(dst_s, src, size) == 0);
    int ok_d = (memcmp(dst_d, src, size) == 0);
    int correct = ok_s && ok_d;

    /* Benchmark SIMD */
    int64_t start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        memcpy_simd(dst_s, src, size);
    int64_t s_ns = (esp_timer_get_time() - start) * 1000 / loops;

    /* Benchmark DMA (wall-clock: submit + wait) */
    start = esp_timer_get_time();
    for (int i = 0; i < loops; i++) {
        esp_async_memcpy(mcp, dst_d, src, size, dma_cb, NULL);
        xSemaphoreTake(dma_done_sem, portMAX_DELAY);
    }
    int64_t d_ns = (esp_timer_get_time() - start) * 1000 / loops;

    snprintf(buf, sizeof(buf), " %7dB: %s  simd:%lld dma:%lldns/call",
             (int)size, correct ? "OK" : "FAIL",
             (long long)s_ns, (long long)d_ns);
    report(fb, blit, (*line)++, buf);

    const char *winner = s_ns <= d_ns ? "SIMD" : "DMA";
    double ratio = s_ns <= d_ns ? (double)d_ns / (double)s_ns
                                : (double)s_ns / (double)d_ns;
    snprintf(buf, sizeof(buf), "           %s wins by %.2fx", winner, ratio);
    report(fb, blit, (*line)++, buf);

    free(src);
    free(dst_s);
    free(dst_d);
}

/* -------------------------------------------------------------------
 * Main test runner
 * ------------------------------------------------------------------- */

void run_all_tests(pax_buf_t *fb, void (*blit)(void)) {
    int line = 0;
    char buf[120];
    (void)buf;

    screen_clear();
    report(fb, blit, line++, "=== SIMD Test Suite ===");
    report(fb, blit, line++, "Waiting for debug monitor...");

    /* Wait for debug monitor to connect before producing test output */
    vTaskDelay(pdMS_TO_TICKS(7000));

    screen_clear();
    screen_redraw(fb, blit);
    report(fb, blit, line++, "=== SIMD Test Suite ===");
    report(fb, blit, line++, "");

    srand(12345);  /* Fixed seed for reproducibility */

    /* --- PIE instruction tests --- */
    run_pie_instruction_tests(fb, blit, &line);
    line++;

    /* --- SIMD memcpy benchmark ---
     * Tests increasing sizes to show how throughput scales. */
    report(fb, blit, line++, "1. memcpy (byte-loop vs libc vs SIMD)");
    test_and_bench_memcpy(fb, blit, &line, 128);
    test_and_bench_memcpy(fb, blit, &line, 256);
    test_and_bench_memcpy(fb, blit, &line, 1024);
    test_and_bench_memcpy(fb, blit, &line, 4096);
    test_and_bench_memcpy(fb, blit, &line, 32768);
    test_and_bench_memcpy(fb, blit, &line, 65536);
    test_and_bench_memcpy(fb, blit, &line, 131072);
    test_and_bench_memcpy(fb, blit, &line, 262144);
    test_and_bench_memcpy(fb, blit, &line, 1048576);
    line++;

    /* --- SIMD memset benchmark ---
     * Same sizes as memcpy to allow direct comparison. */
    report(fb, blit, line++, "2. memset (byte-loop vs libc vs SIMD)");
    test_and_bench_memset(fb, blit, &line, 128);
    test_and_bench_memset(fb, blit, &line, 256);
    test_and_bench_memset(fb, blit, &line, 1024);
    test_and_bench_memset(fb, blit, &line, 4096);
    test_and_bench_memset(fb, blit, &line, 32768);
    test_and_bench_memset(fb, blit, &line, 65536);
    test_and_bench_memset(fb, blit, &line, 131072);
    test_and_bench_memset(fb, blit, &line, 262144);
    test_and_bench_memset(fb, blit, &line, 1048576);
    line++;

    /* --- DMA vs SIMD memcpy benchmark --- */
    dma_done_sem = xSemaphoreCreateBinary();
    async_memcpy_config_t dma_cfg = ASYNC_MEMCPY_DEFAULT_CONFIG();
    async_memcpy_handle_t mcp = NULL;
    esp_err_t dma_err = esp_async_memcpy_install_gdma_axi(&dma_cfg, &mcp);
    if (dma_err == ESP_OK) {
        report(fb, blit, line++, "3. DMA vs SIMD memcpy");
        test_dma_vs_simd(fb, blit, &line, mcp, 128);
        test_dma_vs_simd(fb, blit, &line, mcp, 256);
        test_dma_vs_simd(fb, blit, &line, mcp, 1024);
        test_dma_vs_simd(fb, blit, &line, mcp, 4096);
        test_dma_vs_simd(fb, blit, &line, mcp, 32768);
        test_dma_vs_simd(fb, blit, &line, mcp, 65536);
        test_dma_vs_simd(fb, blit, &line, mcp, 131072);
        test_dma_vs_simd(fb, blit, &line, mcp, 262144);
        test_dma_vs_simd(fb, blit, &line, mcp, 1048576);
        esp_async_memcpy_uninstall(mcp);
    } else {
        snprintf(buf, sizeof(buf), "3. DMA init failed: %s", esp_err_to_name(dma_err));
        report(fb, blit, line++, buf);
    }
    vSemaphoreDelete(dma_done_sem);
    line++;

    /* --- Done --- */
    report(fb, blit, line++, "All tests complete. F1 = exit.");

    /* Idle loop: wait for F1 to return to launcher */
    QueueHandle_t q = NULL;
    bsp_input_get_queue(&q);
    while (1) {
        bsp_input_event_t event;
        if (xQueueReceive(q, &event, portMAX_DELAY) == pdTRUE) {
            if (event.type == INPUT_EVENT_TYPE_NAVIGATION &&
                event.args_navigation.key == BSP_INPUT_NAVIGATION_KEY_F1) {
                bsp_device_restart_to_launcher();
            }
        }
    }
}
