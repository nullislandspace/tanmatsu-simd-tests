/*
 * SIMD test runner: PIE instruction verification and memcpy benchmark.
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
#include <stdlib.h>
#include <string.h>

static const char TAG[] = "test";

#define LINE_HEIGHT  18
#define FONT_SIZE    16

/* -------------------------------------------------------------------
 * Display / logging helper
 * ------------------------------------------------------------------- */

static void report(pax_buf_t *fb, void (*blit)(void), int line, const char *msg) {
    pax_draw_text(fb, 0xFF000000, pax_font_sky_mono, FONT_SIZE, 4, line * LINE_HEIGHT, msg);
    ESP_LOGI(TAG, "%s", msg);
    blit();
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

#define MEMCPY_LOOPCOUNT 100000

static void test_and_bench_memcpy(pax_buf_t *fb, void (*blit)(void), int *line, size_t size) {
    char buf[120];

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
    for (int i = 0; i < MEMCPY_LOOPCOUNT; i++)
        memcpy_c_ref(dst_c, src, size);
    int64_t c_us = esp_timer_get_time() - start;

    /* Benchmark libc memcpy (via function pointer to prevent elimination) */
    start = esp_timer_get_time();
    for (int i = 0; i < MEMCPY_LOOPCOUNT; i++)
        memcpy_libc(dst_m, src, size);
    int64_t m_us = esp_timer_get_time() - start;

    /* Benchmark SIMD */
    start = esp_timer_get_time();
    for (int i = 0; i < MEMCPY_LOOPCOUNT; i++)
        memcpy_simd(dst_s, src, size);
    int64_t s_us = esp_timer_get_time() - start;

    snprintf(buf, sizeof(buf), "   %4dB: %s  byte:%lld libc:%lld simd:%lldus",
             (int)size, correct ? "OK" : "FAIL",
             (long long)c_us, (long long)m_us, (long long)s_us);
    report(fb, blit, (*line)++, buf);
    snprintf(buf, sizeof(buf), "         simd vs byte:%.2fx  simd vs libc:%.2fx",
             s_us > 0 ? (double)c_us / (double)s_us : 0.0,
             s_us > 0 ? (double)m_us / (double)s_us : 0.0);
    report(fb, blit, (*line)++, buf);

    free(src);
    free(dst_c);
    free(dst_m);
    free(dst_s);
}

/* -------------------------------------------------------------------
 * Main test runner
 * ------------------------------------------------------------------- */

void run_all_tests(pax_buf_t *fb, void (*blit)(void)) {
    int line = 0;
    char buf[120];
    (void)buf;

    pax_background(fb, 0xFFFFFFFF);
    report(fb, blit, line++, "=== SIMD Test Suite ===");
    report(fb, blit, line++, "Waiting for debug monitor...");

    /* Wait for debug monitor to connect before producing test output */
    vTaskDelay(pdMS_TO_TICKS(7000));

    pax_background(fb, 0xFFFFFFFF);
    line = 0;
    report(fb, blit, line++, "=== SIMD Test Suite ===");
    line++;

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
