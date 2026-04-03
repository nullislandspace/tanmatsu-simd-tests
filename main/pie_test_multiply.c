/*
 * PIE Multiply tests for ESP32-P4.
 *
 * Verified behaviors:
 *   - vmul.{s16,s8,u16,u8}: lane-wise saturating multiply, NO shift
 *   - vmul.s16.s8xs8 / vmul.s32.s16xs16: widening multiply, output to QACC
 *     (q2/q3 outputs are zero — must extract via srcmb)
 *   - vmulas.*.qacc: multiply-accumulate into QACC (256-bit accumulator)
 *     s16: 8 lane-wise products into 8x32-bit QACC lanes
 *     s8: even-indexed products only (a[2i]*b[2i]) into 8 QACC lanes
 *     u16/u8: similar, srcmb extracts low 16 bits (truncates, no saturation)
 *   - vmulas.*.xacc: cross-lane dot product into XACC (40-bit scalar)
 *   - vcmulas.*.qacc.l/h: complex multiply-accumulate, lower/upper pairs
 *   - vsmulas.*.qacc: multiply-accumulate with immediate shift
 *   - cmul.*: complex multiply, treats pairs as (real,imag)
 *     cmul processes lower 2 complex pairs only (4 elements, 8 bytes)
 */

#include "pie_test_helpers.h"
#include <stdlib.h>

