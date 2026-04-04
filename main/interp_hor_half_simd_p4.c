/*
 * SIMD stub for h264bsdInterpolateHorHalf.
 * Initially calls the plain C version. Will be replaced with PIE SIMD.
 */

#include "h264_test_helpers.h"

void interp_hor_half_simd(
  u8 *ref,
  u8 *mb,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 partWidth,
  u32 partHeight)
{
    /* Stub: delegate to plain C until SIMD is implemented */
    interp_hor_half_plain(ref, mb, x0, y0, width, height, partWidth, partHeight);
}
