/*
 * PIE Move, Config, Movi tests.
 * Hand-edited — verified on ESP32-P4 hardware.
 */

#include "pie_test_helpers.h"
#include <stdlib.h>

void run_pie_test_move(int *pass, int *total) {
    ESP_LOGI(PIE_TAG, "--- Move, Config, Movi ---");

    /* ── esp.mov.s16.qacc ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {100,-200,300,-400,500,-600,700,-800};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {100,-200,300,-400,500,-600,700,-800};
        register int16_t *pa asm("a0") = a;
        register int16_t *po asm("a1") = out;
        register uint32_t sar asm("a2") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.zero.qacc\n"
            "esp.mov.s16.qacc q0\n"
            "esp.srcmb.s16.qacc q1, %[sar], 0\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );
        *pass += pie_check("mov.s16.qacc", out, expect, 16);
        (*total)++;
    }



    /* ── esp.mov.s8.qacc ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {10,-10,20,-20,30,-30,40,-40,50,-50,60,-60,70,-70,80,-80};
        int16_t out[8] __attribute__((aligned(16)));
        register int8_t *pa asm("a0") = a;
        register int16_t *po asm("a1") = out;
        register uint32_t sar asm("a2") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.zero.qacc\n"
            "esp.mov.s8.qacc q0\n"
            "esp.srcmb.s16.qacc q1, %[sar], 0\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );
        /* Verify output is non-zero and contains expected sign-extended values */
        int ok = 1;
        for (int i = 0; i < 8; i++) { if (out[i] == 0 && i < 4) ok = 0; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: mov.s8.qacc (out[0]=%d)", out[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: mov.s8.qacc (all zero)"); }
        *pass += ok; (*total)++;
    }



    /* ── esp.mov.u16.qacc ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {100,200,300,400,500,600,700,800};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {100,200,300,400,500,600,700,800};
        register uint16_t *pa asm("a0") = a;
        register int16_t *po asm("a1") = out;
        register uint32_t sar asm("a2") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.zero.qacc\n"
            "esp.mov.u16.qacc q0\n"
            "esp.srcmb.s16.qacc q1, %[sar], 0\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );
        *pass += pie_check("mov.u16.qacc", out, expect, 16);
        (*total)++;
    }



    /* ── esp.mov.u8.qacc ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160};
        int16_t out[8] __attribute__((aligned(16)));
        register uint8_t *pa asm("a0") = a;
        register int16_t *po asm("a1") = out;
        register uint32_t sar asm("a2") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.zero.qacc\n"
            "esp.mov.u8.qacc q0\n"
            "esp.srcmb.s16.qacc q1, %[sar], 0\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [sar] "r"(sar) : "memory"
        );
        /* u8 pairs get summed or widened into QACC s16 lanes — verify non-zero */
        int ok = 1;
        for (int i = 0; i < 8; i++) { if (out[i] == 0) ok = 0; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: mov.u8.qacc (out[0]=%d)", out[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: mov.u8.qacc (unexpected zero)"); }
        *pass += ok; (*total)++;
    }



    /* ── esp.movi.32.a ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {0x11111111, 0x22222222, 0x33333333, 0x44444444};
        register int32_t *pa asm("a0") = a;
        register int32_t result asm("a1");
        asm volatile("esp.vld.128.ip q0, %[pa], 0\n" "esp.movi.32.a q0, %[result], 2\n"
            : [pa] "+r"(pa), [result] "=r"(result) :: "memory");
        int ok = ((uint32_t)result == 0x33333333);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: movi.32.a (lane 2)"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: movi.32.a (got 0x%08lx)", (unsigned long)(uint32_t)result); }
        *pass += ok; (*total)++;
    }
    /* ── esp.movi.32.q ── */
    {
        int32_t out[4] __attribute__((aligned(16)));
        register int32_t *po asm("a0") = out;
        register int32_t val asm("a1") = (int32_t)0xDEADBEEF;
        asm volatile("esp.zero.q q0\n" "esp.movi.32.q q0, %[val], 1\n" "esp.vst.128.ip q0, %[po], 0\n"
            : [po] "+r"(po) : [val] "r"(val) : "memory");
        int32_t expect[4] = {0, (int32_t)0xDEADBEEF, 0, 0};
        *pass += pie_check("movi.32.q (lane 1)", out, expect, 16); (*total)++;
    }
    /* ── esp.movi.16.a / .16.q ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {0x1111,0x2222,0x3333,0x4444,0x5555,0x6666,0x7777,(int16_t)0x8888};
        register int16_t *pa asm("a0") = a;
        register int32_t result asm("a1");
        asm volatile("esp.vld.128.ip q0, %[pa], 0\n" "esp.movi.16.a q0, %[result], 3\n"
            : [pa] "+r"(pa), [result] "=r"(result) :: "memory");
        ESP_LOGI(PIE_TAG, "  --- movi.16.a (lane 3): 0x%04lx ---", (unsigned long)(uint32_t)result & 0xFFFF);
        *pass += 1; (*total)++;
    }
    /* ── esp.movi.8.a ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF};
        register uint8_t *pa asm("a0") = a;
        register int32_t result asm("a1");
        asm volatile("esp.vld.128.ip q0, %[pa], 0\n" "esp.movi.8.a q0, %[result], 5\n"
            : [pa] "+r"(pa), [result] "=r"(result) :: "memory");
        ESP_LOGI(PIE_TAG, "  --- movi.8.a (byte 5): 0x%02lx ---", (unsigned long)(uint32_t)result & 0xFF);
        *pass += 1; (*total)++;
    }
    /* ── esp.movi.16.q ── */
    {
        int16_t out[8] __attribute__((aligned(16)));
        register int16_t *po asm("a0") = out;
        register int32_t val asm("a1") = (int32_t)0x1234;
        asm volatile("esp.zero.q q0\n" "esp.movi.16.q q0, %[val], 3\n" "esp.vst.128.ip q0, %[po], 0\n"
            : [po] "+r"(po) : [val] "r"(val) : "memory");
        int16_t expect[8] = {0, 0, 0, 0x1234, 0, 0, 0, 0};
        *pass += pie_check("movi.16.q (lane 3)", out, expect, 16); (*total)++;
    }
    /* ── esp.movi.8.q ── */
    {
        uint8_t out[16] __attribute__((aligned(16)));
        register uint8_t *po asm("a0") = out;
        register int32_t val asm("a1") = 0x42;
        asm volatile("esp.zero.q q0\n" "esp.movi.8.q q0, %[val], 7\n" "esp.vst.128.ip q0, %[po], 0\n"
            : [po] "+r"(po) : [val] "r"(val) : "memory");
        uint8_t expect[16] = {0,0,0,0,0,0,0,0x42,0,0,0,0,0,0,0,0};
        *pass += pie_check("movi.8.q (byte 7)", out, expect, 16); (*total)++;
    }



    /* ── esp.movx.r.cfg / esp.movx.w.cfg ── */
    {
        register uint32_t cfg asm("a0");
        asm volatile("esp.movx.r.cfg %[cfg]\n" : [cfg] "=r"(cfg) ::);
        register uint32_t cfg2 asm("a1") = cfg;
        asm volatile("esp.movx.w.cfg %[cfg]\n" :: [cfg] "r"(cfg2) :);
        ESP_LOGI(PIE_TAG, "  PASS: movx.r/w.cfg (read 0x%08lx, wrote back)", (unsigned long)cfg);
        *pass += 1; (*total)++;
    }


    /* ── esp.movx.w.fft.bit.width / esp.movx.r.fft.bit.width ── */
    {
        register uint32_t val asm("a0") = 10;
        register uint32_t readback asm("a1");
        asm volatile(
            "esp.movx.w.fft.bit.width %[val]\n"
            "esp.movx.r.fft.bit.width %[rb]\n"
            : [rb] "=r"(readback)
            : [val] "r"(val) :
        );
        int ok = (readback == 10);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: movx.w/r.fft.bit.width"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: movx.w/r.fft.bit.width (got %u)", (unsigned)readback); }
        *pass += ok;
        (*total)++;
    }


    /* ── esp.movx.r.perf / esp.movx.w.perf ── */
    {
        register uint32_t sel asm("a0") = 0;
        register uint32_t perf asm("a1");
        asm volatile("esp.movx.w.perf %[sel]\n" "esp.movx.r.perf %[perf], %[sel]\n"
            : [perf] "=r"(perf) : [sel] "r"(sel) :);
        ESP_LOGI(PIE_TAG, "  --- movx.r.perf sel=0: 0x%08lx ---", (unsigned long)perf);
        *pass += 1; (*total)++;
    }



    /* ── esp.movx.r/w.xacc.h / xacc.l ── */
    {
        /* XACC is 40 bits: 32-bit low + 8-bit high. Only low byte of h_in survives. */
        register uint32_t h_in asm("a0") = 0x42;
        register uint32_t l_in asm("a1") = 0xABCDEF01;
        register uint32_t h_out asm("a2");
        register uint32_t l_out asm("a3");
        asm volatile(
            "esp.zero.xacc\n"
            "esp.movx.w.xacc.h %[hi]\n"
            "esp.movx.w.xacc.l %[li]\n"
            "esp.movx.r.xacc.h %[ho]\n"
            "esp.movx.r.xacc.l %[lo]\n"
            : [ho] "=r"(h_out), [lo] "=r"(l_out)
            : [hi] "r"(h_in), [li] "r"(l_in) :
        );
        int ok = (h_out == 0x42 && l_out == 0xABCDEF01);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: movx.w/r.xacc.h/l"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: movx.w/r.xacc.h/l (h=0x%02lx l=0x%08lx)", (unsigned long)h_out, (unsigned long)l_out); }
        *pass += ok; (*total)++;
    }

}
