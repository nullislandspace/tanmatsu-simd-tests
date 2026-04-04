/*
 * Plain C implementation of h264bsdInterpolateHorHalf and helpers.
 *
 * Extracted from h264bsd_reconstruct.c. Implements horizontal half-pixel
 * luma interpolation using the 6-tap FIR filter (1,-5,20,20,-5,1)/32.
 * Also includes FillBlock and helper functions needed for boundary handling.
 */

#include "h264_test_helpers.h"

/* ── FillRow helpers ──────────────────────────────────────────── */

static void FillRow1(u8 *ref, u8 *fill, i32 left, i32 center, i32 right)
{
    (void)left;
    (void)right;
    memcpy(fill, ref, center);
}

static void FillRow7(u8 *ref, u8 *fill, i32 left, i32 center, i32 right)
{
    u8 tmp = '\0';

    if (left)
        tmp = *ref;

    for ( ; left; left--)
        *fill++ = tmp;

    for ( ; center; center--)
        *fill++ = *ref++;

    if (right)
        tmp = ref[-1];

    for ( ; right; right--)
        *fill++ = tmp;
}

/* ── h264bsdFillBlock ─────────────────────────────────────────── */

void h264bsdFillBlock_test(
  u8 *ref,
  u8 *fill,
  i32 x0,
  i32 y0,
  u32 width,
  u32 height,
  u32 blockWidth,
  u32 blockHeight,
  u32 fillScanLength)
{
    i32 xstop, ystop;
    i32 left, x, right;
    i32 top, y, bottom;

    xstop = x0 + (i32)blockWidth;
    ystop = y0 + (i32)blockHeight;

    if (ystop < 0)
        y0 = -(i32)blockHeight;

    if (xstop < 0)
        x0 = -(i32)blockWidth;

    if (y0 > (i32)height)
        y0 = (i32)height;

    if (x0 > (i32)width)
        x0 = (i32)width;

    xstop = x0 + (i32)blockWidth;
    ystop = y0 + (i32)blockHeight;

    if (x0 > 0)
        ref += x0;

    if (y0 > 0)
        ref += y0 * (i32)width;

    left = x0 < 0 ? -x0 : 0;
    right = xstop > (i32)width ? xstop - (i32)width : 0;
    x = (i32)blockWidth - left - right;

    top = y0 < 0 ? -y0 : 0;
    bottom = ystop > (i32)height ? ystop - (i32)height : 0;
    y = (i32)blockHeight - top - bottom;

    if (x0 >= 0 && xstop <= (i32)width)
    {
        for ( ; top; top-- )
        {
            FillRow1(ref, fill, left, x, right);
            fill += fillScanLength;
        }
        for ( ; y; y-- )
        {
            FillRow1(ref, fill, left, x, right);
            ref += width;
            fill += fillScanLength;
        }
    }
    else
    {
        for ( ; top; top-- )
        {
            FillRow7(ref, fill, left, x, right);
            fill += fillScanLength;
        }
        for ( ; y; y-- )
        {
            FillRow7(ref, fill, left, x, right);
            ref += width;
            fill += fillScanLength;
        }
    }

    ref -= width;

    /* Bottom-overfilling */
    for ( ; bottom; bottom-- )
    {
        if (x0 >= 0 && xstop <= (i32)width)
            FillRow1(ref, fill, left, x, right);
        else
            FillRow7(ref, fill, left, x, right);
        fill += fillScanLength;
    }
}

/* ── h264bsdInterpolateHorHalf ────────────────────────────────── */

void interp_hor_half_plain(
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

        /* calculate 4 pels per iteration */
        for (x = (partWidth >> 2); x; x--)
        {
            /* First pixel */
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
            /* Second pixel */
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
            /* Third pixel */
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
            /* Fourth pixel */
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
