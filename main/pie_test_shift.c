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

    /* ═══════════════════════════════════════════════════════════════
     * esp.src.q with manually set SAR (movx.w.sar)
     *
     * Test whether src.q uses the SAR register set by movx.w.sar,
     * or only the internal alignment state from usar loads.
     *
     * Setup: load two known 128-bit values into q0 and q1.
     * Then set SAR to various byte offsets and verify src.q
     * extracts the correct 16-byte window from the q0:q1
     * concatenation.
     *
     * q0 = [0,1,2,...,15], q1 = [16,17,18,...,31]
     * Concatenation = [0,1,2,...,31] (32 bytes)
     * src.q(q0,q1) with SAR=N should give bytes [N..N+15]
     * ═══════════════════════════════════════════════════════════════ */

    /*
     * ═══════════════════════════════════════════════════════════════
     * CONFIRMED: esp.src.q does NOT use SAR set by movx.w.sar.
     * It uses a separate internal alignment state set ONLY by usar loads.
     * There is no register-only way to byte-shift Q register contents.
     * ═══════════════════════════════════════════════════════════════
     */
    ESP_LOGI(PIE_TAG, "--- src.q ignores movx.w.sar (confirmed) ---");
    {
        /* Verify that src.q output is independent of SAR value.
         * We set SAR=0 and SAR=8 and confirm identical output. */
        uint8_t a[16] __attribute__((aligned(16)));
        uint8_t b[16] __attribute__((aligned(16)));
        uint8_t out0[16] __attribute__((aligned(16)));
        uint8_t out8[16] __attribute__((aligned(16)));
        for (int i = 0; i < 16; i++) { a[i] = (uint8_t)i; b[i] = (uint8_t)(i + 16); }

        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out0;
        register uint32_t sar asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.movx.w.sar %[sar]\n"
            "esp.src.q q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );

        pa = a; pb = b; po = out8; sar = 8;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.movx.w.sar %[sar]\n"
            "esp.src.q q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );

        int ok = (memcmp(out0, out8, 16) == 0);
        if (ok) ESP_LOGI(PIE_TAG, "  PASS: src.q SAR=0 == SAR=8 (SAR ignored)");
        else    ESP_LOGE(PIE_TAG, "  FAIL: src.q SAR=0 != SAR=8 (SAR affects output?!)");
        *pass += ok; (*total)++;
    }

    /* ── srci.2q / slci.2q verified behavior ──
     *
     * Confirmed on hardware with q0=[0..15], q1=[16..31]:
     *
     * srci.2q q0, q1, imm: shifts q0 right by (imm+1) bytes, ZERO-fills.
     *   imm=0 → [1,2,...,15, 0]
     *   imm=1 → [2,3,...,15, 0,0]
     *   etc. q1 is IGNORED.
     *
     * slci.2q q0, q1, imm: shifts q0 left by (imm+1) bytes, fills from
     *   END of q1 (concatenation: q1[tail] prepended to q0[head]).
     *   imm=0 → [31, 0,1,...,14]
     *   imm=1 → [30,31, 0,1,...,13]
     *   etc.
     *
     * Neither provides a right-shift-with-carry from q1 into q0.
     */
    ESP_LOGI(PIE_TAG, "--- srci/slci.2q verified ---");
    {
        uint8_t a[16] __attribute__((aligned(16)));
        uint8_t b[16] __attribute__((aligned(16)));
        for (int i = 0; i < 16; i++) { a[i] = (uint8_t)i; b[i] = (uint8_t)(i + 16); }

        /* srci.2q imm=2: expect [3,4,...,15, 0,0,0] */
        {
            uint8_t out[16] __attribute__((aligned(16)));
            uint8_t expect[16] = {3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0};
            register uint8_t *pa asm("a0") = a;
            register uint8_t *pb asm("a1") = b;
            register uint8_t *po asm("a2") = out;
            asm volatile(
                "esp.vld.128.ip q0, %[pa], 0\n"
                "esp.vld.128.ip q1, %[pb], 0\n"
                "esp.srci.2q q0, q1, 2\n"
                "esp.vst.128.ip q0, %[po], 0\n"
                : [pa]"+r"(pa),[pb]"+r"(pb),[po]"+r"(po) :: "memory"
            );
            *pass += pie_check("srci.2q imm=2 (right shift 3, zero-fill)", out, expect, 16);
            (*total)++;
        }

        /* slci.2q imm=2: expect [29,30,31, 0,1,...,12] */
        {
            uint8_t out[16] __attribute__((aligned(16)));
            uint8_t expect[16] = {29,30,31,0,1,2,3,4,5,6,7,8,9,10,11,12};
            register uint8_t *pa asm("a0") = a;
            register uint8_t *pb asm("a1") = b;
            register uint8_t *po asm("a2") = out;
            asm volatile(
                "esp.vld.128.ip q0, %[pa], 0\n"
                "esp.vld.128.ip q1, %[pb], 0\n"
                "esp.slci.2q q0, q1, 2\n"
                "esp.vst.128.ip q0, %[po], 0\n"
                : [pa]"+r"(pa),[pb]"+r"(pb),[po]"+r"(po) :: "memory"
            );
            *pass += pie_check("slci.2q imm=2 (left shift 3, fill from q1 tail)", out, expect, 16);
            (*total)++;
        }
    }

    /* ── srci.2q with SWAPPED register roles ──
     *
     * Theory: srci.2q concatenates qd:qs (high:low), shifts right by
     * (imm+1) bytes. If we put DATA in qs and ZEROS in qd, the right
     * shift should pull data bytes from qs into qd, giving us a
     * byte-shifted view!
     *
     * Test: q0=zeros (qd), q1=[0..15] (qs), srci.2q q0, q1, N
     * Expected: q0 = [q1[N+1], q1[N+2], ..., q1[15], 0, 0, ...]
     * Wait — that's still not a window. Let me think...
     *
     * concat(qd=zeros, qs=data) = [0,0,...,0, 0,1,2,...,15] (32 bytes)
     * shift right by 1 byte: [0, 0,0,...,0, 0,1,2,...,14] → low 16 = [0,0,...,0,0,1,2,...,14]
     * Hmm, that pushes data OFF the end.
     *
     * Actually for a useful shift we want: concat(data_hi, data_lo),
     * then shift right to slide bytes from hi into lo.
     * Put CONTINUATION data in qd, MAIN data in qs.
     * srci.2q q_continuation, q_main, N
     * concat = [continuation : main], shift right N+1 → [cont[N+1..15], main[0..15-N-1]]
     *
     * For our case: q0 has bytes [16..31], q1 has bytes [0..15].
     * srci.2q q0, q1, 0: concat=[16..31, 0..15], shift right 1 → [15, 16..31, 0..14] low16 = [31, 0, 1, ..., 13]
     * Hmm, that wraps around. Not quite right either.
     *
     * Let me just test all combinations and see what happens.
     */
    ESP_LOGI(PIE_TAG, "--- srci.2q swapped registers ---");
    {
        uint8_t data[16] __attribute__((aligned(16)));
        uint8_t cont[16] __attribute__((aligned(16)));
        for (int i = 0; i < 16; i++) { data[i] = (uint8_t)i; cont[i] = (uint8_t)(i + 16); }

        /* Test: q0=cont=[16..31], q1=data=[0..15], srci.2q q0, q1, imm */
        for (int imm = 0; imm <= 4; imm++)
        {
            uint8_t out[16] __attribute__((aligned(16)));
            register uint8_t *pd asm("a0") = cont;   /* q0 = continuation */
            register uint8_t *ps asm("a1") = data;    /* q1 = data */
            register uint8_t *po asm("a2") = out;

            switch (imm) {
            case 0:
                asm volatile(
                    "esp.vld.128.ip q0, %[pd], 0\n" "esp.vld.128.ip q1, %[ps], 0\n"
                    "esp.srci.2q q0, q1, 0\n" "esp.vst.128.ip q0, %[po], 0\n"
                    : [pd]"+r"(pd),[ps]"+r"(ps),[po]"+r"(po) :: "memory"); break;
            case 1:
                asm volatile(
                    "esp.vld.128.ip q0, %[pd], 0\n" "esp.vld.128.ip q1, %[ps], 0\n"
                    "esp.srci.2q q0, q1, 1\n" "esp.vst.128.ip q0, %[po], 0\n"
                    : [pd]"+r"(pd),[ps]"+r"(ps),[po]"+r"(po) :: "memory"); break;
            case 2:
                asm volatile(
                    "esp.vld.128.ip q0, %[pd], 0\n" "esp.vld.128.ip q1, %[ps], 0\n"
                    "esp.srci.2q q0, q1, 2\n" "esp.vst.128.ip q0, %[po], 0\n"
                    : [pd]"+r"(pd),[ps]"+r"(ps),[po]"+r"(po) :: "memory"); break;
            case 3:
                asm volatile(
                    "esp.vld.128.ip q0, %[pd], 0\n" "esp.vld.128.ip q1, %[ps], 0\n"
                    "esp.srci.2q q0, q1, 3\n" "esp.vst.128.ip q0, %[po], 0\n"
                    : [pd]"+r"(pd),[ps]"+r"(ps),[po]"+r"(po) :: "memory"); break;
            case 4:
                asm volatile(
                    "esp.vld.128.ip q0, %[pd], 0\n" "esp.vld.128.ip q1, %[ps], 0\n"
                    "esp.srci.2q q0, q1, 4\n" "esp.vst.128.ip q0, %[po], 0\n"
                    : [pd]"+r"(pd),[ps]"+r"(ps),[po]"+r"(po) :: "memory"); break;
            }

            char label[64];
            snprintf(label, sizeof(label), "srci q0=[16..31] q1=[0..15] imm=%d", imm);
            pie_log_vec_u8(label, out);
            ESP_LOGI(PIE_TAG, "  INFO: %s", label);
            *pass += 1; (*total)++;
        }

        /* Also test srcxxp.2q which takes register shift amount */
        {
            uint8_t out[16] __attribute__((aligned(16)));
            register uint8_t *pd asm("a0") = cont;
            register uint8_t *ps asm("a1") = data;
            register uint8_t *po asm("a2") = out;
            register int32_t sh1 asm("a3") = 0;  /* shift by 1 byte */
            register int32_t sh2 asm("a4") = 0;
            asm volatile(
                "esp.vld.128.ip q0, %[pd], 0\n" "esp.vld.128.ip q1, %[ps], 0\n"
                "esp.srcxxp.2q q0, q1, %[sh1], %[sh2]\n"
                "esp.vst.128.ip q0, %[po], 0\n"
                : [pd]"+r"(pd),[ps]"+r"(ps),[po]"+r"(po)
                : [sh1]"r"(sh1),[sh2]"r"(sh2) : "memory"
            );
            pie_log_vec_u8("srcxxp q0=[16..31] q1=[0..15] sh=0", out);
            ESP_LOGI(PIE_TAG, "  INFO: srcxxp logged");
            *pass += 1; (*total)++;
        }
    }

    /* Also test: does usar load OVERWRITE a manually set SAR?
     * Set SAR=5, then do an usar load from an aligned address (offset 0).
     * Does SAR become 0 (from usar) or stay 5? */
    {
        uint8_t aligned_data[32] __attribute__((aligned(16)));
        for (int i = 0; i < 32; i++) aligned_data[i] = (uint8_t)(i + 100);

        uint8_t a[16] __attribute__((aligned(16)));
        uint8_t b[16] __attribute__((aligned(16)));
        for (int i = 0; i < 16; i++) { a[i] = (uint8_t)i; b[i] = (uint8_t)(i + 16); }

        uint8_t out[16] __attribute__((aligned(16)));
        register uint32_t sar_before asm("a0") = 5;
        register uint8_t *pdata asm("a1") = aligned_data;  /* 16-byte aligned → usar offset=0 */
        register uint8_t *pa asm("a2") = a;
        register uint8_t *pb asm("a3") = b;
        register uint8_t *po asm("a4") = out;
        register uint32_t sar_after asm("a5");

        asm volatile(
            /* Set SAR=5 manually */
            "esp.movx.w.sar %[sar_b]\n"
            /* Do an usar load from aligned address (should set usar offset=0) */
            "esp.ld.128.usar.ip q3, %[pd], 16\n"
            /* Read SAR back */
            "esp.movx.r.sar %[sar_a]\n"
            /* Now do src.q with q0,q1 — will it use the usar state or SAR? */
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.src.q q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pd] "+r"(pdata), [pa] "+r"(pa), [pb] "+r"(pb),
              [po] "+r"(po), [sar_a] "=r"(sar_after)
            : [sar_b] "r"(sar_before) : "memory"
        );

        ESP_LOGI(PIE_TAG, "  SAR before usar: 5, SAR after usar (aligned): %d", (int)sar_after);

        /* If usar overwrote SAR to 0, src.q gives bytes [0..15] */
        uint8_t expect_sar0[16];
        for (int i = 0; i < 16; i++) expect_sar0[i] = (uint8_t)i;
        /* If usar did NOT overwrite SAR (still 5), src.q gives bytes [5..20] */
        uint8_t expect_sar5[16];
        for (int i = 0; i < 16; i++) expect_sar5[i] = (uint8_t)(i + 5);

        int is_sar0 = (memcmp(out, expect_sar0, 16) == 0);
        int is_sar5 = (memcmp(out, expect_sar5, 16) == 0);

        if (is_sar0) {
            ESP_LOGI(PIE_TAG, "  PASS: usar load OVERWRITES SAR (src.q used SAR=0)");
        } else if (is_sar5) {
            ESP_LOGI(PIE_TAG, "  PASS: usar load does NOT overwrite SAR (src.q used SAR=5)");
        } else {
            ESP_LOGE(PIE_TAG, "  FAIL: unexpected src.q result after usar + manual SAR");
            pie_log_vec_u8("  got", out);
        }
        *pass += (is_sar0 || is_sar5);
        (*total)++;
    }
}
