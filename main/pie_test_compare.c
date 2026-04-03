/*
 * PIE Comparison, Min/Max, Reduce tests.
 * Hand-edited — verified on ESP32-P4 hardware.
 */

#include "pie_test_helpers.h"
#include <stdlib.h>

void run_pie_test_compare(int *pass, int *total) {
    ESP_LOGI(PIE_TAG, "--- Comparison, Min/Max, Reduce ---");
    /* ── esp.vcmp.eq.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {10, -10, 0, 100, -100, 5, 32767, -32768};
        int16_t b[8] __attribute__((aligned(16))) = {10, 10, 0, 100, 100, 6, 32767, -32768};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {-1, 0, -1, -1, 0, 0, -1, -1};
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.eq.s16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.eq.s16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.eq.s32 ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {100, -100, 0, 0x7FFFFFFF};
        int32_t b[4] __attribute__((aligned(16))) = {100, 100, 0, 0x7FFFFFFF};
        int32_t out[4] __attribute__((aligned(16)));
        int32_t expect[4] = {-1, 0, -1, -1};
        register int32_t *pa asm("a0") = a;
        register int32_t *pb asm("a1") = b;
        register int32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.eq.s32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.eq.s32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.eq.s8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {10, -10, 0, 127, -128, 5, 5, 0, 1, 2, 3, 4, 5, 6, 7, 8};
        int8_t b[16] __attribute__((aligned(16))) = {10, 10, 0, 127, -128, 6, 5, 1, 1, 2, 3, 4, 5, 6, 7, 9};
        int8_t out[16] __attribute__((aligned(16)));
        int8_t expect[16] = {-1, 0, -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, -1, -1, 0};
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register int8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.eq.s8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.eq.s8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.eq.u16 ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {100, 0, 65535, 32768, 1, 0, 1000, 50000};
        uint16_t b[8] __attribute__((aligned(16))) = {100, 1, 65535, 32768, 0, 0, 999, 50000};
        uint16_t out[8] __attribute__((aligned(16)));
        uint16_t expect[8] = {0xFFFF, 0, 0xFFFF, 0xFFFF, 0, 0xFFFF, 0, 0xFFFF};
        register uint16_t *pa asm("a0") = a;
        register uint16_t *pb asm("a1") = b;
        register uint16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.eq.u16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.eq.u16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.eq.u32 ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {100, 0, 0xFFFFFFFF, 0x80000000};
        uint32_t b[4] __attribute__((aligned(16))) = {100, 1, 0xFFFFFFFF, 0x80000000};
        uint32_t out[4] __attribute__((aligned(16)));
        uint32_t expect[4] = {0xFFFFFFFF, 0, 0xFFFFFFFF, 0xFFFFFFFF};
        register uint32_t *pa asm("a0") = a;
        register uint32_t *pb asm("a1") = b;
        register uint32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.eq.u32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.eq.u32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.eq.u8 ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0, 1, 255, 128, 50, 50, 0, 100, 10, 20, 30, 40, 50, 60, 70, 80};
        uint8_t b[16] __attribute__((aligned(16))) = {0, 0, 255, 128, 51, 50, 1, 100, 10, 20, 30, 40, 50, 60, 70, 81};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {255, 0, 255, 255, 0, 255, 0, 255, 255, 255, 255, 255, 255, 255, 255, 0};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.eq.u8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.eq.u8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.gt.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {10, -10, 0, 100, -100, 5, 5, 0};
        int16_t b[8] __attribute__((aligned(16))) = {5, -5, -1, 50, -50, 5, 6, 0};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {(int16_t)0xFFFF, 0, (int16_t)0xFFFF, (int16_t)0xFFFF, 0, 0, 0, 0};
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.gt.s16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.gt.s16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.gt.s32 ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {10, -10, 0, 0x7FFFFFFF};
        int32_t b[4] __attribute__((aligned(16))) = {5, -5, -1, 0x7FFFFFFE};
        int32_t out[4] __attribute__((aligned(16)));
        int32_t expect[4] = {-1, 0, -1, -1};
        register int32_t *pa asm("a0") = a;
        register int32_t *pb asm("a1") = b;
        register int32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.gt.s32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.gt.s32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.gt.s8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {10, -10, 0, 127, -128, 5, 5, 0, 1, 2, 3, 4, 5, 6, 7, 8};
        int8_t b[16] __attribute__((aligned(16))) = {5, -5, -1, 126, -127, 5, 6, 0, 0, 1, 2, 3, 4, 5, 6, 8};
        int8_t out[16] __attribute__((aligned(16)));
        int8_t expect[16] = {-1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0};
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register int8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.gt.s8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.gt.s8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.gt.u16 ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {100, 0, 65535, 32768, 1, 50, 1000, 0};
        uint16_t b[8] __attribute__((aligned(16))) = {50, 0, 65534, 32767, 0, 50, 1001, 1};
        uint16_t out[8] __attribute__((aligned(16)));
        uint16_t expect[8] = {0xFFFF, 0, 0xFFFF, 0xFFFF, 0xFFFF, 0, 0, 0};
        register uint16_t *pa asm("a0") = a;
        register uint16_t *pb asm("a1") = b;
        register uint16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.gt.u16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.gt.u16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.gt.u32 ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {100, 0, 0xFFFFFFFF, 0x80000000};
        uint32_t b[4] __attribute__((aligned(16))) = {50, 0, 0xFFFFFFFE, 0x7FFFFFFF};
        uint32_t out[4] __attribute__((aligned(16)));
        uint32_t expect[4] = {0xFFFFFFFF, 0, 0xFFFFFFFF, 0xFFFFFFFF};
        register uint32_t *pa asm("a0") = a;
        register uint32_t *pb asm("a1") = b;
        register uint32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.gt.u32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.gt.u32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.gt.u8 ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {100, 0, 255, 128, 50, 50, 1, 0, 10, 20, 30, 40, 50, 60, 70, 80};
        uint8_t b[16] __attribute__((aligned(16))) = {50, 0, 254, 127, 51, 50, 0, 0, 9, 19, 29, 39, 49, 59, 69, 80};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {255, 0, 255, 255, 0, 0, 255, 0, 255, 255, 255, 255, 255, 255, 255, 0};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.gt.u8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.gt.u8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.lt.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {5, -5, -1, 50, -50, 5, 6, 0};
        int16_t b[8] __attribute__((aligned(16))) = {10, -10, 0, 100, -100, 5, 5, 0};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {(int16_t)0xFFFF, 0, (int16_t)0xFFFF, (int16_t)0xFFFF, 0, 0, 0, 0};
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.lt.s16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.lt.s16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.lt.s32 ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {5, -5, -1, (int32_t)0x80000000};
        int32_t b[4] __attribute__((aligned(16))) = {10, -10, 0, 0x7FFFFFFF};
        int32_t out[4] __attribute__((aligned(16)));
        int32_t expect[4] = {-1, 0, -1, -1};
        register int32_t *pa asm("a0") = a;
        register int32_t *pb asm("a1") = b;
        register int32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.lt.s32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.lt.s32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.lt.s8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {5, -5, -1, 126, -127, 5, 6, 0, 0, 1, 2, 3, 4, 5, 6, 8};
        int8_t b[16] __attribute__((aligned(16))) = {10, -10, 0, 127, -128, 5, 5, 0, 1, 2, 3, 4, 5, 6, 7, 8};
        int8_t out[16] __attribute__((aligned(16)));
        int8_t expect[16] = {-1, 0, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, 0};
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register int8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.lt.s8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.lt.s8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.lt.u16 ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {50, 0, 65534, 32767, 0, 50, 1001, 1};
        uint16_t b[8] __attribute__((aligned(16))) = {100, 0, 65535, 32768, 1, 50, 1000, 0};
        uint16_t out[8] __attribute__((aligned(16)));
        uint16_t expect[8] = {0xFFFF, 0, 0xFFFF, 0xFFFF, 0xFFFF, 0, 0, 0};
        register uint16_t *pa asm("a0") = a;
        register uint16_t *pb asm("a1") = b;
        register uint16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.lt.u16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.lt.u16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.lt.u32 ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {50, 0, 0xFFFFFFFE, 0x7FFFFFFF};
        uint32_t b[4] __attribute__((aligned(16))) = {100, 0, 0xFFFFFFFF, 0x80000000};
        uint32_t out[4] __attribute__((aligned(16)));
        uint32_t expect[4] = {0xFFFFFFFF, 0, 0xFFFFFFFF, 0xFFFFFFFF};
        register uint32_t *pa asm("a0") = a;
        register uint32_t *pb asm("a1") = b;
        register uint32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.lt.u32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.lt.u32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vcmp.lt.u8 ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {50, 0, 254, 127, 51, 50, 0, 0, 9, 19, 29, 39, 49, 59, 69, 80};
        uint8_t b[16] __attribute__((aligned(16))) = {100, 0, 255, 128, 50, 50, 1, 0, 10, 20, 30, 40, 50, 60, 70, 80};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {255, 0, 255, 255, 0, 0, 255, 0, 255, 255, 255, 255, 255, 255, 255, 0};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vcmp.lt.u8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vcmp.lt.u8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmax.s32 ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {-5, 10, -100, 0};
        int32_t b[4] __attribute__((aligned(16))) = {5, -10, 100, 0};
        int32_t out[4] __attribute__((aligned(16)));
        int32_t expect[4] = {5, 10, 100, 0};
        register int32_t *pa asm("a0") = a;
        register int32_t *pb asm("a1") = b;
        register int32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmax.s32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmax.s32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmin.s32 ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {-5, 10, -100, 0};
        int32_t b[4] __attribute__((aligned(16))) = {5, -10, 100, 0};
        int32_t out[4] __attribute__((aligned(16)));
        int32_t expect[4] = {-5, -10, -100, 0};
        register int32_t *pa asm("a0") = a;
        register int32_t *pb asm("a1") = b;
        register int32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmin.s32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmin.s32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmax.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {-5, 10, -100, 0, 500, -500, 32767, -32768};
        int16_t b[8] __attribute__((aligned(16))) = {5, -10, 100, 0, -500, 500, -32768, 32767};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {5, 10, 100, 0, 500, 500, 32767, 32767};
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmax.s16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmax.s16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmin.s16 ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {-5, 10, -100, 0, 500, -500, 32767, -32768};
        int16_t b[8] __attribute__((aligned(16))) = {5, -10, 100, 0, -500, 500, -32768, 32767};
        int16_t out[8] __attribute__((aligned(16)));
        int16_t expect[8] = {-5, -10, -100, 0, -500, -500, -32768, -32768};
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmin.s16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmin.s16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmax.s8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {-5, 10, -100, 0, 127, -128, 50, -50, 1, 2, 3, 4, 5, 6, 7, 8};
        int8_t b[16] __attribute__((aligned(16))) = {5, -10, 100, 0, -128, 127, -50, 50, -1, -2, -3, -4, -5, -6, -7, -8};
        int8_t out[16] __attribute__((aligned(16)));
        int8_t expect[16] = {5, 10, 100, 0, 127, 127, 50, 50, 1, 2, 3, 4, 5, 6, 7, 8};
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register int8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmax.s8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmax.s8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmin.s8 ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {-5, 10, -100, 0, 127, -128, 50, -50, 1, 2, 3, 4, 5, 6, 7, 8};
        int8_t b[16] __attribute__((aligned(16))) = {5, -10, 100, 0, -128, 127, -50, 50, -1, -2, -3, -4, -5, -6, -7, -8};
        int8_t out[16] __attribute__((aligned(16)));
        int8_t expect[16] = {-5, -10, -100, 0, -128, -128, -50, -50, -1, -2, -3, -4, -5, -6, -7, -8};
        register int8_t *pa asm("a0") = a;
        register int8_t *pb asm("a1") = b;
        register int8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmin.s8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmin.s8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmax.u8 ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0, 10, 200, 255, 50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0};
        uint8_t b[16] __attribute__((aligned(16))) = {1, 5, 100, 0, 60, 40, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {1, 10, 200, 255, 60, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmax.u8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmax.u8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmin.u8 ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {0, 10, 200, 255, 50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0};
        uint8_t b[16] __attribute__((aligned(16))) = {1, 5, 100, 0, 60, 40, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0};
        uint8_t out[16] __attribute__((aligned(16)));
        uint8_t expect[16] = {0, 5, 100, 0, 50, 40, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0};
        register uint8_t *pa asm("a0") = a;
        register uint8_t *pb asm("a1") = b;
        register uint8_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmin.u8   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmin.u8", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmax.u16 ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {0, 100, 60000, 65535, 50, 50, 32768, 0};
        uint16_t b[8] __attribute__((aligned(16))) = {1, 50, 10000, 0, 60, 40, 32767, 1};
        uint16_t out[8] __attribute__((aligned(16)));
        uint16_t expect[8] = {1, 100, 60000, 65535, 60, 50, 32768, 1};
        register uint16_t *pa asm("a0") = a;
        register uint16_t *pb asm("a1") = b;
        register uint16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmax.u16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmax.u16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmin.u16 ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {0, 100, 60000, 65535, 50, 50, 32768, 0};
        uint16_t b[8] __attribute__((aligned(16))) = {1, 50, 10000, 0, 60, 40, 32767, 1};
        uint16_t out[8] __attribute__((aligned(16)));
        uint16_t expect[8] = {0, 50, 10000, 0, 50, 40, 32767, 0};
        register uint16_t *pa asm("a0") = a;
        register uint16_t *pb asm("a1") = b;
        register uint16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmin.u16   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmin.u16", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmax.u32 ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {0, 100, 0xFFFFFFFF, 0x80000000};
        uint32_t b[4] __attribute__((aligned(16))) = {1, 50, 0xFFFFFFFE, 0x80000001};
        uint32_t out[4] __attribute__((aligned(16)));
        uint32_t expect[4] = {1, 100, 0xFFFFFFFF, 0x80000001};
        register uint32_t *pa asm("a0") = a;
        register uint32_t *pb asm("a1") = b;
        register uint32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmax.u32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmax.u32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.vmin.u32 ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {0, 100, 0xFFFFFFFF, 0x80000000};
        uint32_t b[4] __attribute__((aligned(16))) = {1, 50, 0xFFFFFFFE, 0x80000001};
        uint32_t out[4] __attribute__((aligned(16)));
        uint32_t expect[4] = {0, 50, 0xFFFFFFFE, 0x80000000};
        register uint32_t *pa asm("a0") = a;
        register uint32_t *pb asm("a1") = b;
        register uint32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vmin.u32   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("vmin.u32", out, expect, 16);
        (*total)++;
    }

    /* ── esp.max.s32.a ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {-100, 500, 200, -50};
        register int32_t *pa asm("a0") = a;
        register int32_t result asm("a1") = (int32_t)0x80000000;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.max.s32.a  q0, %[result]\n"
            : [pa] "+r"(pa), [result] "+r"(result) :: "memory"
        );
        int ok = ((int32_t)result == (int32_t)500);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: max.s32.a"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: max.s32.a (got %d, expect 500)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.min.s32.a ── */
    {
        int32_t a[4] __attribute__((aligned(16))) = {-100, 500, 200, -50};
        register int32_t *pa asm("a0") = a;
        register int32_t result asm("a1") = 0x7FFFFFFF;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.min.s32.a  q0, %[result]\n"
            : [pa] "+r"(pa), [result] "+r"(result) :: "memory"
        );
        int ok = ((int32_t)result == (int32_t)-100);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: min.s32.a"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: min.s32.a (got %d, expect -100)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.max.s16.a ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {-100, 500, 200, -50, 32767, -32768, 0, 1};
        register int16_t *pa asm("a0") = a;
        register int32_t result asm("a1") = (int32_t)0x80000000;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.max.s16.a  q0, %[result]\n"
            : [pa] "+r"(pa), [result] "+r"(result) :: "memory"
        );
        int ok = ((int32_t)result == (int32_t)32767);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: max.s16.a"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: max.s16.a (got %d, expect 32767)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.min.s16.a ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {-100, 500, 200, -50, 32767, -32768, 0, 1};
        register int16_t *pa asm("a0") = a;
        register int32_t result asm("a1") = 0x7FFFFFFF;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.min.s16.a  q0, %[result]\n"
            : [pa] "+r"(pa), [result] "+r"(result) :: "memory"
        );
        int ok = ((int32_t)result == (int32_t)-32768);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: min.s16.a"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: min.s16.a (got %d, expect -32768)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.max.s8.a ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {10, -50, 127, -128, 0, 1, 100, -100, 50, 60, 70, 80, 90, 64, 32, 1};
        register int8_t *pa asm("a0") = a;
        register int32_t result asm("a1") = (int32_t)0x80000000;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.max.s8.a  q0, %[result]\n"
            : [pa] "+r"(pa), [result] "+r"(result) :: "memory"
        );
        int ok = ((int32_t)result == (int32_t)127);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: max.s8.a"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: max.s8.a (got %d, expect 127)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.min.s8.a ── */
    {
        int8_t a[16] __attribute__((aligned(16))) = {10, -50, 127, -128, 0, 1, 100, -100, 50, 60, 70, 80, 90, 64, 32, 1};
        register int8_t *pa asm("a0") = a;
        register int32_t result asm("a1") = 0x7FFFFFFF;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.min.s8.a  q0, %[result]\n"
            : [pa] "+r"(pa), [result] "+r"(result) :: "memory"
        );
        int ok = ((int32_t)result == (int32_t)-128);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: min.s8.a"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: min.s8.a (got %d, expect -128)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.max.u32.a ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {100, 500, 0xFFFFFFFF, 0};
        register uint32_t *pa asm("a0") = a;
        register uint32_t result asm("a1") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.max.u32.a  q0, %[result]\n"
            : [pa] "+r"(pa), [result] "+r"(result) :: "memory"
        );
        int ok = ((uint32_t)result == (uint32_t)4294967295);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: max.u32.a"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: max.u32.a (got %d, expect 4294967295)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.min.u32.a ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {100, 500, 0xFFFFFFFF, 0};
        register uint32_t *pa asm("a0") = a;
        register uint32_t result asm("a1") = 0xFFFFFFFF;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.min.u32.a  q0, %[result]\n"
            : [pa] "+r"(pa), [result] "+r"(result) :: "memory"
        );
        int ok = ((uint32_t)result == (uint32_t)0);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: min.u32.a"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: min.u32.a (got %d, expect 0)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.max.u16.a ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {100, 500, 65535, 0, 32768, 1, 1000, 50000};
        register uint16_t *pa asm("a0") = a;
        register uint32_t result asm("a1") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.max.u16.a  q0, %[result]\n"
            : [pa] "+r"(pa), [result] "+r"(result) :: "memory"
        );
        int ok = ((uint32_t)result == (uint32_t)65535);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: max.u16.a"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: max.u16.a (got %d, expect 65535)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.min.u16.a ── */
    {
        uint16_t a[8] __attribute__((aligned(16))) = {100, 500, 65535, 0, 32768, 1, 1000, 50000};
        register uint16_t *pa asm("a0") = a;
        register uint32_t result asm("a1") = 0xFFFFFFFF;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.min.u16.a  q0, %[result]\n"
            : [pa] "+r"(pa), [result] "+r"(result) :: "memory"
        );
        int ok = ((uint32_t)result == (uint32_t)0);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: min.u16.a"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: min.u16.a (got %d, expect 0)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.max.u8.a ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {10, 200, 50, 0, 255, 128, 64, 32, 1, 2, 3, 4, 5, 6, 7, 8};
        register uint8_t *pa asm("a0") = a;
        register uint32_t result asm("a1") = 0;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.max.u8.a  q0, %[result]\n"
            : [pa] "+r"(pa), [result] "+r"(result) :: "memory"
        );
        int ok = ((uint32_t)result == (uint32_t)255);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: max.u8.a"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: max.u8.a (got %d, expect 255)", (int)result); }
        *pass += ok;
        (*total)++;
    }

    /* ── esp.min.u8.a ── */
    {
        uint8_t a[16] __attribute__((aligned(16))) = {10, 200, 50, 0, 255, 128, 64, 32, 1, 2, 3, 4, 5, 6, 7, 8};
        register uint8_t *pa asm("a0") = a;
        register uint32_t result asm("a1") = 0xFFFFFFFF;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.min.u8.a  q0, %[result]\n"
            : [pa] "+r"(pa), [result] "+r"(result) :: "memory"
        );
        int ok = ((uint32_t)result == (uint32_t)0);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: min.u8.a"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: min.u8.a (got %d, expect 0)", (int)result); }
        *pass += ok;
        (*total)++;
    }
}
