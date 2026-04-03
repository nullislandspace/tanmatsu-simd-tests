/*
 * Shared helpers for PIE SIMD instruction tests.
 *
 * Provides check/logging functions used by all pie_test_*.c files.
 * Each sub-file implements a run_pie_test_XXX() function that takes
 * pass/total counters and updates them.
 */

#pragma once

#include "pax_gfx.h"
#include "esp_log.h"
#include "pax_fonts.h"
#include "pax_text.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define PIE_LINE_HEIGHT 18
#define PIE_FONT_SIZE   16

static const char PIE_TAG[] = "pie_test";

/* ── Display + serial logging ─────────────────────────────────── */

static inline void pie_report_line(pax_buf_t *fb, void (*blit)(void), int line, const char *msg) {
    pax_draw_text(fb, 0xFF000000, pax_font_sky_mono, PIE_FONT_SIZE, 4, line * PIE_LINE_HEIGHT, msg);
    ESP_LOGI(PIE_TAG, "%s", msg);
    blit();
}

/* ── Result checking ──────────────────────────────────────────── */

/* Compare got vs expect, log PASS/FAIL, return 1 on match */
static inline int pie_check(const char *name, const void *got, const void *expect, size_t len) {
    if (memcmp(got, expect, len) == 0) {
        ESP_LOGI(PIE_TAG, "  PASS: %s", name);
        return 1;
    }
    ESP_LOGE(PIE_TAG, "  FAIL: %s", name);
    const uint8_t *g = (const uint8_t *)got;
    const uint8_t *e = (const uint8_t *)expect;
    for (size_t i = 0; i < len; i++) {
        if (g[i] != e[i]) {
            ESP_LOGE(PIE_TAG, "    byte %d: got 0x%02x, expect 0x%02x", (int)i, g[i], e[i]);
            if (i >= 3) break;
        }
    }
    return 0;
}

/* Log a full 16-byte vector as u8 values */
static inline void pie_log_vec_u8(const char *label, const uint8_t *v) {
    ESP_LOGI(PIE_TAG, "    %s: [%3d %3d %3d %3d %3d %3d %3d %3d | %3d %3d %3d %3d %3d %3d %3d %3d]",
        label,
        v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7],
        v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15]);
}

/* Log a 16-byte vector as s16 values */
static inline void pie_log_vec_s16(const char *label, const int16_t *v) {
    ESP_LOGI(PIE_TAG, "    %s: [%6d %6d %6d %6d %6d %6d %6d %6d]",
        label, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
}

/* Log a 16-byte vector as s32 values */
static inline void pie_log_vec_s32(const char *label, const int32_t *v) {
    ESP_LOGI(PIE_TAG, "    %s: [%d %d %d %d]",
        label, (int)v[0], (int)v[1], (int)v[2], (int)v[3]);
}

/* Log a 16-byte vector as u32 hex values */
static inline void pie_log_vec_x32(const char *label, const uint32_t *v) {
    ESP_LOGI(PIE_TAG, "    %s: [0x%08lx 0x%08lx 0x%08lx 0x%08lx]",
        label,
        (unsigned long)v[0], (unsigned long)v[1],
        (unsigned long)v[2], (unsigned long)v[3]);
}

/* Check with full diagnostic dump of input and output vectors (u8) */
static inline int pie_check_verbose(const char *name,
    const uint8_t *in_a, const uint8_t *in_b,
    const uint8_t *out_a, const uint8_t *out_b,
    const uint8_t *exp_a, const uint8_t *exp_b)
{
    int ok_a = (memcmp(out_a, exp_a, 16) == 0);
    int ok_b = (memcmp(out_b, exp_b, 16) == 0);

    ESP_LOGI(PIE_TAG, "  --- %s ---", name);
    pie_log_vec_u8("in  qd", in_a);
    pie_log_vec_u8("in  qs", in_b);
    pie_log_vec_u8("out qd", out_a);
    pie_log_vec_u8("out qs", out_b);
    pie_log_vec_u8("exp qd", exp_a);
    pie_log_vec_u8("exp qs", exp_b);

    if (ok_a && ok_b) {
        ESP_LOGI(PIE_TAG, "  PASS: %s", name);
        return 1;
    }
    ESP_LOGE(PIE_TAG, "  FAIL: %s (qd %s, qs %s)", name,
        ok_a ? "ok" : "MISMATCH", ok_b ? "ok" : "MISMATCH");
    return 0;
}

/*
 * Check for saturating vs wrapping behavior.
 * Returns 1 if either matches (both are valid behaviors to discover).
 * Logs which mode was detected.
 */
static inline int pie_check_sat_or_wrap(const char *name,
    const void *got, const void *expect_sat, const void *expect_wrap, size_t len)
{
    int is_sat  = (memcmp(got, expect_sat, len) == 0);
    int is_wrap = (memcmp(got, expect_wrap, len) == 0);
    if (is_sat) {
        ESP_LOGI(PIE_TAG, "  PASS: %s (saturating)", name);
        return 1;
    } else if (is_wrap) {
        ESP_LOGI(PIE_TAG, "  PASS: %s (wrapping)", name);
        return 1;
    }
    ESP_LOGE(PIE_TAG, "  FAIL: %s (neither saturating nor wrapping)", name);
    return 0;
}

/* ── Sub-test function prototypes ─────────────────────────────── */

void run_pie_test_load_store(int *pass, int *total);
void run_pie_test_arithmetic(int *pass, int *total);
void run_pie_test_multiply(int *pass, int *total);
void run_pie_test_compare(int *pass, int *total);
void run_pie_test_shift(int *pass, int *total);
void run_pie_test_logical(int *pass, int *total);
void run_pie_test_convert(int *pass, int *total);
void run_pie_test_move(int *pass, int *total);
void run_pie_test_fft(int *pass, int *total);
void run_pie_test_loop(int *pass, int *total);
