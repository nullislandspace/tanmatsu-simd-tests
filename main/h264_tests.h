/*
 * H264 SIMD function test orchestrator header.
 */

#pragma once

#include "test_runner.h"

void run_h264_tests(pax_buf_t *fb, void (*blit)(void), int *line);
