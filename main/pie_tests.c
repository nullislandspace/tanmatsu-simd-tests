/*
 * ESP32-P4 PIE SIMD Instruction Reference & Verification Tests
 *
 * ORCHESTRATOR — calls all sub-test files and reports totals.
 *
 * This file serves as both a test suite and a reference manual for the
 * ESP32-P4 PIE (Processor Instruction Extension) SIMD instructions.
 * Each sub-file tests a category of instructions with detailed comments
 * explaining behavior, data format, and boundary conditions — verified
 * on real hardware.
 *
 * ═══════════════════════════════════════════════════════════════════
 * GENERAL PIE ARCHITECTURE NOTES
 * ═══════════════════════════════════════════════════════════════════
 *
 * Registers:
 *   - 8 x 128-bit vector registers: q0-q7
 *   - Can hold: 16 x u8, 8 x i16, or 4 x i32 per register
 *   - 256-bit accumulator: QACC (for multiply-accumulate)
 *   - Extended accumulator: XACC (for cross-lane reduction)
 *   - Shift Amount Register: SAR (controls shift distances)
 *
 * Memory:
 *   - All load/store addresses MUST be 16-byte aligned
 *   - Bottom 4 address bits are silently masked (misaligned loads
 *     will read from the wrong address without error)
 *   - Data is little-endian (byte 0 = least significant)
 *
 * Register constraints for inline asm:
 *   - PIE instructions only accept scalar registers x8-x15
 *     (RISC-V names: s0, s1, a0, a1, a2, a3, a4, a5)
 *   - Use "register ... asm("aX")" to force GCC to allocate
 *     variables into the correct register range
 *   - Using registers outside x8-x15 causes assembler errors
 *
 * Enabling PIE:
 *   - PIE is auto-enabled by FreeRTOS lazy coprocessor switching
 *   - No manual CSR setup needed — just use PIE instructions
 *   - First PIE instruction triggers an illegal instruction trap,
 *     FreeRTOS handler enables PIE and re-executes
 *
 * Key discovery — ALL arithmetic is SATURATING:
 *   - vadd/vsub for s32, s16, u8 all saturate (never wrap)
 *   - This differs from x86 SSE where only some variants saturate
 *
 * Reference: ESP32-S3 TRM Chapter 8 (translate ee.* → esp.* for P4)
 * Instruction list: esp-idf/.../rv_decode/xesppie.S
 * ═══════════════════════════════════════════════════════════════════
 */

#include "pie_tests.h"
#include "pie_test_helpers.h"

void run_pie_instruction_tests(pax_buf_t *fb, void (*blit)(void), int *line) {
    int pass = 0, total = 0;
    char buf[80];

    pie_report_line(fb, blit, (*line)++, "--- PIE Instruction Tests ---");

    /* Run all sub-test categories */
    run_pie_test_load_store(&pass, &total);
    run_pie_test_arithmetic(&pass, &total);
    run_pie_test_multiply(&pass, &total);
    run_pie_test_compare(&pass, &total);
    run_pie_test_shift(&pass, &total);
    run_pie_test_logical(&pass, &total);
    run_pie_test_convert(&pass, &total);
    run_pie_test_move(&pass, &total);
    run_pie_test_fft(&pass, &total);
    run_pie_test_loop(&pass, &total);

    /* Report totals */
    snprintf(buf, sizeof(buf), "PIE instructions: %d/%d PASS", pass, total);
    pie_report_line(fb, blit, (*line)++, buf);

    if (pass < total) {
        ESP_LOGE(PIE_TAG, "PIE instruction tests: %d FAILURES", total - pass);
    }
}
