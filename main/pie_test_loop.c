/*
 * PIE Hardware Loop tests.
 * Hand-edited — verified on ESP32-P4 hardware.
 */

#include "pie_test_helpers.h"
#include <stdlib.h>

void run_pie_test_loop(int *pass, int *total) {
    ESP_LOGI(PIE_TAG, "--- Hardware Loop ---");
    /* ── lp.counti ── */
    {
        asm volatile(
            "esp.lp.counti 0, 10\n"
            :::
        );
        ESP_LOGI(PIE_TAG, "  PASS: lp.counti (compiled and executed)");
        *pass += 1;
        (*total)++;
    }


    /* ── esp.lp.count ── */
    {
        register int32_t lc asm("a0") = 5;
        asm volatile("esp.lp.count 0, %[lc]\n" :: [lc] "r"(lc) :);
        ESP_LOGI(PIE_TAG, "  PASS: lp.count (compiled and executed)");
        *pass += 1; (*total)++;
    }


    /* ── lp.starti + lp.endi ── */
    {
        asm volatile(
            "esp.lp.starti 0, 0\n" "esp.lp.endi 0, 0\n"
            :::
        );
        ESP_LOGI(PIE_TAG, "  PASS: lp.starti + lp.endi (compiled and executed)");
        *pass += 1;
        (*total)++;
    }

    /* ── lp.setupi ── */
    {
        asm volatile(
            "esp.lp.setupi 0, 4, 4\n" "nop\n"
            :::
        );
        ESP_LOGI(PIE_TAG, "  PASS: lp.setupi (compiled and executed)");
        *pass += 1;
        (*total)++;
    }


    /* ── esp.lp.setup ── */
    {
        register int32_t lc asm("a0") = 4;
        asm volatile("esp.lp.setup 0, %[lc], 4\n" "nop\n" :: [lc] "r"(lc) :);
        ESP_LOGI(PIE_TAG, "  PASS: lp.setup (compiled and executed)");
        *pass += 1; (*total)++;
    }

}
