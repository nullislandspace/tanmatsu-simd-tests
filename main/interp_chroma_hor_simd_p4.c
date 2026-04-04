/*
 * h264bsdInterpolateChromaHor — kept as plain C (scalar).
 *
 * Bilinear horizontal chroma interpolation:
 *   out = (val * A + xFrac * B + 4) >> 3, where val = 8 - xFrac
 *
 * Not viable for SIMD optimization on ESP32-P4 PIE:
 *   - Same horizontal sliding window problem as luma (A and B at offsets 0,1)
 *   - No register-only byte shift (confirmed exhaustively in Step 4)
 *   - Only 2 taps (vs luma's 6) — even less compute to amortize load overhead
 *   - Small blocks: max 8×8 pixels per component
 *   - Trivial per-pixel cost: 2 muls + 1 add + 1 shift = 4 ops
 *   - esp.vmul.s16 has unwanted >>4, would need QACC multiply path
 *   - The 2×2 interleaved row processing in the original is already
 *     very cache-friendly for the scalar access pattern
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
