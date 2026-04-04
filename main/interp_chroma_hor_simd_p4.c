/*
 * h264bsdInterpolateChromaHor — kept as plain C.
 *
 * Analysis: PIE SIMD is NOT well-suited to this function.
 *
 * The bilinear chroma interpolation has:
 *   - Small working set: max 8 pixels per row (one i16 register after widen)
 *   - Overlapping horizontal reads (A, B are adjacent pixels) — same
 *     shifted-view problem as luma interpolation
 *   - Requires multiply: val*A + xFrac*B per pixel. esp.vmul.s16 has an
 *     unwanted >>4 shift, so must use QACC path (vmulas + srcmb)
 *   - 2x2 block inner loop interleaves current and next row reads
 *   - Only 4-6 scalar ops per pixel — not enough compute to amortize
 *     SIMD widen/narrow/QACC overhead
 *
 * Based on luma interpolation findings (1.02x best), SIMD optimization
 * of this function is not expected to provide meaningful speedup.
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
    interp_chroma_hor_plain(pRef, predPartChroma, x0, y0, width, height,
            xFrac, chromaPartWidth, chromaPartHeight);
}
