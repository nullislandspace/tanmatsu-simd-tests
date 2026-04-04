/*
 * Plain C implementation of H.264 deblocking filter functions.
 *
 * Extracted from h264bsd_deblocking.c. Contains the 6 leaf filter functions
 * that apply edge filtering at 4x4 block boundaries.
 */

#include "h264_test_helpers.h"

/*--------------------------------------------------------------------------*/
/*  FilterVerLumaEdge: Filter one vertical 4-pixel luma edge                */
/*--------------------------------------------------------------------------*/
void FilterVerLumaEdge_plain(
  u8 *data,
  u32 bS,
  test_edge_threshold_t *thresholds,
  u32 imageWidth)
{
    i32 delta, tc, tmp;
    u32 i;
    i32 p0, q0, p1, q1, p2, q2;
    u32 tmpFlag;
    const u8 *clp = TEST_CLP;

    u32 alpha = thresholds->alpha;
    u32 beta = thresholds->beta;
    i32 val;

    if (bS < 4)
    {
        tc = thresholds->tc0[bS-1];
        tmp = tc;
        for (i = 4; i; i--, data += imageWidth)
        {
            p1 = data[-2]; p0 = data[-1];
            q0 = data[0]; q1 = data[1];

            if ( ((u32)ABS(p0 - q0) < alpha) &&
                 ((u32)ABS(p1 - p0) < beta)  &&
                 ((u32)ABS(q1 - q0) < beta) )
            {
                p2 = data[-3];
                q2 = data[2];

                if ((u32)ABS(p2 - p0) < beta)
                {
                    val = (p2 + ((p0 + q0 + 1) >> 1) - (p1 << 1)) >> 1;
                    data[-2] = (p1 + CLIP3(-tc, tc, val));
                    tmp++;
                }

                if ((u32)ABS(q2 - q0) < beta)
                {
                    val = (q2 + ((p0 + q0 + 1) >> 1) - (q1 << 1)) >> 1;
                    data[1] = (q1 + CLIP3(-tc, tc, val));
                    tmp++;
                }

                val = (((q0 - p0) << 2) + (p1 - q1) + 4) >> 3;
                delta = CLIP3(-tmp, tmp, val);

                p0 = clp[p0 + delta];
                q0 = clp[q0 - delta];
                tmp = tc;
                data[-1] = p0;
                data[ 0] = q0;
            }
        }
    }
    else
    {
        for (i = 4; i; i--, data += imageWidth)
        {
            p1 = data[-2]; p0 = data[-1];
            q0 = data[0]; q1 = data[1];
            if ( ((u32)ABS(p0-q0) < alpha) &&
                 ((u32)ABS(p1-p0) < beta)  &&
                 ((u32)ABS(q1-q0) < beta) )
            {
                tmpFlag = ((u32)ABS(p0 - q0) < ((alpha >> 2) +2)) ? HANTRO_TRUE : HANTRO_FALSE;

                p2 = data[-3];
                q2 = data[2];

                if (tmpFlag && (u32)ABS(p2-p0) < beta)
                {
                    tmp = p1 + p0 + q0;
                    data[-1] = ((p2 + 2 * tmp + q1 + 4) >> 3);
                    data[-2] = ((p2 + tmp + 2) >> 2);
                    data[-3] = ((2 * data[-4] + 3 * p2 + tmp + 4) >> 3);
                }
                else
                    data[-1] = (2 * p1 + p0 + q1 + 2) >> 2;

                if (tmpFlag && (u32)ABS(q2-q0) < beta)
                {
                    tmp = p0 + q0 + q1;
                    data[0] = ((p1 + 2 * tmp + q2 + 4) >> 3);
                    data[1] = ((tmp + q2 + 2) >> 2);
                    data[2] = ((2 * data[3] + 3 * q2 + tmp + 4) >> 3);
                }
                else
                    data[0] = ((2 * q1 + q0 + p1 + 2) >> 2);
            }
        }
    }
}

/*--------------------------------------------------------------------------*/
/*  FilterHorLumaEdge: Filter one horizontal 4-pixel luma edge              */
/*--------------------------------------------------------------------------*/
void FilterHorLumaEdge_plain(
  u8 *data,
  u32 bS,
  test_edge_threshold_t *thresholds,
  i32 imageWidth)
{
    i32 delta, tc, tmp;
    u32 i;
    u8 p0, q0, p1, q1, p2, q2;
    const u8 *clp = TEST_CLP;
    i32 val;

    tc = thresholds->tc0[bS-1];
    tmp = tc;
    for (i = 4; i; i--, data++)
    {
        p1 = data[-imageWidth*2]; p0 = data[-imageWidth];
        q0 = data[0]; q1 = data[imageWidth];
        if ( ((u32)ABS(p0-q0) < thresholds->alpha) &&
             ((u32)ABS(p1-p0) < thresholds->beta)  &&
             ((u32)ABS(q1-q0) < thresholds->beta) )
        {
            p2 = data[-imageWidth*3];

            if ((u32)ABS(p2-p0) < thresholds->beta)
            {
                val = (p2 + ((p0 + q0 + 1) >> 1) - (p1 << 1)) >> 1;
                data[-imageWidth*2] = (p1 + CLIP3(-tc, tc, val));
                tmp++;
            }

            q2 = data[imageWidth*2];

            if ((u32)ABS(q2-q0) < thresholds->beta)
            {
                val = (q2 + ((p0 + q0 + 1) >> 1) - (q1 << 1)) >> 1;
                data[imageWidth] = (q1 + CLIP3(-tc, tc, val));
                tmp++;
            }

            val = ((((q0 - p0) << 2) + (p1 - q1) + 4) >> 3);
            delta = CLIP3(-tmp, tmp, val);

            p0 = clp[p0 + delta];
            q0 = clp[q0 - delta];
            tmp = tc;
            data[-imageWidth] = p0;
            data[  0] = q0;
        }
    }
}

