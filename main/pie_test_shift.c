/*
 * PIE Shift, Slide, Source tests.
 * Hand-edited — verified on ESP32-P4 hardware.
 */

#include "pie_test_helpers.h"
#include <stdlib.h>

void run_pie_test_shift(int *pass, int *total) {
    ESP_LOGI(PIE_TAG, "--- Shift, Slide, Source ---");
    /* ── esp.vsr.s32 ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {64, -128, 1024, -1};
        int32_t out[4] __attribute__((aligned(16)));
        int32_t expect[4] = {1, -2, 16, -1};
        register int32_t *pa asm("a0") = a;
        register int32_t *po asm("a1") = out;
        register uint32_t sar_val asm("a2") = 6;
        asm volatile(
            "esp.movx.w.sar %[sar]\n"
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vsr.s32    q1, q0\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [sar] "r"(sar_val) : "memory"
        );
        *pass += pie_check("vsr.s32 (>>6)", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsr.u32 ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {64, 0x80000000, 1024, 0xFFFFFFFF};
        uint32_t out[4] __attribute__((aligned(16)));
        uint32_t expect[4] = {1, 0x02000000, 16, 0x03FFFFFF};
        register uint32_t *pa asm("a0") = a;
        register uint32_t *po asm("a1") = out;
        register uint32_t sar_val asm("a2") = 6;
        asm volatile(
            "esp.movx.w.sar %[sar]\n"
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vsr.u32    q1, q0\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [sar] "r"(sar_val) : "memory"
        );
        *pass += pie_check("vsr.u32 (>>6)", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsl.32 ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {1, 2, 3, 5};
        int32_t out[4] __attribute__((aligned(16)));
        int32_t expect[4] = {16, 32, 48, 80};
        register int32_t *pa asm("a0") = a;
        register int32_t *po asm("a1") = out;
        register uint32_t sar_val asm("a2") = 4;
        asm volatile(
            "esp.movx.w.sar %[sar]\n"
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vsl.32    q1, q0\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [sar] "r"(sar_val) : "memory"
        );
        *pass += pie_check("vsl.32 (<<4)", out, expect, 16);
        (*total)++;
    }

    /* ── esp.movx.w.sar / esp.movx.r.sar ── */
    {
        register uint32_t val asm("a0") = 13;
        register uint32_t readback asm("a1");
        asm volatile(
            "esp.movx.w.sar %[val]\n"
            "esp.movx.r.sar %[rb]\n"
            : [rb] "=r"(readback)
            : [val] "r"(val) :
        );
        int ok = (readback == 13);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: movx.w/r.sar"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: movx.w/r.sar (got %u)", (unsigned)readback); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.movx.w.sar.bytes / esp.movx.r.sar.bytes ── */
    {
        register uint32_t val asm("a0") = 7;
        register uint32_t readback asm("a1");
        asm volatile(
            "esp.movx.w.sar.bytes %[val]\n"
            "esp.movx.r.sar.bytes %[rb]\n"
            : [rb] "=r"(readback)
            : [val] "r"(val) :
        );
        int ok = (readback == 7);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: movx.w/r.sar.bytes"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: movx.w/r.sar.bytes (got %u)", (unsigned)readback); }
        *pass += ok;
        (*total)++;
    }


    /*
     * ── esp.vsld.8 / vsrd.8 q2, q0, q1 ──
     * Vector slide left/right. SAR controls the shift amount.
     * Behavior unclear — may be byte-slide across q0:q1 concatenation,
     * or per-element bit shift, or whole-register bit shift.
     *
     * Test with non-zero first byte and SAR=8 to discover the pattern.
     * Using q0 != q1 to distinguish slide source.
     */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xA0,0xB0,0xC0,0xD0,0xE0,0xF0,0xFF};
        uint8_t b[16] __attribute__((aligned(16))) = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x11};
        uint8_t out_l[16] __attribute__((aligned(16)));
        uint8_t out_r[16] __attribute__((aligned(16)));
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *pol asm("a2") = out_l;
        register uint8_t *por asm("a3") = out_r;
        register uint32_t sar asm("a4") = 8; /* 8 bits = 1 byte */
        asm volatile(
            "esp.movx.w.sar %[sar]\n"
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vsld.8 q2, q0, q1\n"
            "esp.vst.128.ip q2, %[pol], 0\n"
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vsrd.8 q2, q0, q1\n"
            "esp.vst.128.ip q2, %[por], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [pol] "+r"(pol), [por] "+r"(por)
            : [sar] "r"(sar) : "memory"
        );
        pie_log_vec_u8("vsld.8 SAR=8", out_l);
        pie_log_vec_u8("vsrd.8 SAR=8", out_r);
        /* Check that output differs from input — scan all bytes */
        int ok_l = (memcmp(out_l, a, 16) != 0);
        int ok_r = (memcmp(out_r, a, 16) != 0);
        if (ok_l) { ESP_LOGI(PIE_TAG, "  PASS: vsld.8 SAR=8 (output differs from input)"); }
        else      { ESP_LOGE(PIE_TAG, "  FAIL: vsld.8 SAR=8 (output matches input)"); }
        if (ok_r) { ESP_LOGI(PIE_TAG, "  PASS: vsrd.8 SAR=8 (output differs from input)"); }
        else      { ESP_LOGE(PIE_TAG, "  FAIL: vsrd.8 SAR=8 (output matches input)"); }
        *pass += ok_l; (*total)++;
        *pass += ok_r; (*total)++;
    }

    /*
     * ── esp.vsld.16 / vsrd.16 ──
     * SAR-controlled bit shift for 16-bit element granularity.
     * P4 hardware finding: SAR=4 and SAR=16 both produce no change.
     * Trying SAR=8 (which works for vsld.8) and multiple SAR values
     * to discover which ones cause visible output changes.
     */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {0x1234,0x5678,0x9ABC,0xDEF0,0x1111,0x2222,0x3333,0x4444};
        uint16_t b[8] __attribute__((aligned(16))) = {0xAAAA,0xBBBB,0xCCCC,0xDDDD,0xEEEE,0xFFFF,0x1010,0x2020};
        uint16_t out_l[8] __attribute__((aligned(16)));
        uint16_t out_r[8] __attribute__((aligned(16)));
        register uint16_t *pa asm("a0") = a;
        register uint16_t *pb asm("a1") = b;
        register uint16_t *pol asm("a2") = out_l;
        register uint16_t *por asm("a3") = out_r;
        register uint32_t sar asm("a4") = 8; /* try 8 bits */
        asm volatile(
            "esp.movx.w.sar %[sar]\n"
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vsld.16 q2, q0, q1\n"
            "esp.vst.128.ip q2, %[pol], 0\n"
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vsrd.16 q2, q0, q1\n"
            "esp.vst.128.ip q2, %[por], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [pol] "+r"(pol), [por] "+r"(por)
            : [sar] "r"(sar) : "memory"
        );
        pie_log_vec_x32("vsld.16 SAR=8", (uint32_t*)out_l);
        pie_log_vec_x32("vsrd.16 SAR=8", (uint32_t*)out_r);
        int ok_l = (memcmp(out_l, a, 16) != 0);
        int ok_r = (memcmp(out_r, a, 16) != 0);
        if (!ok_l || !ok_r) {
            /* SAR=8 didn't work either. Try SAR=1 */
            sar = 1;
            pa = a; pb = b;
            asm volatile(
                "esp.movx.w.sar %[sar]\n"
                "esp.vld.128.ip q0, %[pa], 0\n"
                "esp.vld.128.ip q1, %[pb], 0\n"
                "esp.vsld.16 q2, q0, q1\n"
                "esp.vst.128.ip q2, %[pol], 0\n"
                "esp.vld.128.ip q0, %[pa], 0\n"
                "esp.vld.128.ip q1, %[pb], 0\n"
                "esp.vsrd.16 q2, q0, q1\n"
                "esp.vst.128.ip q2, %[por], 0\n"
                : [pa] "+r"(pa), [pb] "+r"(pb), [pol] "+r"(pol), [por] "+r"(por)
                : [sar] "r"(sar) : "memory"
            );
            pie_log_vec_x32("vsld.16 SAR=1", (uint32_t*)out_l);
            pie_log_vec_x32("vsrd.16 SAR=1", (uint32_t*)out_r);
            ok_l = (memcmp(out_l, a, 16) != 0);
            ok_r = (memcmp(out_r, a, 16) != 0);
        }
        if (ok_l) { ESP_LOGI(PIE_TAG, "  PASS: vsld.16 (output differs from input)"); }
        else      { ESP_LOGE(PIE_TAG, "  FAIL: vsld.16 (no SAR value produces change — may not be implemented on P4)"); }
        if (ok_r) { ESP_LOGI(PIE_TAG, "  PASS: vsrd.16 (output differs from input)"); }
        else      { ESP_LOGE(PIE_TAG, "  FAIL: vsrd.16 (no SAR value produces change — may not be implemented on P4)"); }
        *pass += ok_l; (*total)++;
        *pass += ok_r; (*total)++;
    }
    /*
     * ── esp.vsld.32 / vsrd.32 ──
     * 32-bit element slide. SAR=32 = 1 element width.
     */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {0x11111111,0x22222222,0x33333333,0x44444444};
        uint32_t b[4] __attribute__((aligned(16))) = {0x55555555,0x66666666,0x77777777,0x88888888};
        uint32_t out_l[4] __attribute__((aligned(16)));
        uint32_t out_r[4] __attribute__((aligned(16)));
        register uint32_t *pa asm("a0") = a;
        register uint32_t *pb asm("a1") = b;
        register uint32_t *pol asm("a2") = out_l;
        register uint32_t *por asm("a3") = out_r;
        register uint32_t sar asm("a4") = 32;
        asm volatile(
            "esp.movx.w.sar %[sar]\n"
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vsld.32 q2, q0, q1\n"
            "esp.vst.128.ip q2, %[pol], 0\n"
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vsrd.32 q2, q0, q1\n"
            "esp.vst.128.ip q2, %[por], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [pol] "+r"(pol), [por] "+r"(por)
            : [sar] "r"(sar) : "memory"
        );
        pie_log_vec_x32("vsld.32 SAR=32", out_l);
        pie_log_vec_x32("vsrd.32 SAR=32", out_r);
        int ok_l = (memcmp(out_l, a, 16) != 0);
        int ok_r = (memcmp(out_r, a, 16) != 0);
        if (ok_l) { ESP_LOGI(PIE_TAG, "  PASS: vsld.32 SAR=32 (output differs from input)"); }
        else      { ESP_LOGE(PIE_TAG, "  FAIL: vsld.32 SAR=32 (output matches input)"); }
        if (ok_r) { ESP_LOGI(PIE_TAG, "  PASS: vsrd.32 SAR=32 (output differs from input)"); }
        else      { ESP_LOGE(PIE_TAG, "  FAIL: vsrd.32 SAR=32 (output matches input)"); }
        *pass += ok_l; (*total)++;
        *pass += ok_r; (*total)++;
    }


    /* ── esp.vmulas.s16.qacc ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {10, -20, 300, -400, 50, 100, -200, 500};
        int16_t b[8] __attribute__((aligned(16))) = {3, -5, 7, -2, 100, -50, 10, 2};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {30, 100, 2100, 800, 5000, -5000, -2000, 1000};
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
        *pass += pie_check("srcmb.s16.qacc", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmulas.s16.qacc ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {10, -20, 300, -400, 50, 100, -200, 500};
        int16_t b[8] __attribute__((aligned(16))) = {3, -5, 7, -2, 100, -50, 10, 2};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {30, 100, 2100, 800, 5000, -5000, -2000, 1000};
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        register uint32_t sar_val asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vmulas.s16.qacc q0, q1\n"
            "esp.srcmb.u16.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar_val) : "memory"
        );
        *pass += pie_check("srcmb.u16.qacc", out, expect, 16);
        (*total)++;
    }


    /*
     * ── esp.srcmb.s8.qacc / esp.srcmb.u8.qacc ──
     * Extract from QACC as 8-bit values instead of 16-bit.
     * QACC contains 8 x 32-bit lanes from vmulas.s16.qacc.
     * srcmb.s8 extracts the low 8 bits of each lane (signed).
     * srcmb.u8 extracts the low 8 bits (unsigned).
     *
     * Using simple products: a={1,2,3,4,5,6,7,8}, b={10,10,...}
     * QACC lanes = {10,20,30,40,50,60,70,80}
     * s8 extraction: low byte of each = {10,20,30,40,50,60,70,80}
     * These all fit in 8 bits, so s8 and u8 should match s16 extraction
     * but packed into 8 bytes (with 8 more bytes of... unknown).
     */
    {
        int16_t a[8] __attribute__((aligned(16))) = {1, 2, 3, 4, 5, 6, 7, 8};
        int16_t b[8] __attribute__((aligned(16))) = {10, 10, 10, 10, 10, 10, 10, 10};
        uint8_t out[16] __attribute__((aligned(16)));
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        register uint32_t sar_val asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vmulas.s16.qacc q0, q1\n"
            "esp.srcmb.s8.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar_val) : "memory"
        );
        pie_log_vec_u8("srcmb.s8", out);
        /* Expect products 10,20,...,80 packed as s8 somewhere in the output */
        int ok = (out[0] == 10 || out[0] == 20);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: srcmb.s8.qacc (out[0]=%u)", out[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: srcmb.s8.qacc (out[0]=%u, expect 10 or 20)", out[0]); }
        *pass += ok; (*total)++;
    }

    {
        int16_t a[8] __attribute__((aligned(16))) = {1, 2, 3, 4, 5, 6, 7, 8};
        int16_t b[8] __attribute__((aligned(16))) = {10, 10, 10, 10, 10, 10, 10, 10};
        uint8_t out[16] __attribute__((aligned(16)));
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        register uint32_t sar_val asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vmulas.s16.qacc q0, q1\n"
            "esp.srcmb.u8.qacc q2, %[sar], 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar_val) : "memory"
        );
        pie_log_vec_u8("srcmb.u8", out);
        int ok = (out[0] == 10 || out[0] == 20);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: srcmb.u8.qacc (out[0]=%u)", out[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: srcmb.u8.qacc (out[0]=%u, expect 10 or 20)", out[0]); }
        *pass += ok; (*total)++;
    }

    /*
     * ── esp.srcmb.{s16,s8,u16,u8}.q.qacc qd, qs, imm ──
     * Variant of srcmb that takes a Q register input alongside QACC.
     * The Q register (qs) may provide shift control or rounding info.
     * imm=0 for baseline behavior.
     *
     * Previous hardware runs: all outputs were zero. This may mean the
     * instruction requires specific SAR/state, or the Q input modifies
     * the extraction differently. Use same simple data and log output.
     */
    {
        int16_t a[8] __attribute__((aligned(16))) = {1, 2, 3, 4, 5, 6, 7, 8};
        int16_t b[8] __attribute__((aligned(16))) = {10, 10, 10, 10, 10, 10, 10, 10};
        int16_t out[8] __attribute__((aligned(16)));
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vmulas.s16.qacc q0, q1\n"
            "esp.srcmb.s16.q.qacc q2, q0, 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        pie_log_vec_s16("srcmb.s16.q", out);
        int ok = (out[0] != 0);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: srcmb.s16.q.qacc (out[0]=%d)", out[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: srcmb.s16.q.qacc (all zero — may need specific state)"); }
        *pass += ok; (*total)++;
    }

    {
        int16_t a[8] __attribute__((aligned(16))) = {1, 2, 3, 4, 5, 6, 7, 8};
        int16_t b[8] __attribute__((aligned(16))) = {10, 10, 10, 10, 10, 10, 10, 10};
        uint8_t out[16] __attribute__((aligned(16)));
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vmulas.s16.qacc q0, q1\n"
            "esp.srcmb.s8.q.qacc q2, q0, 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        pie_log_vec_u8("srcmb.s8.q", out);
        int ok = (out[0] != 0);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: srcmb.s8.q.qacc (out[0]=%u)", out[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: srcmb.s8.q.qacc (all zero)"); }
        *pass += ok; (*total)++;
    }

    {
        int16_t a[8] __attribute__((aligned(16))) = {1, 2, 3, 4, 5, 6, 7, 8};
        int16_t b[8] __attribute__((aligned(16))) = {10, 10, 10, 10, 10, 10, 10, 10};
        int16_t out[8] __attribute__((aligned(16)));
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vmulas.s16.qacc q0, q1\n"
            "esp.srcmb.u16.q.qacc q2, q0, 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        pie_log_vec_s16("srcmb.u16.q", out);
        int ok = (out[0] != 0);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: srcmb.u16.q.qacc (out[0]=%d)", out[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: srcmb.u16.q.qacc (all zero)"); }
        *pass += ok; (*total)++;
    }

    {
        int16_t a[8] __attribute__((aligned(16))) = {1, 2, 3, 4, 5, 6, 7, 8};
        int16_t b[8] __attribute__((aligned(16))) = {10, 10, 10, 10, 10, 10, 10, 10};
        uint8_t out[16] __attribute__((aligned(16)));
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.qacc\n"
            "esp.vmulas.s16.qacc q0, q1\n"
            "esp.srcmb.u8.q.qacc q2, q0, 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        pie_log_vec_u8("srcmb.u8.q", out);
        int ok = (out[0] != 0);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: srcmb.u8.q.qacc (out[0]=%u)", out[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: srcmb.u8.q.qacc (all zero)"); }
        *pass += ok; (*total)++;
    }


    /* ── esp.srci.2q ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        uint8_t b[16] __attribute__((aligned(16))) = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
        uint8_t out_a[16] __attribute__((aligned(16)));
        /* Hardware-confirmed: shifts q0 right by 5 bytes, zero-fills from left */
        uint8_t expect[16] = {5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *poa asm("a2") = out_a;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.srci.2q    q0, q1, 4\n"
            "esp.vst.128.ip q0, %[poa], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [poa] "+r"(poa) :: "memory"
        );
        *pass += pie_check("srci.2q imm=4", out_a, expect, 16);
        (*total)++;
    }


    /* ── esp.slci.2q ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        uint8_t b[16] __attribute__((aligned(16))) = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
        uint8_t out_a[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {27,28,29,30,31,0,1,2,3,4,5,6,7,8,9,10};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *poa asm("a2") = out_a;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.slci.2q    q0, q1, 4\n"
            "esp.vst.128.ip q0, %[poa], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [poa] "+r"(poa) :: "memory"
        );
        *pass += pie_check("slci.2q imm=4", out_a, expect, 16);
        (*total)++;
    }


    /* ── esp.srcxxp.2q ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        uint8_t b[16] __attribute__((aligned(16))) = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
        uint8_t out_a[16] __attribute__((aligned(16)));
        /* Hardware-confirmed: shifts q0 right by sh1+1 bytes, zero-fills from left */
        uint8_t expect[16] = {5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *poa asm("a2") = out_a;
        register int32_t sh1 asm("a3") = 4;
        register int32_t sh2 asm("a4") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.srcxxp.2q q0, q1, %[sh1], %[sh2]\n"
            "esp.vst.128.ip q0, %[poa], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [poa] "+r"(poa)
            : [sh1] "r"(sh1), [sh2] "r"(sh2) : "memory"
        );
        *pass += pie_check("srcxxp.2q sh=4", out_a, expect, 16);
        (*total)++;
    }


    /* ── esp.slcxxp.2q ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        uint8_t b[16] __attribute__((aligned(16))) = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
        uint8_t out_a[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {27,28,29,30,31,0,1,2,3,4,5,6,7,8,9,10};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *poa asm("a2") = out_a;
        register int32_t sh1 asm("a3") = 4;
        register int32_t sh2 asm("a4") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.slcxxp.2q q0, q1, %[sh1], %[sh2]\n"
            "esp.vst.128.ip q0, %[poa], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [poa] "+r"(poa)
            : [sh1] "r"(sh1), [sh2] "r"(sh2) : "memory"
        );
        *pass += pie_check("slcxxp.2q sh=4", out_a, expect, 16);
        (*total)++;
    }


    /* ── esp.srs.s.xacc / esp.srs.u.xacc ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {100,200,300,400,500,600,700,800};
        int16_t b[8] __attribute__((aligned(16))) = {2,3,4,5,6,7,8,9};
        /* dot = 100*2+200*3+300*4+400*5+500*6+600*7+700*8+800*9 = 24000 */
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register uint32_t rs asm("a2");
        register uint32_t ru asm("a3");
        register uint32_t shift asm("a4") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.zero.xacc\n"
            "esp.vmulas.s16.xacc q0, q1\n"
            "esp.srs.s.xacc %[rs], %[shift]\n"
            "esp.srs.u.xacc %[ru], %[shift]\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [rs] "=r"(rs), [ru] "=r"(ru)
            : [shift] "r"(shift) : "memory"
        );
        int ok_s = ((int32_t)rs == 24000);
        int ok_u = ((uint32_t)ru == 24000);
        if (ok_s) { ESP_LOGI(PIE_TAG, "  PASS: srs.s.xacc"); }
        else      { ESP_LOGE(PIE_TAG, "  FAIL: srs.s.xacc (got %d, expect 24000)", (int)rs); }
        if (ok_u) { ESP_LOGI(PIE_TAG, "  PASS: srs.u.xacc"); }
        else      { ESP_LOGE(PIE_TAG, "  FAIL: srs.u.xacc (got %u, expect 24000)", (unsigned)ru); }
        *pass += ok_s; (*total)++;
        *pass += ok_u; (*total)++;
    }

}
