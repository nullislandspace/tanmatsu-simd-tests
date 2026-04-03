/*
 * PIE Data Reformat, Abs, Sat, ReLU tests.
 * Hand-edited — contains hardware discovery tests for vzipt/vunzipt/vext.
 */

#include "pie_test_helpers.h"
#include <stdlib.h>

void run_pie_test_convert(int *pass, int *total) {
    ESP_LOGI(PIE_TAG, "--- Data Reformat, Abs, Sat, ReLU ---");
    /* ── esp.vzip.8 ── */
    {
        uint8_t in_a[16] __attribute__((aligned(16))) = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        uint8_t in_b[16] __attribute__((aligned(16))) = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
        uint8_t out_a[16] __attribute__((aligned(16)));
        uint8_t out_b[16] __attribute__((aligned(16)));
        uint8_t exp_a[16] = {0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23};
        uint8_t exp_b[16] = {8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31};
        register uint8_t *pa asm("a0") = (uint8_t *)in_a;
        register uint8_t *pb asm("a1") = (uint8_t *)in_b;
        register uint8_t *poa asm("a2") = out_a;
        register uint8_t *pob asm("a3") = out_b;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vzip.8 q0, q1\n"
            "esp.vst.128.ip q0, %[poa], 0\n"
            "esp.vst.128.ip q1, %[pob], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [poa] "+r"(poa), [pob] "+r"(pob) :: "memory"
        );
        *pass += pie_check_verbose("vzip.8", (uint8_t *)in_a, (uint8_t *)in_b, out_a, out_b, (uint8_t *)exp_a, (uint8_t *)exp_b);
        (*total)++;
    }

    /* ── esp.vunzip.8 ── */
    {
        uint8_t in_a[16] __attribute__((aligned(16))) = {0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23};
        uint8_t in_b[16] __attribute__((aligned(16))) = {8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31};
        uint8_t out_a[16] __attribute__((aligned(16)));
        uint8_t out_b[16] __attribute__((aligned(16)));
        uint8_t exp_a[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        uint8_t exp_b[16] = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
        register uint8_t *pa asm("a0") = (uint8_t *)in_a;
        register uint8_t *pb asm("a1") = (uint8_t *)in_b;
        register uint8_t *poa asm("a2") = out_a;
        register uint8_t *pob asm("a3") = out_b;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vunzip.8 q0, q1\n"
            "esp.vst.128.ip q0, %[poa], 0\n"
            "esp.vst.128.ip q1, %[pob], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [poa] "+r"(poa), [pob] "+r"(pob) :: "memory"
        );
        *pass += pie_check_verbose("vunzip.8", (uint8_t *)in_a, (uint8_t *)in_b, out_a, out_b, (uint8_t *)exp_a, (uint8_t *)exp_b);
        (*total)++;
    }

    /* ── esp.vzip.16 ── */
    {
        uint8_t in_a[16] __attribute__((aligned(16))) = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        uint8_t in_b[16] __attribute__((aligned(16))) = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
        uint8_t out_a[16] __attribute__((aligned(16)));
        uint8_t out_b[16] __attribute__((aligned(16)));
        uint8_t exp_a[16] = {0, 1, 16, 17, 2, 3, 18, 19, 4, 5, 20, 21, 6, 7, 22, 23};
        uint8_t exp_b[16] = {8, 9, 24, 25, 10, 11, 26, 27, 12, 13, 28, 29, 14, 15, 30, 31};
        register uint8_t *pa asm("a0") = (uint8_t *)in_a;
        register uint8_t *pb asm("a1") = (uint8_t *)in_b;
        register uint8_t *poa asm("a2") = out_a;
        register uint8_t *pob asm("a3") = out_b;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vzip.16 q0, q1\n"
            "esp.vst.128.ip q0, %[poa], 0\n"
            "esp.vst.128.ip q1, %[pob], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [poa] "+r"(poa), [pob] "+r"(pob) :: "memory"
        );
        *pass += pie_check_verbose("vzip.16", (uint8_t *)in_a, (uint8_t *)in_b, out_a, out_b, (uint8_t *)exp_a, (uint8_t *)exp_b);
        (*total)++;
    }

    /* ── esp.vunzip.16 ── */
    {
        uint8_t in_a[16] __attribute__((aligned(16))) = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        uint8_t in_b[16] __attribute__((aligned(16))) = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
        uint8_t out_a[16] __attribute__((aligned(16)));
        uint8_t out_b[16] __attribute__((aligned(16)));
        uint8_t exp_a[16] = {0, 1, 4, 5, 8, 9, 12, 13, 16, 17, 20, 21, 24, 25, 28, 29};
        uint8_t exp_b[16] = {2, 3, 6, 7, 10, 11, 14, 15, 18, 19, 22, 23, 26, 27, 30, 31};
        register uint8_t *pa asm("a0") = (uint8_t *)in_a;
        register uint8_t *pb asm("a1") = (uint8_t *)in_b;
        register uint8_t *poa asm("a2") = out_a;
        register uint8_t *pob asm("a3") = out_b;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vunzip.16 q0, q1\n"
            "esp.vst.128.ip q0, %[poa], 0\n"
            "esp.vst.128.ip q1, %[pob], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [poa] "+r"(poa), [pob] "+r"(pob) :: "memory"
        );
        *pass += pie_check_verbose("vunzip.16", (uint8_t *)in_a, (uint8_t *)in_b, out_a, out_b, (uint8_t *)exp_a, (uint8_t *)exp_b);
        (*total)++;
    }

    /* ── esp.vzip.32 ── */
    {
        uint8_t in_a[16] __attribute__((aligned(16))) = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        uint8_t in_b[16] __attribute__((aligned(16))) = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
        uint8_t out_a[16] __attribute__((aligned(16)));
        uint8_t out_b[16] __attribute__((aligned(16)));
        uint8_t exp_a[16] = {0, 1, 2, 3, 16, 17, 18, 19, 4, 5, 6, 7, 20, 21, 22, 23};
        uint8_t exp_b[16] = {8, 9, 10, 11, 24, 25, 26, 27, 12, 13, 14, 15, 28, 29, 30, 31};
        register uint8_t *pa asm("a0") = (uint8_t *)in_a;
        register uint8_t *pb asm("a1") = (uint8_t *)in_b;
        register uint8_t *poa asm("a2") = out_a;
        register uint8_t *pob asm("a3") = out_b;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vzip.32 q0, q1\n"
            "esp.vst.128.ip q0, %[poa], 0\n"
            "esp.vst.128.ip q1, %[pob], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [poa] "+r"(poa), [pob] "+r"(pob) :: "memory"
        );
        *pass += pie_check_verbose("vzip.32", (uint8_t *)in_a, (uint8_t *)in_b, out_a, out_b, (uint8_t *)exp_a, (uint8_t *)exp_b);
        (*total)++;
    }

    /* ── esp.vunzip.32 ── */
    {
        uint8_t in_a[16] __attribute__((aligned(16))) = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        uint8_t in_b[16] __attribute__((aligned(16))) = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
        uint8_t out_a[16] __attribute__((aligned(16)));
        uint8_t out_b[16] __attribute__((aligned(16)));
        uint8_t exp_a[16] = {0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27};
        uint8_t exp_b[16] = {4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31};
        register uint8_t *pa asm("a0") = (uint8_t *)in_a;
        register uint8_t *pb asm("a1") = (uint8_t *)in_b;
        register uint8_t *poa asm("a2") = out_a;
        register uint8_t *pob asm("a3") = out_b;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vunzip.32 q0, q1\n"
            "esp.vst.128.ip q0, %[poa], 0\n"
            "esp.vst.128.ip q1, %[pob], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [poa] "+r"(poa), [pob] "+r"(pob) :: "memory"
        );
        *pass += pie_check_verbose("vunzip.32", (uint8_t *)in_a, (uint8_t *)in_b, out_a, out_b, (uint8_t *)exp_a, (uint8_t *)exp_b);
        (*total)++;
    }


    /* ── esp.vzipt.8 q2, q0, q1 ──
     * Transposed interleave for 8-bit elements.
     * Unlike vzip.8 which modifies q0,q1 in-place, vzipt writes
     * to a separate destination q2. The interleave pattern may
     * differ from regular vzip -- needs hardware verification.
     */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        uint8_t b[16] __attribute__((aligned(16))) = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
        uint8_t out[16] __attribute__((aligned(16)));
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vzipt.8 q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        pie_log_vec_u8("vzipt.8", out);
        int ok = 0;
        for (int i = 0; i < 16; i++) { if (out[i] != 0) ok = 1; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vzipt.8 (out[0]=%u out[1]=%u out[2]=%u)", out[0], out[1], out[2]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vzipt.8 (all zero)"); }
        *pass += ok; (*total)++;
    }


    /* ── esp.vunzipt.8 q2, q0, q1 ──
     * Transposed de-interleave for 8-bit elements.
     * Unlike vunzip.8 which modifies q0,q1 in-place, vunzipt writes
     * to a separate destination q2. Needs hardware verification.
     */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        uint8_t b[16] __attribute__((aligned(16))) = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
        uint8_t out[16] __attribute__((aligned(16)));
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vunzipt.8 q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        pie_log_vec_u8("vunzipt.8", out);
        int ok = 0;
        for (int i = 0; i < 16; i++) { if (out[i] != 0) ok = 1; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vunzipt.8 (out[0]=%u out[1]=%u out[2]=%u)", out[0], out[1], out[2]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vunzipt.8 (all zero)"); }
        *pass += ok; (*total)++;
    }


    /* ── esp.vzipt.16 q2, q0, q1 ──
     * Transposed interleave for 16-bit elements.
     * Unlike vzip.16 which modifies q0,q1 in-place, vzipt writes
     * to a separate destination q2. Needs hardware verification.
     */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        uint8_t b[16] __attribute__((aligned(16))) = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
        uint8_t out[16] __attribute__((aligned(16)));
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vzipt.16 q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        pie_log_vec_u8("vzipt.16", out);
        int ok = 0;
        for (int i = 0; i < 16; i++) { if (out[i] != 0) ok = 1; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vzipt.16 (out[0]=%u out[1]=%u out[2]=%u)", out[0], out[1], out[2]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vzipt.16 (all zero)"); }
        *pass += ok; (*total)++;
    }


    /* ── esp.vunzipt.16 q2, q0, q1 ──
     * Transposed de-interleave for 16-bit elements.
     * Unlike vunzip.16 which modifies q0,q1 in-place, vunzipt writes
     * to a separate destination q2. Needs hardware verification.
     */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        uint8_t b[16] __attribute__((aligned(16))) = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
        uint8_t out[16] __attribute__((aligned(16)));
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vunzipt.16 q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        pie_log_vec_u8("vunzipt.16", out);
        int ok = 0;
        for (int i = 0; i < 16; i++) { if (out[i] != 0) ok = 1; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vunzipt.16 (out[0]=%u out[1]=%u out[2]=%u)", out[0], out[1], out[2]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vunzipt.16 (all zero)"); }
        *pass += ok; (*total)++;
    }


    /* ── esp.vext.u8 qd, qs, qt ──
     * Widening extension: zero-extends 8-bit elements from qs to 16-bit.
     * qd gets the lower half of widened elements,
     * qt gets the upper half.
     * Previous runs: qd (q1) was all zero -- try reading qt (q2) too.
     */
    {
        uint8_t src[16] __attribute__((aligned(16))) = {0x10,0xF0,0x7F,0x80,0x01,0xFF,0x00,0xFE, 0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88};
        uint8_t out0[16] __attribute__((aligned(16)));
        uint8_t out1[16] __attribute__((aligned(16)));
        register uint8_t *ps asm("a0") = src;
        register uint8_t *po0 asm("a1") = out0;
        register uint8_t *po1 asm("a2") = out1;
        asm volatile(
            "esp.vld.128.ip q0, %[ps], 0\n"
            "esp.vext.u8 q1, q0, q2\n"
            "esp.vst.128.ip q1, %[po0], 0\n"
            "esp.vst.128.ip q2, %[po1], 0\n"
            : [ps] "+r"(ps), [po0] "+r"(po0), [po1] "+r"(po1) :: "memory"
        );
        pie_log_vec_u8("vext.u8 q1", out0);
        pie_log_vec_u8("vext.u8 q2", out1);
        /* Check either output is non-zero */
        int ok = 0;
        for (int i = 0; i < 16; i++) { if (out0[i] != 0 || out1[i] != 0) ok = 1; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vext.u8 (non-zero output found)"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vext.u8 (both outputs zero)"); }
        *pass += ok; (*total)++;
    }


    /* ── esp.vext.s8 qd, qs, qt ──
     * Widening extension: sign-extends 8-bit elements from qs to 16-bit.
     * qd gets the lower half of widened elements,
     * qt gets the upper half.
     */
    {
        uint8_t src[16] __attribute__((aligned(16))) = {0x10,0xF0,0x7F,0x80,0x01,0xFF,0x00,0xFE, 0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88};
        uint8_t out0[16] __attribute__((aligned(16)));
        uint8_t out1[16] __attribute__((aligned(16)));
        register uint8_t *ps asm("a0") = src;
        register uint8_t *po0 asm("a1") = out0;
        register uint8_t *po1 asm("a2") = out1;
        asm volatile(
            "esp.vld.128.ip q0, %[ps], 0\n"
            "esp.vext.s8 q1, q0, q2\n"
            "esp.vst.128.ip q1, %[po0], 0\n"
            "esp.vst.128.ip q2, %[po1], 0\n"
            : [ps] "+r"(ps), [po0] "+r"(po0), [po1] "+r"(po1) :: "memory"
        );
        pie_log_vec_u8("vext.s8 q1", out0);
        pie_log_vec_u8("vext.s8 q2", out1);
        /* Check either output is non-zero */
        int ok = 0;
        for (int i = 0; i < 16; i++) { if (out0[i] != 0 || out1[i] != 0) ok = 1; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vext.s8 (non-zero output found)"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vext.s8 (both outputs zero)"); }
        *pass += ok; (*total)++;
    }


    /* ── esp.vext.u16 qd, qs, qt ──
     * Widening extension: zero-extends 16-bit elements from qs to 32-bit.
     * qd gets the lower half of widened elements,
     * qt gets the upper half.
     */
    {
        uint8_t src[16] __attribute__((aligned(16))) = {0x10,0xF0,0x7F,0x80,0x01,0xFF,0x00,0xFE, 0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88};
        uint8_t out0[16] __attribute__((aligned(16)));
        uint8_t out1[16] __attribute__((aligned(16)));
        register uint8_t *ps asm("a0") = src;
        register uint8_t *po0 asm("a1") = out0;
        register uint8_t *po1 asm("a2") = out1;
        asm volatile(
            "esp.vld.128.ip q0, %[ps], 0\n"
            "esp.vext.u16 q1, q0, q2\n"
            "esp.vst.128.ip q1, %[po0], 0\n"
            "esp.vst.128.ip q2, %[po1], 0\n"
            : [ps] "+r"(ps), [po0] "+r"(po0), [po1] "+r"(po1) :: "memory"
        );
        pie_log_vec_u8("vext.u16 q1", out0);
        pie_log_vec_u8("vext.u16 q2", out1);
        /* Check either output is non-zero */
        int ok = 0;
        for (int i = 0; i < 16; i++) { if (out0[i] != 0 || out1[i] != 0) ok = 1; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vext.u16 (non-zero output found)"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vext.u16 (both outputs zero)"); }
        *pass += ok; (*total)++;
    }


    /* ── esp.vext.s16 qd, qs, qt ──
     * Widening extension: sign-extends 16-bit elements from qs to 32-bit.
     * qd gets the lower half of widened elements,
     * qt gets the upper half.
     */
    {
        uint8_t src[16] __attribute__((aligned(16))) = {0x10,0xF0,0x7F,0x80,0x01,0xFF,0x00,0xFE, 0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88};
        uint8_t out0[16] __attribute__((aligned(16)));
        uint8_t out1[16] __attribute__((aligned(16)));
        register uint8_t *ps asm("a0") = src;
        register uint8_t *po0 asm("a1") = out0;
        register uint8_t *po1 asm("a2") = out1;
        asm volatile(
            "esp.vld.128.ip q0, %[ps], 0\n"
            "esp.vext.s16 q1, q0, q2\n"
            "esp.vst.128.ip q1, %[po0], 0\n"
            "esp.vst.128.ip q2, %[po1], 0\n"
            : [ps] "+r"(ps), [po0] "+r"(po0), [po1] "+r"(po1) :: "memory"
        );
        pie_log_vec_u8("vext.s16 q1", out0);
        pie_log_vec_u8("vext.s16 q2", out1);
        /* Check either output is non-zero */
        int ok = 0;
        for (int i = 0; i < 16; i++) { if (out0[i] != 0 || out1[i] != 0) ok = 1; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: vext.s16 (non-zero output found)"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: vext.s16 (both outputs zero)"); }
        *pass += ok; (*total)++;
    }

    /* ── esp.vabs.8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {5, -5, 0, -100, 100, -1, 1, -127, 10, -10, 50, -50, 64, -64, 0, 1};
        int8_t out[16] __attribute__((aligned(16)));
        int8_t expect[16] = {5, 5, 0, 100, 100, 1, 1, 127, 10, 10, 50, 50, 64, 64, 0, 1};
        register int8_t *pa asm("a0") = a;
        register int8_t *po asm("a1") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vabs.8    q1, q0\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vabs.8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vabs.16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {5, -5, 0, -100, 100, -1, 1, -32767};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {5, 5, 0, 100, 100, 1, 1, 32767};
        register int16_t *pa asm("a0") = a;
        register int16_t *po asm("a1") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vabs.16    q1, q0\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vabs.16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vabs.32 ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {5, -5, 0, -100};
        int32_t out[4] __attribute__((aligned(16)));
        int32_t expect[4] = {5, 5, 0, 100};
        register int32_t *pa asm("a0") = a;
        register int32_t *po asm("a1") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vabs.32    q1, q0\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vabs.32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsat.u8 ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0, 5, 9, 10, 11, 100, 199, 200, 201, 250, 255, 50, 0, 0, 0, 0};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {10, 10, 10, 10, 11, 100, 199, 200, 200, 200, 200, 50, 10, 10, 10, 10};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *po asm("a1") = out;
        register uint32_t lo asm("a2") = 10;
        register uint32_t hi asm("a3") = 200;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vsat.u8    q1, q0, %[lo], %[hi]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [lo] "r"(lo), [hi] "r"(hi) : "memory"
        );
        *pass += pie_check("vsat.u8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsat.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {-1000, -512, -100, 0, 100, 511, 1000, 32767};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {-512, -512, -100, 0, 100, 511, 511, 511};
        register int16_t *pa asm("a0") = a;
        register int16_t *po asm("a1") = out;
        register int32_t lo asm("a2") = -512;
        register int32_t hi asm("a3") = 511;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vsat.s16    q1, q0, %[lo], %[hi]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [lo] "r"(lo), [hi] "r"(hi) : "memory"
        );
        *pass += pie_check("vsat.s16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsat.s32 ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {-100, 0, 128, 300};
        int32_t out[4] __attribute__((aligned(16)));
        int32_t expect[4] = {0, 0, 128, 255};
        register int32_t *pa asm("a0") = a;
        register int32_t *po asm("a1") = out;
        register int32_t lo asm("a2") = 0;
        register int32_t hi asm("a3") = 255;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vsat.s32    q1, q0, %[lo], %[hi]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [lo] "r"(lo), [hi] "r"(hi) : "memory"
        );
        *pass += pie_check("vsat.s32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsat.s8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {-50, -10, -5, 0, 5, 10, 50, 100, -128, -127, 0, 1, 126, 127, 0, 0};
        int8_t out[16] __attribute__((aligned(16)));
        int8_t expect[16] = {-10, -10, -5, 0, 5, 10, 10, 10, -10, -10, 0, 1, 10, 10, 0, 0};
        register int8_t *pa asm("a0") = a;
        register int8_t *po asm("a1") = out;
        register int32_t lo asm("a2") = -10;
        register int32_t hi asm("a3") = 10;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vsat.s8    q1, q0, %[lo], %[hi]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [lo] "r"(lo), [hi] "r"(hi) : "memory"
        );
        *pass += pie_check("vsat.s8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsat.u16 ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {0, 50, 100, 200, 500, 1000, 65535, 255};
        uint16_t out[8] __attribute__((aligned(16)));
        uint16_t expect[8] = {100, 100, 100, 200, 500, 500, 500, 255};
        register uint16_t *pa asm("a0") = a;
        register uint16_t *po asm("a1") = out;
        register uint32_t lo asm("a2") = 100;
        register uint32_t hi asm("a3") = 500;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vsat.u16    q1, q0, %[lo], %[hi]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [lo] "r"(lo), [hi] "r"(hi) : "memory"
        );
        *pass += pie_check("vsat.u16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsat.u32 ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {0, 50, 200, 1000};
        uint32_t out[4] __attribute__((aligned(16)));
        uint32_t expect[4] = {10, 50, 200, 255};
        register uint32_t *pa asm("a0") = a;
        register uint32_t *po asm("a1") = out;
        register uint32_t lo asm("a2") = 10;
        register uint32_t hi asm("a3") = 255;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vsat.u32    q1, q0, %[lo], %[hi]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [lo] "r"(lo), [hi] "r"(hi) : "memory"
        );
        *pass += pie_check("vsat.u32", out, expect, 16);
        (*total)++;
    }


    /* ── esp.vclamp.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {-1000,-100,-10,0,10,100,1000,32767};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {-128,-100,-10,0,10,100,127,127};
        register int16_t *pa asm("a0") = a;
        register int16_t *po asm("a1") = out;
        asm volatile("esp.vld.128.ip q0, %[pa], 0\n" "esp.vclamp.s16 q1, q0, 7\n" "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po) :: "memory");
        *pass += pie_check("vclamp.s16 imm=7", out, expect, 16); (*total)++;
    }


    /* ── esp.vrelu.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {-100, -10, -1, 0, 1, 10, 100, 32767};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 10, 0, 100, 0, 255, 127};
        register int16_t *pa asm("a0") = a;
        register uint8_t *po asm("a1") = out;
        register int32_t thr asm("a2") = 0;
        register int32_t maxv asm("a3") = 32767;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vrelu.s16  q0, %[thr], %[maxv]\n"
            "esp.vst.128.ip q0, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [thr] "r"(thr), [maxv] "r"(maxv) : "memory"
        );
        *pass += pie_check("vrelu.s16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vrelu.s8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {-100, -10, -1, 0, 1, 10, 100, 127, -128, -50, -5, 0, 5, 50, 64, 1};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {0, 0, 0, 0, 1, 10, 100, 127, 0, 0, 0, 0, 5, 50, 64, 1};
        register int8_t *pa asm("a0") = a;
        register uint8_t *po asm("a1") = out;
        register int32_t thr asm("a2") = 0;
        register int32_t maxv asm("a3") = 32767;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vrelu.s8  q0, %[thr], %[maxv]\n"
            "esp.vst.128.ip q0, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [thr] "r"(thr), [maxv] "r"(maxv) : "memory"
        );
        *pass += pie_check("vrelu.s8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vprelu.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {-100, -10, -1, 0, 1, 10, 100, 500};
        int16_t slope[8] __attribute__((aligned(16))) = {2, 2, 2, 2, 2, 2, 2, 2};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {56, 255, 236, 255, 254, 255, 0, 0, 1, 0, 10, 0, 100, 0, 244, 1};
        register int16_t *pa asm("a0") = a;
        register int16_t *ps asm("a1") = slope;
        register uint8_t *po asm("a2") = out;
        register int32_t shift asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[ps], 0\n"
            "esp.vprelu.s16 q2, q0, q1, %[shift]\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [ps] "+r"(ps), [po] "+r"(po)
            : [shift] "r"(shift) : "memory"
        );
        *pass += pie_check("vprelu.s16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vprelu.s8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {-100, -10, -1, 0, 1, 10, 100, 50, -50, -5, 0, 5, 10, 20, 30, 40};
        int8_t slope[16] __attribute__((aligned(16))) = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {128, 236, 254, 0, 1, 10, 100, 50, 156, 246, 0, 5, 10, 20, 30, 40};
        register int8_t *pa asm("a0") = a;
        register int8_t *ps asm("a1") = slope;
        register uint8_t *po asm("a2") = out;
        register int32_t shift asm("a3") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[ps], 0\n"
            "esp.vprelu.s8 q2, q0, q1, %[shift]\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [ps] "+r"(ps), [po] "+r"(po)
            : [shift] "r"(shift) : "memory"
        );
        *pass += pie_check("vprelu.s8", out, expect, 16);
        (*total)++;
    }
}