/*--------------------------------------------------------------------------*/
/*  FilterHorLuma: Filter all 16 horizontal luma pixels at one edge         */
/*--------------------------------------------------------------------------*/
void FilterHorLuma_plain(
  u8 *data,
  u32 bS,
  test_edge_threshold_t *thresholds,
  i32 imageWidth)
{
    i32 delta, tc, tmp;
    u32 i;
    i32 p0, q0, p1, q1, p2, q2;
    u32 tmpFlag;
    const u8 *clp = TEST_CLP;
    u32 alpha = thresholds->alpha;
    u32 beta = thresholds->beta;
    i32 val;

    if (bS < 4)
    {
        tc = thresholds->tc0[bS-1];
        tmp = tc;
        for (i = 16; i; i--, data++)
        {
            p1 = data[-imageWidth*2]; p0 = data[-imageWidth];
            q0 = data[0]; q1 = data[imageWidth];
            if ( ((u32)ABS(p0 - q0) < alpha) &&
                 ((u32)ABS(p1 - p0) < beta)  &&
                 ((u32)ABS(q1 - q0) < beta) )
            {
                p2 = data[-imageWidth*3];

                if ((u32)ABS(p2 - p0) < beta)
                {
                    val = (p2 + ((p0 + q0 + 1) >> 1) - (p1 << 1)) >> 1;
                    data[-imageWidth*2] = (u8)(p1 + CLIP3(-tc, tc, val));
                    tmp++;
                }

                q2 = data[imageWidth*2];

                if ((u32)ABS(q2-q0) < beta)
                {
                    val = (q2 + ((p0 + q0 + 1) >> 1) - (q1 << 1)) >> 1;
                    data[imageWidth] = (u8)(q1 + CLIP3(-tc, tc, val));
                    tmp++;
                }

                val = ((((q0 - p0) << 2) + (p1 - q1) + 4) >> 3);
                delta = CLIP3(-tmp, tmp, val);

                p0 = clp[p0 + delta];
                q0 = clp[q0 - delta];
                tmp = tc;
                data[-imageWidth] = p0;
                data[  0] = q0;
            }
        }
    }
    else
    {
        for (i = 16; i; i--, data++)
        {
            p1 = data[-imageWidth*2]; p0 = data[-imageWidth];
            q0 = data[0]; q1 = data[imageWidth];
            if ( ((u32)ABS(p0 - q0) < alpha) &&
                 ((u32)ABS(p1 - p0) < beta)  &&
                 ((u32)ABS(q1 - q0) < beta) )
            {
                tmpFlag = ((u32)ABS(p0 - q0) < ((alpha >> 2) +2))
                            ? HANTRO_TRUE : HANTRO_FALSE;

                p2 = data[-imageWidth*3];
                q2 = data[imageWidth*2];

                if (tmpFlag && (u32)ABS(p2 - p0) < beta)
                {
                    tmp = p1 + p0 + q0;
                    data[-imageWidth] = (u8)((p2 + 2 * tmp + q1 + 4) >> 3);
                    data[-imageWidth*2] = (u8)((p2 + tmp + 2) >> 2);
                    data[-imageWidth*3] = (u8)((2 * data[-imageWidth*4] +
                                           3 * p2 + tmp + 4) >> 3);
                }
                else
                    data[-imageWidth] = (u8)((2 * p1 + p0 + q1 + 2) >> 2);

                if (tmpFlag && (u32)ABS(q2 - q0) < beta)
                {
                    tmp = p0 + q0 + q1;
                    data[ 0] = (u8)((p1 + 2 * tmp + q2 + 4) >> 3);
                    data[imageWidth] = (u8)((tmp + q2 + 2) >> 2);
                    data[imageWidth*2] = (u8)((2 * data[imageWidth*3] +
                                          3 * q2 + tmp + 4) >> 3);
                }
                else
                    data[0] = (2 * q1 + q0 + p1 + 2) >> 2;
            }
        }
    }
}

