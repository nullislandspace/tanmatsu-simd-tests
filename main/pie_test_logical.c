/*
 * PIE Bitwise Logical tests.
 * Hand-edited — verified on ESP32-P4 hardware.
 */

#include "pie_test_helpers.h"
#include <stdlib.h>

void run_pie_test_logical(int *pass, int *total) {
    ESP_LOGI(PIE_TAG, "--- Bitwise Logical ---");
    /* ── esp.andq ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {0xFFFF0000, 0x12345678, 0xAAAAAAAA, 0x00000000};
        uint32_t b[4] __attribute__((aligned(16))) = {0xFF00FF00, 0x0F0F0F0F, 0x55555555, 0xFFFFFFFF};
        uint32_t out[4] __attribute__((aligned(16)));
        uint32_t expect[4] = {0xFF000000, 0x02040608, 0x00000000, 0x00000000};
        register uint32_t *pa asm("a0") = a;
        register uint32_t *pb asm("a1") = b;
        register uint32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.andq   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("andq", out, expect, 16);
        (*total)++;
    }

    /* ── esp.orq ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF};
        uint32_t b[4] __attribute__((aligned(16))) = {0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000};
        uint32_t out[4] __attribute__((aligned(16)));
        uint32_t expect[4] = {0xFFFF0000, 0x00FFFF00, 0x0000FFFF, 0xFF0000FF};
        register uint32_t *pa asm("a0") = a;
        register uint32_t *pb asm("a1") = b;
        register uint32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.orq   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("orq", out, expect, 16);
        (*total)++;
    }

    /* ── esp.xorq ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {0xFF00FF00, 0x12345678, 0xAAAAAAAA, 0xFFFFFFFF};
        uint32_t b[4] __attribute__((aligned(16))) = {0xFF00FF00, 0x12345678, 0x55555555, 0x00000000};
        uint32_t out[4] __attribute__((aligned(16)));
        uint32_t expect[4] = {0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF};
        register uint32_t *pa asm("a0") = a;
        register uint32_t *pb asm("a1") = b;
        register uint32_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.xorq   q2, q0, q1\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("xorq", out, expect, 16);
        (*total)++;
    }

    /* ── esp.notq ── */
    {
        uint32_t a[4] __attribute__((aligned(16))) = {0x00000000, 0xFFFFFFFF, 0xAAAAAAAA, 0x12345678};
        uint32_t out[4] __attribute__((aligned(16)));
        uint32_t expect[4] = {0xFFFFFFFF, 0x00000000, 0x55555555, 0xEDCBA987};
        register uint32_t *pa asm("a0") = a;
        register uint32_t *po asm("a1") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.notq    q1, q0\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po) :: "memory"
        );
        *pass += pie_check("notq", out, expect, 16);
        (*total)++;
    }
}