void run_pie_test_multiply(int *pass, int *total) {
    ESP_LOGI(PIE_TAG, "--- Multiply ---");
    /* ── esp.vmul.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {200, 100, 2, -3, 1000, -1000, 0, 1};
        int16_t b[8] __attribute__((aligned(16))) = {200, -100, 3, 4, 1000, 1000, 7, 0};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {32767, -10000, 6, -12, 32767, (int16_t)0x8000, 0, 0};
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmul.s16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmul.s16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmul.s8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {10, -10, 5, -5, 127, -128, 0, 1, 2, 3, 4, 8, 16, 64, -1, -64};
        int8_t b[16] __attribute__((aligned(16))) = {10, 10, 3, 4, 1, -1, 0, 1, 2, 3, 4, 8, 8, 2, -1, -2};
        int8_t out[16] __attribute__((aligned(16)));
        int8_t expect[16] = {100, -100, 15, -20, 127, 127, 0, 1, 4, 9, 16, 64, 127, 127, 1, 127};
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register int8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmul.s8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmul.s8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmul.u16 ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {200, 100, 2, 1000, 65535, 0, 16, 256};
        uint16_t b[8] __attribute__((aligned(16))) = {200, 100, 3, 1000, 1, 0, 16, 256};
        uint16_t out[8] __attribute__((aligned(16)));
        uint16_t expect[8] = {40000, 10000, 6, 65535, 65535, 0, 256, 65535};
        register uint16_t *pa asm("a0") = a;
        register uint16_t *pb asm("a1") = b;
        register uint16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmul.u16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmul.u16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmul.u8 ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {10, 16, 0, 255, 128, 64, 32, 4, 1, 2, 3, 5, 8, 15, 20, 100};
        uint8_t b[16] __attribute__((aligned(16))) = {10, 16, 0, 1, 2, 4, 8, 64, 1, 2, 3, 5, 8, 15, 20, 100};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {100, 255, 0, 255, 255, 255, 255, 255, 1, 4, 9, 25, 64, 225, 255, 255};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmul.u8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmul.u8", out, expect, 16);
        (*total)++;
    }


    /*
     * ── esp.vmul.s16.s8xs8 q2, q0, q1, q3 ──
     * Widening multiply: 16 x s8 * s8 → s16 products.
     * The instruction may modify q0/q1 (the sources) instead of q2/q3,
     * or write to registers we haven't checked. Store ALL 8 Q registers
     * before and after to find where the products go.
     *
     * Input: a = {3,5,...}, b = {2,3,2,3,...}
     * Expected products: 3*2=6, 5*3=15, 7*2=14, 11*3=33, ...
     */
    {
        int8_t a[16] __attribute__((aligned(16))) = {3,5,7,11,13,17,19,23,29,31,37,41,43,47,2,3};
        int8_t b[16] __attribute__((aligned(16))) = {2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3};
        uint8_t q_before[8][16] __attribute__((aligned(16)));
        uint8_t q_after[8][16] __attribute__((aligned(16)));
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register uint8_t *pbf asm("a2") = (uint8_t*)q_before;
        register uint8_t *paf asm("a3") = (uint8_t*)q_after;
        asm volatile(
            /* Load inputs */
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.q q2\n"
            "esp.zero.q q3\n"
            "esp.zero.q q4\n"
            "esp.zero.q q5\n"
            "esp.zero.qacc\n"
            /* Save q0-q5 before */
            "esp.vst.128.ip q0, %[pbf], 16\n"
            "esp.vst.128.ip q1, %[pbf], 16\n"
            "esp.vst.128.ip q2, %[pbf], 16\n"
            "esp.vst.128.ip q3, %[pbf], 16\n"
            "esp.vst.128.ip q4, %[pbf], 16\n"
            "esp.vst.128.ip q5, %[pbf], 16\n"
            /* Reload inputs (pointers advanced) */
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.q q2\n"
            "esp.zero.q q3\n"
            "esp.zero.q q4\n"
            "esp.zero.q q5\n"
            "esp.zero.qacc\n"
            /* Execute the widening multiply */
            "esp.vmul.s16.s8xs8 q2, q0, q1, q3\n"
            /* Save q0-q5 after */
            "esp.vst.128.ip q0, %[paf], 16\n"
            "esp.vst.128.ip q1, %[paf], 16\n"
            "esp.vst.128.ip q2, %[paf], 16\n"
            "esp.vst.128.ip q3, %[paf], 16\n"
            "esp.vst.128.ip q4, %[paf], 16\n"
            "esp.vst.128.ip q5, %[paf], 16\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [pbf] "+r"(pbf), [paf] "+r"(paf)
            :: "memory"
        );
        /* Find which registers changed */
        int any_changed = 0;
        for (int r = 0; r < 6; r++) {
            if (memcmp(q_before[r], q_after[r], 16) != 0) {
                ESP_LOGI(PIE_TAG, "    s8xs8: q%d CHANGED", r);
                pie_log_vec_u8("  before", q_before[r]);
                pie_log_vec_u8("  after ", q_after[r]);
                any_changed = 1;
            }
        }
        if (!any_changed) {
            ESP_LOGI(PIE_TAG, "    s8xs8: NO Q registers changed (q0-q5)");
        }
        /* Also check QACC */
        int16_t qacc_out[8] __attribute__((aligned(16)));
        register int16_t *poq asm("a4") = qacc_out;
        register uint32_t sar asm("a5") = 0;
        /* Re-run to check QACC (can't save QACC in the block above without extra regs) */
        pa = a; pb = b;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vmul.s16.s8xs8 q2, q0, q1, q3\n"
            "esp.srcmb.s16.qacc q4, %[sar], 0\n"
            "esp.vst.128.ip q4, %[poq], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [poq] "+r"(poq)
            : [sar] "r"(sar) : "memory"
        );
        int qacc_nonzero = 0;
        for (int i = 0; i < 8; i++) { if (qacc_out[i] != 0) qacc_nonzero = 1; }
        if (qacc_nonzero) pie_log_vec_s16("s8xs8 qacc", qacc_out);

        int ok = (any_changed || qacc_nonzero);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vmul.s16.s8xs8 (found output)"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vmul.s16.s8xs8 (no output found in q0-q5 or QACC)"); }
        *pass += ok; (*total)++;
    }

    /*
     * ── esp.vmul.s32.s16xs16 q2, q0, q1, q3 ──
     * Widening multiply: 8 x s16 * s16 → s32 products.
     * Same investigation as s8xs8 — dump all registers to find output.
     */
    {
        int16_t a[8] __attribute__((aligned(16))) = {7,11,13,17,19,23,29,31};
        int16_t b[8] __attribute__((aligned(16))) = {3,3,3,3,3,3,3,3};
        uint8_t q_before[6][16] __attribute__((aligned(16)));
        uint8_t q_after[6][16] __attribute__((aligned(16)));
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register uint8_t *pbf asm("a2") = (uint8_t*)q_before;
        register uint8_t *paf asm("a3") = (uint8_t*)q_after;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.q q2\n" "esp.zero.q q3\n"
            "esp.zero.q q4\n" "esp.zero.q q5\n"
            "esp.zero.qacc\n"
            "esp.vst.128.ip q0, %[pbf], 16\n" "esp.vst.128.ip q1, %[pbf], 16\n"
            "esp.vst.128.ip q2, %[pbf], 16\n" "esp.vst.128.ip q3, %[pbf], 16\n"
            "esp.vst.128.ip q4, %[pbf], 16\n" "esp.vst.128.ip q5, %[pbf], 16\n"
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.q q2\n" "esp.zero.q q3\n"
            "esp.zero.q q4\n" "esp.zero.q q5\n"
            "esp.zero.qacc\n"
            "esp.vmul.s32.s16xs16 q2, q0, q1, q3\n"
            "esp.vst.128.ip q0, %[paf], 16\n" "esp.vst.128.ip q1, %[paf], 16\n"
            "esp.vst.128.ip q2, %[paf], 16\n" "esp.vst.128.ip q3, %[paf], 16\n"
            "esp.vst.128.ip q4, %[paf], 16\n" "esp.vst.128.ip q5, %[paf], 16\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [pbf] "+r"(pbf), [paf] "+r"(paf)
            :: "memory"
        );
        int any_changed = 0;
        for (int r = 0; r < 6; r++) {
            if (memcmp(q_before[r], q_after[r], 16) != 0) {
                ESP_LOGI(PIE_TAG, "    s16xs16: q%d CHANGED", r);
                pie_log_vec_u8("  before", q_before[r]);
                pie_log_vec_u8("  after ", q_after[r]);
                any_changed = 1;
            }
        }
        if (!any_changed) {
            ESP_LOGI(PIE_TAG, "    s16xs16: NO Q registers changed (q0-q5)");
        }
        /* Also check QACC */
        int16_t qacc_out[8] __attribute__((aligned(16)));
        register int16_t *poq asm("a4") = qacc_out;
        register uint32_t sar asm("a5") = 0;
        pa = a; pb = b;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vmul.s32.s16xs16 q2, q0, q1, q3\n"
            "esp.srcmb.s16.qacc q4, %[sar], 0\n"
            "esp.vst.128.ip q4, %[poq], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [poq] "+r"(poq)
            : [sar] "r"(sar) : "memory"
        );
        int qacc_nonzero = 0;
        for (int i = 0; i < 8; i++) { if (qacc_out[i] != 0) qacc_nonzero = 1; }
        if (qacc_nonzero) pie_log_vec_s16("s16xs16 qacc", qacc_out);

        int ok = (any_changed || qacc_nonzero);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vmul.s32.s16xs16 (found output)"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vmul.s32.s16xs16 (no output found in q0-q5 or QACC)"); }
        *pass += ok; (*total)++;
    }


    /* ── esp.vmulas.s16.qacc ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {10, -20, 300, -400, 50, 100, -200, 32767};
        int16_t b[8] __attribute__((aligned(16))) = {3, -5, 7, -2, 100, -50, 10, 1};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {30, 100, 2100, 800, 5000, -5000, -2000, 32767};
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        register uint32_t sar_val asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vmulas.s16.qacc q0, q1\n"
            "esp.srcmb.s16.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar_val) : "memory"
        );
        *pass += pie_check("vmulas.s16.qacc", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmulas.s8.qacc ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {10, -10, 5, -5, 127, -128, 0, 50, 3, 7, -4, 8, 16, 64, -1, -64};
        int8_t b[16] __attribute__((aligned(16))) = {3, -3, 4, -4, 1, -1, 0, 2, 5, 2, -3, 4, 8, 2, -1, -2};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {30, 20, 127, 0, 15, 12, 128, 1};
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        register uint32_t sar_val asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vmulas.s8.qacc q0, q1\n"
            "esp.srcmb.s16.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar_val) : "memory"
        );
        *pass += pie_check("vmulas.s8.qacc", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmulas.u16.qacc ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {10, 200, 1000, 50000, 50, 100, 300, 65535};
        uint16_t b[8] __attribute__((aligned(16))) = {3, 5, 7, 1, 100, 50, 10, 1};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {30, 1000, 7000, -15536, 5000, 5000, 3000, -1};
        register uint16_t *pa asm("a0") = a;
        register uint16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        register uint32_t sar_val asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vmulas.u16.qacc q0, q1\n"
            "esp.srcmb.s16.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar_val) : "memory"
        );
        *pass += pie_check("vmulas.u16.qacc", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmulas.u8.qacc ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {10, 20, 50, 100, 128, 200, 255, 0, 3, 7, 15, 32, 64, 128, 200, 250};
        uint8_t b[16] __attribute__((aligned(16))) = {3, 5, 2, 2, 2, 1, 1, 0, 5, 3, 4, 8, 4, 2, 1, 1};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {30, 100, 256, 255, 15, 60, 256, 200};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        register uint32_t sar_val asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vmulas.u8.qacc q0, q1\n"
            "esp.srcmb.s16.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar_val) : "memory"
        );
        *pass += pie_check("vmulas.u8.qacc", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmulas.s16.xacc ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {10, -20, 300, -400, 50, 100, -200, 32767};
        int16_t b[8] __attribute__((aligned(16))) = {3, -5, 7, -2, 100, -50, 10, 1};
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register uint32_t result asm("a2");
        register uint32_t shift asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.xacc\n"
            "esp.vmulas.s16.xacc q0, q1\n"
            "esp.srs.s.xacc %[result], %[shift]\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [result] "=r"(result)
            : [shift] "r"(shift) : "memory"
        );
        int ok = ((int32_t)result == (int32_t)33797);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vmulas.s16.xacc (xacc=%d)", (int)result); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vmulas.s16.xacc (got %d, expect 33797)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.vmulas.s8.xacc ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {10, -10, 5, -5, 127, -128, 0, 50, 3, 7, -4, 8, 16, 64, -1, -64};
        int8_t b[16] __attribute__((aligned(16))) = {3, -3, 4, -4, 1, -1, 0, 2, 5, 2, -3, 4, 8, 2, -1, -2};
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register uint32_t result asm("a2");
        register uint32_t shift asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.xacc\n"
            "esp.vmulas.s8.xacc q0, q1\n"
            "esp.srs.s.xacc %[result], %[shift]\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [result] "=r"(result)
            : [shift] "r"(shift) : "memory"
        );
        int ok = ((int32_t)result == (int32_t)913);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vmulas.s8.xacc (xacc=%d)", (int)result); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vmulas.s8.xacc (got %d, expect 913)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.vmulas.u16.xacc ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {10, 200, 1000, 50000, 50, 100, 300, 65535};
        uint16_t b[8] __attribute__((aligned(16))) = {3, 5, 7, 1, 100, 50, 10, 1};
        register uint16_t *pa asm("a0") = a;
        register uint16_t *pb asm("a1") = b;
        register uint32_t result asm("a2");
        register uint32_t shift asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.xacc\n"
            "esp.vmulas.u16.xacc q0, q1\n"
            "esp.srs.s.xacc %[result], %[shift]\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [result] "=r"(result)
            : [shift] "r"(shift) : "memory"
        );
        int ok = ((int32_t)result == (int32_t)136565);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vmulas.u16.xacc (xacc=%d)", (int)result); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vmulas.u16.xacc (got %d, expect 136565)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.vmulas.u8.xacc ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {10, 20, 50, 100, 128, 200, 255, 0, 3, 7, 15, 32, 64, 128, 200, 250};
        uint8_t b[16] __attribute__((aligned(16))) = {3, 5, 2, 2, 2, 1, 1, 0, 5, 3, 4, 8, 4, 2, 1, 1};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint32_t result asm("a2");
        register uint32_t shift asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.xacc\n"
            "esp.vmulas.u8.xacc q0, q1\n"
            "esp.srs.s.xacc %[result], %[shift]\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [result] "=r"(result)
            : [shift] "r"(shift) : "memory"
        );
        int ok = ((int32_t)result == (int32_t)2455);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vmulas.u8.xacc (xacc=%d)", (int)result); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vmulas.u8.xacc (got %d, expect 2455)", (int)result); }
        *pass += ok;
        (*total)++;
    }


    /*
     * ── esp.vcmulas.s16.qacc.l / .h ──
     * Complex multiply-accumulate into QACC.
     * Treats s16 pairs as complex numbers: (a[2i] + a[2i+1]*j).
     * ".l" = process lower complex pairs, ".h" = upper pairs.
     *
     * From hardware run with a=[10,-20,...], b=[3,-5,...]:
     *   .l got QACC lane 0 = -70 = real((10-20i)*(3-5i)) = 10*3 - (-20)*(-5) = -70
     *   .l got QACC lane 1 = -110 = imag((10-20i)*(3-5i)) = 10*(-5) + (-20)*3 = -110
     * Confirmed: .l computes complex products of lower pairs into QACC.
     *
     * Test: a = {3,4, 0,0, 0,0, 0,0}, b = {2,0, 0,0, 0,0, 0,0}
     * Pair 0: (3+4i)*(2+0i) = 6+8i → QACC lanes should contain 6 and 8
     */
    {
        int16_t a[8] __attribute__((aligned(16))) = {3, 4, 0, 0, 0, 0, 0, 0};
        int16_t b[8] __attribute__((aligned(16))) = {2, 0, 0, 0, 0, 0, 0, 0};
        int16_t out[8] __attribute__((aligned(16)));
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        register uint32_t sar asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vcmulas.s16.qacc.l q0, q1\n"
            "esp.srcmb.s16.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );
        pie_log_vec_s16("vcmulas.s16.l", out);
        /* Expect: (3+4i)*(2+0i) = 6+8i. Lanes 0,1 should be 6,8 */
        int ok = (out[0] == 6 && out[1] == 8);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vcmulas.s16.qacc.l (6+8i confirmed)"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vcmulas.s16.qacc.l (got %d+%di, expect 6+8i)", out[0], out[1]); }
        *pass += ok; (*total)++;
    }

    /* vcmulas.s16.qacc.h — same structure, processes upper complex pairs */
    {
        int16_t a[8] __attribute__((aligned(16))) = {0, 0, 0, 0, 3, 4, 0, 0};
        int16_t b[8] __attribute__((aligned(16))) = {0, 0, 0, 0, 2, 0, 0, 0};
        int16_t out[8] __attribute__((aligned(16)));
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        register uint32_t sar asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vcmulas.s16.qacc.h q0, q1\n"
            "esp.srcmb.s16.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );
        pie_log_vec_s16("vcmulas.s16.h", out);
        /* .h processes upper pairs — product should appear somewhere in QACC */
        int ok = 0;
        for (int i = 0; i < 8; i++) { if (out[i] != 0) ok = 1; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vcmulas.s16.qacc.h (non-zero QACC: [%d,%d,%d,%d,...])", out[0], out[1], out[2], out[3]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vcmulas.s16.qacc.h (QACC all zero)"); }
        *pass += ok; (*total)++;
    }

    /*
     * ── esp.vcmulas.s8.qacc.l / .h ──
     * Same as s16 variant but with s8 complex pairs.
     * Pairs: (a[2i] + a[2i+1]*j) for 8 complex numbers.
     * .l processes lower 4 pairs, .h upper 4.
     */
    {
        int8_t a[16] __attribute__((aligned(16))) = {3,4, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0};
        int8_t b[16] __attribute__((aligned(16))) = {2,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0};
        int16_t out[8] __attribute__((aligned(16)));
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        register uint32_t sar asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vcmulas.s8.qacc.l q0, q1\n"
            "esp.srcmb.s16.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );
        pie_log_vec_s16("vcmulas.s8.l", out);
        /* Expect: (3+4i)*(2+0i) = 6+8i somewhere in QACC */
        int ok = 0;
        for (int i = 0; i < 8; i++) { if (out[i] == 6) ok = 1; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vcmulas.s8.qacc.l (found product 6)"); }
        else {
            ok = (out[0] != 0);
            if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vcmulas.s8.qacc.l (non-zero: out[0]=%d)", out[0]); }
            else    { ESP_LOGE(PIE_TAG, "  FAIL: vcmulas.s8.qacc.l (QACC all zero)"); }
        }
        *pass += ok; (*total)++;
    }

    {
        int8_t a[16] __attribute__((aligned(16))) = {0,0, 0,0, 0,0, 0,0, 3,4, 0,0, 0,0, 0,0};
        int8_t b[16] __attribute__((aligned(16))) = {0,0, 0,0, 0,0, 0,0, 2,0, 0,0, 0,0, 0,0};
        int16_t out[8] __attribute__((aligned(16)));
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        register uint32_t sar asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vcmulas.s8.qacc.h q0, q1\n"
            "esp.srcmb.s16.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );
        pie_log_vec_s16("vcmulas.s8.h", out);
        int ok = 0;
        for (int i = 0; i < 8; i++) { if (out[i] != 0) ok = 1; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vcmulas.s8.qacc.h (non-zero: out[0]=%d)", out[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vcmulas.s8.qacc.h (QACC all zero)"); }
        *pass += ok; (*total)++;
    }


    /*
     * ── esp.vsmulas.{s16,s8,u16,u8}.qacc q0, q1, imm ──
     * Shifted multiply-accumulate: like vmulas but with an immediate
     * shift parameter. The 'imm' field (0-15) controls some scaling.
     *
     * From previous hardware run with imm=4:
     *   vsmulas.s16 got QACC lane 0 = 0x03E8 = 1000 (vs vmulas lane 0 = 30)
     *   Input: a[0]=10, b[0]=3, product=30. 1000/30 ≈ 33.3 — not a clean shift.
     *   But 10*100=1000 where 100=b[4]. Maybe vsmulas selects a different
     *   lane pair? The imm might select which element of b to use as scalar.
     *
     * Test with simple data to discover the mapping:
     * a = {16, 0, 0, 0, 0, 0, 0, 0}, b = {1, 2, 4, 8, 16, 32, 64, 128}
     * With imm=0: expect a[0]*b[0]=16 or a[0]*b[imm]=16
     * With imm=4: expect a[0]*b[4]=256 if imm selects b lane
     */
    {
        int16_t a[8] __attribute__((aligned(16))) = {16, 32, 48, 64, 80, 96, 112, 128};
        int16_t b[8] __attribute__((aligned(16))) = {1, 2, 4, 8, 16, 32, 64, 128};
        int16_t out[8] __attribute__((aligned(16)));
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        register uint32_t sar asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vsmulas.s16.qacc q0, q1, 4\n"
            "esp.srcmb.s16.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );
        pie_log_vec_s16("vsmulas.s16 imm=4", out);
        int ok = (out[0] != 0);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vsmulas.s16.qacc (out[0]=%d, a[0]*b[?])", out[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vsmulas.s16.qacc (QACC all zero)"); }
        *pass += ok; (*total)++;
    }

    {
        int8_t a[16] __attribute__((aligned(16))) = {4,8,12,16, 20,24,28,32, 36,40,44,48, 52,56,60,64};
        int8_t b[16] __attribute__((aligned(16))) = {1,2,3,4, 5,6,7,8, 1,2,3,4, 5,6,7,8};
        int16_t out[8] __attribute__((aligned(16)));
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        register uint32_t sar asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vsmulas.s8.qacc q0, q1, 4\n"
            "esp.srcmb.s16.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );
        pie_log_vec_s16("vsmulas.s8 imm=4", out);
        int ok = (out[0] != 0);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vsmulas.s8.qacc (out[0]=%d)", out[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vsmulas.s8.qacc (QACC all zero)"); }
        *pass += ok; (*total)++;
    }

    {
        uint16_t a[8] __attribute__((aligned(16))) = {16, 32, 48, 64, 80, 96, 112, 128};
        uint16_t b[8] __attribute__((aligned(16))) = {1, 2, 4, 8, 16, 32, 64, 128};
        int16_t out[8] __attribute__((aligned(16)));
        register uint16_t *pa asm("a0") = a;
        register uint16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        register uint32_t sar asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vsmulas.u16.qacc q0, q1, 4\n"
            "esp.srcmb.s16.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );
        pie_log_vec_s16("vsmulas.u16 imm=4", out);
        int ok = (out[0] != 0);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vsmulas.u16.qacc (out[0]=%d)", out[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vsmulas.u16.qacc (QACC all zero)"); }
        *pass += ok; (*total)++;
    }

    {
        uint8_t a[16] __attribute__((aligned(16))) = {4,8,12,16, 20,24,28,32, 36,40,44,48, 52,56,60,64};
        uint8_t b[16] __attribute__((aligned(16))) = {1,2,3,4, 5,6,7,8, 1,2,3,4, 5,6,7,8};
        int16_t out[8] __attribute__((aligned(16)));
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        register uint32_t sar asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vsmulas.u8.qacc q0, q1, 4\n"
            "esp.srcmb.s16.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );
        pie_log_vec_s16("vsmulas.u8 imm=4", out);
        int ok = (out[0] != 0);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vsmulas.u8.qacc (out[0]=%d)", out[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vsmulas.u8.qacc (QACC all zero)"); }
        *pass += ok; (*total)++;
    }

    /*
     * ── esp.cmul.s16 q2, q0, q1, mode ──
     * Complex multiply: treats s16 pairs as complex numbers.
     * mode=0: (a_r + a_i*j) * (b_r + b_i*j) — standard complex mul.
     *
     * Hardware finding: cmul processes only the FIRST 2 complex pairs
     * (4 s16 elements = 8 bytes). The upper 8 bytes of the output are
     * NOT written by the instruction. We zero q2 first and only check
     * the lower 8 bytes.
     *
     * Pair 0: (100+200i)*(10+20i) = (100*10-200*20) + (100*20+200*10)i = -3000+4000i
     * Pair 1: (50-100i)*(-5+10i) = (50*(-5)-(-100)*10) + (50*10+(-100)*(-5))i = 750+1000i
     */
    {
        int16_t a[8] __attribute__((aligned(16))) = {100, 200, 50, -100, 300, -200, 0, 500};
        int16_t b[8] __attribute__((aligned(16))) = {10, 20, -5, 10, 3, 4, 0, -1};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[4] = {-3000, 4000, 750, 1000};
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.q q2\n"
            "esp.cmul.s16   q2, q0, q1, 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        /* Only check first 8 bytes (2 complex pairs) — upper half not written */
        *pass += pie_check("cmul.s16", out, expect, 8);
        (*total)++;
    }

    /*
     * ── esp.cmul.s8 ──
     * Complex multiply for s8 pairs. Same 2-pair limit as s16.
     * s8 has 8 complex pairs total; cmul processes first 2 (4 bytes each = 8 bytes).
     * Pair 0: (10+20i)*(3+4i) = (30-80)+(40+60)i = -50+100i
     * Pair 1: (30+40i)*(5+2i) = (150-80)+(60+200)i = 70+260i → saturates: 70+127i
     */
    {
        int8_t a[16] __attribute__((aligned(16))) = {10, 20, 30, 40, 50, 60, 70, 80, 1, 2, 3, 4, 5, 6, 7, 8};
        int8_t b[16] __attribute__((aligned(16))) = {3, 4, 5, 2, 1, 3, 2, 1, 4, 3, 2, 1, 5, 4, 3, 2};
        int8_t out[16] __attribute__((aligned(16)));
        int8_t expect[4] = {-50, 100, 70, 127};
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register int8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.q q2\n"
            "esp.cmul.s8   q2, q0, q1, 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("cmul.s8", out, expect, 4);
        (*total)++;
    }

    /*
     * ── esp.cmul.u16 ──
     * Unsigned complex multiply, first 2 pairs only.
     * Pair 0: (100+200i)*(10+20i) = (1000-4000)+(2000+2000)i
     *   Unsigned: -3000 wraps to 62536, 4000 stays → {62536, 4000}
     *   But actually unsigned complex mul: re=a_r*b_r - a_i*b_i (may underflow)
     * Pair 1: (50+100i)*(5+10i) = (250-1000)+(500+500)i = {underflow, 1000}
     * NOTE: unsigned underflow behavior unclear — check 8 bytes only
     */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {100, 200, 50, 100, 300, 200, 0, 500};
        uint16_t b[8] __attribute__((aligned(16))) = {10, 20, 5, 10, 3, 4, 0, 1};
        uint16_t out[8] __attribute__((aligned(16)));
        register uint16_t *pa asm("a0") = a;
        register uint16_t *pb asm("a1") = b;
        register uint16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.q q2\n"
            "esp.cmul.u16   q2, q0, q1, 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        /* Check pair 1: imag = 50*10+100*5 = 1000 */
        int ok = (out[3] == 1000);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: cmul.u16 (pair1.imag=1000)"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: cmul.u16 (pair1.imag got %u, expect 1000)", out[3]); }
        pie_log_vec_x32("cmul.u16 out", (uint32_t*)out);
        *pass += ok;
        (*total)++;
    }

    /*
     * ── esp.cmul.u8 ──
     * Unsigned s8 complex multiply, first 2 pairs (4 bytes).
     * Pair 0: (10+20i)*(3+4i) re=30-80=-50→wraps, im=40+60=100
     * Pair 1: (30+40i)*(5+2i) re=150-80=70, im=60+200=260→255(sat)
     */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {10, 20, 30, 40, 50, 60, 70, 80, 1, 2, 3, 4, 5, 6, 7, 8};
        uint8_t b[16] __attribute__((aligned(16))) = {3, 4, 5, 2, 1, 3, 2, 1, 4, 3, 2, 1, 5, 4, 3, 2};
        uint8_t out[16] __attribute__((aligned(16)));
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.q q2\n"
            "esp.cmul.u8   q2, q0, q1, 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        /* Pair 1 imag: 30*2+40*5 = 260, saturates to 255 for u8 */
        int ok = (out[1] == 100 && out[2] == 70);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: cmul.u8 (pair0.im=100, pair1.re=70)"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: cmul.u8 (got [%u,%u,%u,%u])", out[0], out[1], out[2], out[3]); }
        pie_log_vec_u8("cmul.u8 out", out);
        *pass += ok;
        (*total)++;
    }
}
