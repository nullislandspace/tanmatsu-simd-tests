/*
 * SIMD test runner: correctness and performance benchmarks
 * for H.264 decoder functions (C reference vs PIE SIMD).
 */

#pragma once

#include "pax_gfx.h"

/* Run all tests: PIE instruction verification, then H.264 function tests.
 * Results displayed on screen and logged via ESP_LOGI.
 * Does not return — enters idle loop after tests complete. */
void run_all_tests(pax_buf_t *fb, void (*blit)(void));
