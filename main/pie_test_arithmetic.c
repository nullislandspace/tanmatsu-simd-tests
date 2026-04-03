/*
 * PIE Arithmetic tests.
 * Hand-edited — verified on ESP32-P4 hardware.
 */

#include "pie_test_helpers.h"
#include <stdlib.h>

void run_pie_test_arithmetic(int *pass, int *total) {
    ESP_LOGI(PIE_TAG, "--- Arithmetic ---");
    /* ── esp.vadd.s32 ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {100, 0x7FFFFFFF, (int32_t)0x80000000, 0x7FFFFFF0};
        int32_t b[4] __attribute__((aligned(16))) = {50, 1, -1, 100};
        int32_t out[4] __attribute__((aligned(16)));
        int32_t expect[4] = {150, 2147483647, (int32_t)0x80000000, 2147483647};
        register int32_t *pa asm("a0") = a;
        register int32_t *pb asm("a1") = b;
        register int32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vadd.s32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vadd.s32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vadd.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {100, -100, 32000, 32767, -32000, -32768, 0, 1};
        int16_t b[8] __attribute__((aligned(16))) = {50, -50, 1000, 1, -1000, -1, 0, -1};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {150, -150, 32767, 32767, (int16_t)0x8000, (int16_t)0x8000, 0, 0};
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vadd.s16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vadd.s16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vadd.s8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {10, -10, 120, 127, -120, -128, 0, 1, 50, -50, 100, -100, 64, -64, 1, -1};
        int8_t b[16] __attribute__((aligned(16))) = {5, -5, 20, 1, -20, -1, 0, -1, 50, -50, 50, -50, 64, -64, -1, 1};
        int8_t out[16] __attribute__((aligned(16)));
        int8_t expect[16] = {15, -15, 127, 127, (int8_t)0x80, (int8_t)0x80, 0, 0, 100, -100, 127, (int8_t)0x80, 127, (int8_t)0x80, 0, 0};
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register int8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vadd.s8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vadd.s8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vadd.u8 ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {1, 100, 200, 255, 255, 255, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50};
        uint8_t b[16] __attribute__((aligned(16))) = {1, 50, 100, 1, 255, 0, 0, 1, 50, 50, 50, 50, 50, 50, 50, 50};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {2, 150, 255, 255, 255, 255, 0, 1, 100, 100, 100, 100, 100, 100, 100, 100};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vadd.u8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vadd.u8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vadd.u16 ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {100, 60000, 65535, 65535, 0, 1, 32768, 50000};
        uint16_t b[8] __attribute__((aligned(16))) = {50, 10000, 1, 65535, 0, 65535, 32768, 20000};
        uint16_t out[8] __attribute__((aligned(16)));
        uint16_t expect[8] = {150, 65535, 65535, 65535, 0, 65535, 65535, 65535};
        register uint16_t *pa asm("a0") = a;
        register uint16_t *pb asm("a1") = b;
        register uint16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vadd.u16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vadd.u16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vadd.u32 ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {100, 0xFFFFFFFF, 0xFFFFFFF0, 0};
        uint32_t b[4] __attribute__((aligned(16))) = {50, 1, 100, 0};
        uint32_t out[4] __attribute__((aligned(16)));
        uint32_t expect[4] = {150, 0xFFFFFFFF, 0xFFFFFFFF, 0};
        register uint32_t *pa asm("a0") = a;
        register uint32_t *pb asm("a1") = b;
        register uint32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vadd.u32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vadd.u32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsub.s32 ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {100, (int32_t)0x80000000, 0x7FFFFFFF, (int32_t)0x80000010};
        int32_t b[4] __attribute__((aligned(16))) = {50, 1, -1, 100};
        int32_t out[4] __attribute__((aligned(16)));
        int32_t expect[4] = {50, (int32_t)0x80000000, 2147483647, (int32_t)0x80000000};
        register int32_t *pa asm("a0") = a;
        register int32_t *pb asm("a1") = b;
        register int32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vsub.s32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vsub.s32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsub.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {100, -100, -32000, -32768, 32000, 32767, 0, -1};
        int16_t b[8] __attribute__((aligned(16))) = {50, -50, 1000, 1, -1000, -1, 0, -1};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {50, -50, (int16_t)0x8000, (int16_t)0x8000, 32767, 32767, 0, 0};
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vsub.s16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vsub.s16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsub.s8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {10, -10, -120, -128, 120, 127, 0, -1, 50, -50, 100, -100, 64, -64, 1, -1};
        int8_t b[16] __attribute__((aligned(16))) = {5, -5, 20, 1, -20, -1, 0, -1, 50, -50, -50, 50, -64, 64, -1, 1};
        int8_t out[16] __attribute__((aligned(16)));
        int8_t expect[16] = {5, -5, (int8_t)0x80, (int8_t)0x80, 127, 127, 0, 0, 0, 0, 127, (int8_t)0x80, 127, (int8_t)0x80, 2, -2};
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register int8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vsub.s8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vsub.s8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsub.u8 ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {100, 50, 10, 0, 0, 0, 255, 255, 200, 200, 200, 200, 200, 200, 200, 200};
        uint8_t b[16] __attribute__((aligned(16))) = {50, 50, 100, 1, 255, 0, 0, 1, 100, 100, 100, 100, 100, 100, 100, 100};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {50, 0, 0, 0, 0, 0, 255, 254, 100, 100, 100, 100, 100, 100, 100, 100};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vsub.u8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vsub.u8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsub.u16 ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {100, 50, 0, 0, 65535, 65535, 32768, 1};
        uint16_t b[8] __attribute__((aligned(16))) = {50, 50, 1, 65535, 0, 1, 32768, 0};
        uint16_t out[8] __attribute__((aligned(16)));
        uint16_t expect[8] = {50, 0, 0, 0, 65535, 65534, 0, 1};
        register uint16_t *pa asm("a0") = a;
        register uint16_t *pb asm("a1") = b;
        register uint16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vsub.u16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vsub.u16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsub.u32 ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {100, 0, 0xFFFFFFFF, 50};
        uint32_t b[4] __attribute__((aligned(16))) = {50, 1, 0, 50};
        uint32_t out[4] __attribute__((aligned(16)));
        uint32_t expect[4] = {50, 0, 0xFFFFFFFF, 0};
        register uint32_t *pa asm("a0") = a;
        register uint32_t *pb asm("a1") = b;
        register uint32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vsub.u32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vsub.u32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsadds.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {100, -100, 32000, -32000, 32767, -32768, 0, 1};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {110, -90, 32010, -31990, 32767, -32758, 10, 11};
        register int16_t *pa asm("a0") = a;
        register int16_t *po asm("a1") = out;
        register int32_t scalar asm("a2") = 10;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vsadds.s16 q1, q0, %[scalar]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [scalar] "r"(scalar) : "memory"
        );
        *pass += pie_check("vsadds.s16 (+10)", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsadds.s8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {10, -10, 120, -120, 127, -128, 0, 1, 50, -50, 100, -100, 64, -64, -1, 0};
        int8_t out[16] __attribute__((aligned(16)));
        int8_t expect[16] = {15, -5, 125, -115, 127, -123, 5, 6, 55, -45, 105, -95, 69, -59, 4, 5};
        register int8_t *pa asm("a0") = a;
        register int8_t *po asm("a1") = out;
        register int32_t scalar asm("a2") = 5;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vsadds.s8 q1, q0, %[scalar]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [scalar] "r"(scalar) : "memory"
        );
        *pass += pie_check("vsadds.s8 (+5)", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsadds.u16 ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {100, 0, 65530, 65535, 32768, 1000, 0, 50000};
        uint16_t out[8] __attribute__((aligned(16)));
        uint16_t expect[8] = {120, 20, 65535, 65535, 32788, 1020, 20, 50020};
        register uint16_t *pa asm("a0") = a;
        register uint16_t *po asm("a1") = out;
        register uint32_t scalar asm("a2") = 20;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vsadds.u16 q1, q0, %[scalar]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [scalar] "r"(scalar) : "memory"
        );
        *pass += pie_check("vsadds.u16 (+20)", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vsadds.u8 ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0, 10, 100, 200, 250, 255, 128, 1, 50, 50, 50, 50, 50, 50, 50, 50};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {10, 20, 110, 210, 255, 255, 138, 11, 60, 60, 60, 60, 60, 60, 60, 60};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *po asm("a1") = out;
        register uint32_t scalar asm("a2") = 10;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vsadds.u8 q1, q0, %[scalar]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [scalar] "r"(scalar) : "memory"
        );
        *pass += pie_check("vsadds.u8 (+10)", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vssubs.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {100, -100, 32000, -32000, 32767, -32768, 0, 10};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {90, -110, 31990, -32010, 32757, (int16_t)0x8000, -10, 0};
        register int16_t *pa asm("a0") = a;
        register int16_t *po asm("a1") = out;
        register int32_t scalar asm("a2") = 10;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vssubs.s16 q1, q0, %[scalar]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [scalar] "r"(scalar) : "memory"
        );
        *pass += pie_check("vssubs.s16 (-10)", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vssubs.s8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {10, -10, 120, -120, 127, -128, 5, 0, 50, -50, 100, -100, 64, -64, -1, 1};
        int8_t out[16] __attribute__((aligned(16)));
        int8_t expect[16] = {5, -15, 115, -125, 122, (int8_t)0x80, 0, -5, 45, -55, 95, -105, 59, -69, -6, -4};
        register int8_t *pa asm("a0") = a;
        register int8_t *po asm("a1") = out;
        register int32_t scalar asm("a2") = 5;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vssubs.s8 q1, q0, %[scalar]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [scalar] "r"(scalar) : "memory"
        );
        *pass += pie_check("vssubs.s8 (-5)", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vssubs.u16 ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {100, 20, 0, 5, 65535, 32768, 1000, 50};
        uint16_t out[8] __attribute__((aligned(16)));
        uint16_t expect[8] = {80, 0, 0, 0, 65515, 32748, 980, 30};
        register uint16_t *pa asm("a0") = a;
        register uint16_t *po asm("a1") = out;
        register uint32_t scalar asm("a2") = 20;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vssubs.u16 q1, q0, %[scalar]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [scalar] "r"(scalar) : "memory"
        );
        *pass += pie_check("vssubs.u16 (-20)", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vssubs.u8 ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {100, 50, 10, 0, 5, 255, 128, 20, 60, 60, 60, 60, 60, 60, 60, 60};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {90, 40, 0, 0, 0, 245, 118, 10, 50, 50, 50, 50, 50, 50, 50, 50};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *po asm("a1") = out;
        register uint32_t scalar asm("a2") = 10;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vssubs.u8 q1, q0, %[scalar]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po)
            : [scalar] "r"(scalar) : "memory"
        );
        *pass += pie_check("vssubs.u8 (-10)", out, expect, 16);
        (*total)++;
    }

    /* ── esp.addx2 ── */
    {
        register int32_t rd asm("a0");
        register int32_t rs1 asm("a1") = 100;
        register int32_t rs2 asm("a2") = 25;
        asm volatile(
            "esp.addx2 %[rd], %[rs1], %[rs2]\n"
            : [rd] "=r"(rd)
            : [rs1] "r"(rs1), [rs2] "r"(rs2) :
        );
        int ok = (rd == 150);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: addx2"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: addx2 (got %d, expect 150)", (int)rd); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.addx4 ── */
    {
        register int32_t rd asm("a0");
        register int32_t rs1 asm("a1") = 100;
        register int32_t rs2 asm("a2") = 25;
        asm volatile(
            "esp.addx4 %[rd], %[rs1], %[rs2]\n"
            : [rd] "=r"(rd)
            : [rs1] "r"(rs1), [rs2] "r"(rs2) :
        );
        int ok = (rd == 200);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: addx4"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: addx4 (got %d, expect 200)", (int)rd); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.subx2 ── */
    {
        register int32_t rd asm("a0");
        register int32_t rs1 asm("a1") = 100;
        register int32_t rs2 asm("a2") = 25;
        asm volatile(
            "esp.subx2 %[rd], %[rs1], %[rs2]\n"
            : [rd] "=r"(rd)
            : [rs1] "r"(rs1), [rs2] "r"(rs2) :
        );
        int ok = (rd == 50);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: subx2"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: subx2 (got %d, expect 50)", (int)rd); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.subx4 ── */
    {
        register int32_t rd asm("a0");
        register int32_t rs1 asm("a1") = 200;
        register int32_t rs2 asm("a2") = 25;
        asm volatile(
            "esp.subx4 %[rd], %[rs1], %[rs2]\n"
            : [rd] "=r"(rd)
            : [rs1] "r"(rs1), [rs2] "r"(rs2) :
        );
        int ok = (rd == 100);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: subx4"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: subx4 (got %d, expect 100)", (int)rd); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.sat ── */
    {
        register int32_t rd asm("a0");
        register int32_t rs1 asm("a1") = 300;
        register int32_t rs2 asm("a2") = 8;
        asm volatile(
            "esp.sat %[rd], %[rs1], %[rs2]\n"
            : [rd] "=r"(rd)
            : [rs1] "r"(rs1), [rs2] "r"(rs2) :
        );
        int ok = (rd == 34);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: sat"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: sat (got %d, expect 34)", (int)rd); }
        *pass += ok;
        (*total)++;
    }
}
