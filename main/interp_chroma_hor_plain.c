/*
 * Plain C implementation of h264bsdInterpolateChromaHor.
 *
 * Extracted from h264bsd_reconstruct.c. Implements horizontal bilinear
 * chroma interpolation for H.264 motion compensation.
 */

#include "h264_test_helpers.h"

void interp_chroma_hor_plain(
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
    u32 x, y, tmp1, tmp2, tmp3, tmp4, c, val;
    u8 *ptrA, *cbr;
    u32 comp;
    u8 block[9*8*2];

    if ((x0 < 0) || ((u32)x0+chromaPartWidth+1 > width) ||
        (y0 < 0) || ((u32)y0+chromaPartHeight > height))
    {
        h264bsdFillBlock_test(pRef, block, x0, y0, width, height,
            chromaPartWidth + 1, chromaPartHeight, chromaPartWidth + 1);
        pRef += width * height;
        h264bsdFillBlock_test(pRef, block + (chromaPartWidth+1)*chromaPartHeight,
            x0, y0, width, height, chromaPartWidth + 1,
            chromaPartHeight, chromaPartWidth + 1);

        pRef = block;
        x0 = 0;
        y0 = 0;
        width = chromaPartWidth+1;
        height = chromaPartHeight;
    }

    val = 8 - xFrac;

    for (comp = 0; comp <= 1; comp++)
    {
        ptrA = pRef + (comp * height + (u32)y0) * width + x0;
        cbr = predPartChroma + comp * 8 * 8;

        /* 2x2 pels per iteration
         * bilinear horizontal interpolation */
        for (y = (chromaPartHeight >> 1); y; y--)
        {
            for (x = (chromaPartWidth >> 1); x; x--)
            {
                tmp1 = ptrA[width];
                tmp2 = *ptrA++;
                tmp3 = ptrA[width];
                tmp4 = *ptrA++;
                c = ((val * tmp1 + xFrac * tmp3) << 3) + 32;
                c >>= 6;
                cbr[8] = (u8)c;
                c = ((val * tmp2 + xFrac * tmp4) << 3) + 32;
                c >>= 6;
                *cbr++ = (u8)c;
                tmp1 = ptrA[width];
                tmp2 = *ptrA;
                c = ((val * tmp3 + xFrac * tmp1) << 3) + 32;
                c >>= 6;
                cbr[8] = (u8)c;
                c = ((val * tmp4 + xFrac * tmp2) << 3) + 32;
                c >>= 6;
                *cbr++ = (u8)c;
            }
            cbr += 2*8 - chromaPartWidth;
            ptrA += 2*width - chromaPartWidth;
        }
    }
}