/*--------------------------------------------------------------------------*/
/*  FilterVerChromaEdge: Filter one vertical 2-pixel chroma edge            */
/*--------------------------------------------------------------------------*/
void FilterVerChromaEdge_plain(
  u8 *data,
  u32 bS,
  test_edge_threshold_t *thresholds,
  u32 width)
{
    i32 delta, tc;
    u8 p0, q0, p1, q1;
    const u8 *clp = TEST_CLP;

    p1 = data[-2]; p0 = data[-1];
    q0 = data[0]; q1 = data[1];
    if ( ((u32)ABS(p0-q0) < thresholds->alpha) &&
         ((u32)ABS(p1-p0) < thresholds->beta)  &&
         ((u32)ABS(q1-q0) < thresholds->beta) )
    {
        if (bS < 4)
        {
            tc = thresholds->tc0[bS-1] + 1;
            delta = CLIP3(-tc, tc, ((((q0 - p0) << 2) +
                      (p1 - q1) + 4) >> 3));
            p0 = clp[p0 + delta];
            q0 = clp[q0 - delta];
            data[-1] = p0;
            data[ 0] = q0;
        }
        else
        {
            data[-1] = (2 * p1 + p0 + q1 + 2) >> 2;
            data[ 0] = (2 * q1 + q0 + p1 + 2) >> 2;
        }
    }
    data += width;
    p1 = data[-2]; p0 = data[-1];
    q0 = data[0]; q1 = data[1];
    if ( ((u32)ABS(p0-q0) < thresholds->alpha) &&
         ((u32)ABS(p1-p0) < thresholds->beta)  &&
         ((u32)ABS(q1-q0) < thresholds->beta) )
    {
        if (bS < 4)
        {
            tc = thresholds->tc0[bS-1] + 1;
            delta = CLIP3(-tc, tc, ((((q0 - p0) << 2) +
                      (p1 - q1) + 4) >> 3));
            p0 = clp[p0 + delta];
            q0 = clp[q0 - delta];
            data[-1] = p0;
            data[ 0] = q0;
        }
        else
        {
            data[-1] = (2 * p1 + p0 + q1 + 2) >> 2;
            data[ 0] = (2 * q1 + q0 + p1 + 2) >> 2;
        }
    }
}

/*--------------------------------------------------------------------------*/
/*  FilterHorChromaEdge: Filter one horizontal 2-pixel chroma edge          */
/*--------------------------------------------------------------------------*/
void FilterHorChromaEdge_plain(
  u8 *data,
  u32 bS,
  test_edge_threshold_t *thresholds,
  i32 width)
{
    i32 delta, tc;
    u32 i;
    u8 p0, q0, p1, q1;
    const u8 *clp = TEST_CLP;

    tc = thresholds->tc0[bS-1] + 1;
    for (i = 2; i; i--, data++)
    {
        p1 = data[-width*2]; p0 = data[-width];
        q0 = data[0]; q1 = data[width];
        if ( ((u32)ABS(p0-q0) < thresholds->alpha) &&
             ((u32)ABS(p1-p0) < thresholds->beta)  &&
             ((u32)ABS(q1-q0) < thresholds->beta) )
        {
            delta = CLIP3(-tc, tc, ((((q0 - p0) << 2) +
                      (p1 - q1) + 4) >> 3));
            p0 = clp[p0 + delta];
            q0 = clp[q0 - delta];
            data[-width] = p0;
            data[  0] = q0;
        }
    }
}

/*--------------------------------------------------------------------------*/
/*  FilterHorChroma: Filter all 8 horizontal chroma pixels at one edge      */
/*--------------------------------------------------------------------------*/
void FilterHorChroma_plain(
  u8 *data,
  u32 bS,
  test_edge_threshold_t *thresholds,
  i32 width)
{
    i32 delta, tc;
    u32 i;
    u8 p0, q0, p1, q1;
    const u8 *clp = TEST_CLP;

    if (bS < 4)
    {
        tc = thresholds->tc0[bS-1] + 1;
        for (i = 8; i; i--, data++)
        {
            p1 = data[-width*2]; p0 = data[-width];
            q0 = data[0]; q1 = data[width];
            if ( ((u32)ABS(p0-q0) < thresholds->alpha) &&
                 ((u32)ABS(p1-p0) < thresholds->beta)  &&
                 ((u32)ABS(q1-q0) < thresholds->beta) )
            {
                delta = CLIP3(-tc, tc, ((((q0 - p0) << 2) +
                          (p1 - q1) + 4) >> 3));
                p0 = clp[p0 + delta];
                q0 = clp[q0 - delta];
                data[-width] = p0;
                data[  0] = q0;
            }
        }
    }
    else
    {
        for (i = 8; i; i--, data++)
        {
            p1 = data[-width*2]; p0 = data[-width];
            q0 = data[0]; q1 = data[width];
            if ( ((u32)ABS(p0-q0) < thresholds->alpha) &&
                 ((u32)ABS(p1-p0) < thresholds->beta)  &&
                 ((u32)ABS(q1-q0) < thresholds->beta) )
            {
                    data[-width] = (2 * p1 + p0 + q1 + 2) >> 2;
                    data[  0] = (2 * q1 + q0 + p1 + 2) >> 2;
            }
        }
    }
}
