/*
 * PIE SIMD instruction verification tests.
 * Tests individual ESP32-P4 PIE instructions with known input/output
 * to verify they compile, execute, and produce correct results.
 */

#pragma once

#include "pax_gfx.h"

/* Run PIE instruction tests, report results on screen and via ESP_LOGI.
 * line: current display line number (updated on return). */
void run_pie_instruction_tests(pax_buf_t *fb, void (*blit)(void), int *line);
