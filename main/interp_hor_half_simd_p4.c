/*
 * h264bsdInterpolateHorHalf — kept as plain C (scalar).
 *
 * Horizontal 6-tap FIR filter: out = clp[(A-5B+20C+20D-5E+F+16)>>5]
 *
 * Extensively tested for SIMD optimization — not viable on ESP32-P4 PIE.
 *
 * The fundamental problem: each output pixel reads 6 horizontally adjacent
 * input bytes. Processing 8 outputs needs 6 "shifted views" of the same
 * data (at byte offsets 0-5). PIE has NO register-only byte shift:
 *
 *   - esp.src.q ignores movx.w.sar — only uses usar internal alignment
 *   - esp.srci.2q / srcxxp.2q zero-fill on right shift (ignores qs register)
 *   - esp.slci.2q fills from wrong direction (qs tail, not qs head)
 *   - esp.vsld.8 / vsrd.8 are bit-level shifts within lanes, not byte slides
 *
 * Each shifted view requires a 3-instruction usar load from memory.
 * 6 views × 3 insns = 18 load insns to feed 11 compute insns.
 * The scalar sliding-window code loads each byte exactly once (~13 ops/pixel
 * with perfect register reuse), which SIMD cannot beat.
 *
 * Versions tested (16×16 block):
 *   v1 (1.06x): widen to i16 scratch, 6 usar from i16 buffer
 *   v2 (1.02x): 6 usar direct from u8 source, widen each
 *   v3 (0.77x): widen all rows upfront, usar from widened buffer
 *   v4 (FAIL):  src.q + movx.w.sar — doesn't work (src.q ignores SAR)
 *   v5 (1.07x): per-row i16 scratch, usar from scratch
 *   v6 (1.03x): aligned u8 scratch, process 16px in two 8px halves
 *
 * Confirmed by internet research: this is a known architectural limitation
 * shared with ESP32-S3. Espressif's own implementations use scalar or
 * accept the usar overhead for horizontal filters.
 */

#include "h264_test_helpers.h"
#include <string.h>

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
    u32 p1[21*21/4+1];
    u8 *ptrJ;
    u32 x, y;
    i32 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    const u8 *clp = TEST_CLP;

    if ((x0 < 0) || ((u32)x0+partWidth+5 > width) ||
        (y0 < 0) || ((u32)y0+partHeight > height))
    {
        h264bsdFillBlock_test(ref, (u8*)p1, x0, y0, width, height,
                partWidth+5, partHeight, partWidth+5);
        x0 = 0;
        y0 = 0;
        ref = (u8*)p1;
        width = partWidth + 5;
    }

    ref += (u32)y0 * width + (u32)x0;
    ptrJ = ref + 5;

    for (y = partHeight; y; y--)
    {
        tmp6 = *(ptrJ - 5);
        tmp5 = *(ptrJ - 4);
        tmp4 = *(ptrJ - 3);
        tmp3 = *(ptrJ - 2);
        tmp2 = *(ptrJ - 1);

        for (x = (partWidth >> 2); x; x--)
        {
            tmp6 += 16;
            tmp7 = tmp3 + tmp4;
            tmp6 += (tmp7 << 4);
            tmp6 += (tmp7 << 2);
            tmp7 = tmp2 + tmp5;
            tmp1 = *ptrJ++;
            tmp6 -= (tmp7 << 2);
            tmp6 -= tmp7;
            tmp6 += tmp1;
            tmp6 = clp[tmp6>>5];

            tmp5 += 16;
            tmp7 = tmp2 + tmp3;
            *mb++ = (u8)tmp6;
            tmp5 += (tmp7 << 4);
            tmp5 += (tmp7 << 2);
            tmp7 = tmp1 + tmp4;
            tmp6 = *ptrJ++;
            tmp5 -= (tmp7 << 2);
            tmp5 -= tmp7;
            tmp5 += tmp6;
            tmp5 = clp[tmp5>>5];

            tmp4 += 16;
            tmp7 = tmp1 + tmp2;
            *mb++ = (u8)tmp5;
            tmp4 += (tmp7 << 4);
            tmp4 += (tmp7 << 2);
            tmp7 = tmp6 + tmp3;
            tmp5 = *ptrJ++;
            tmp4 -= (tmp7 << 2);
            tmp4 -= tmp7;
            tmp4 += tmp5;
            tmp4 = clp[tmp4>>5];

            tmp3 += 16;
            tmp7 = tmp6 + tmp1;
            *mb++ = (u8)tmp4;
            tmp3 += (tmp7 << 4);
            tmp3 += (tmp7 << 2);
            tmp7 = tmp5 + tmp2;
            tmp4 = *ptrJ++;
            tmp3 -= (tmp7 << 2);
            tmp3 -= tmp7;
            tmp3 += tmp4;
            tmp3 = clp[tmp3>>5];
            tmp7 = tmp4;
            tmp4 = tmp6;
            tmp6 = tmp2;
            tmp2 = tmp7;
            *mb++ = (u8)tmp3;
            tmp3 = tmp5;
            tmp5 = tmp1;
        }
        ptrJ += width - partWidth;
        mb += 16 - partWidth;
    }
}
