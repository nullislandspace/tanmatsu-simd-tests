/*
 * SIMD stub for h264bsdInterpolateChromaHor.
 * Initially calls the plain C version. Will be replaced with PIE SIMD.
 */

#include "h264_test_helpers.h"

void interp_chroma_hor_simd(
  u8 *pRef,
  u8 *predPartChroma,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 xFrac,
  u32 chromaPartWidth,
  u32 chromaPartHeight)
{
    /* Stub: delegate to plain C until SIMD is implemented */
    interp_chroma_hor_plain(pRef, predPartChroma, x0, y0, width, height,
            xFrac, chromaPartWidth, chromaPartHeight);
}
