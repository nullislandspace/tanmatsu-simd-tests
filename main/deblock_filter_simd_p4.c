/*
 * SIMD-optimized H.264 deblocking filter functions for ESP32-P4.
 *
 * FilterHorLuma processes 16 horizontal pixels at a horizontal edge.
 * Each row is 16 contiguous bytes in memory -- ideal for SIMD loading.
 *
 * Optimization strategy:
 *   Phase 1 (SIMD): Load all 4 threshold rows (p1, p0, q0, q1) and
 *     compute the 16-pixel threshold mask using PIE vector absolute
 *     difference and comparison instructions. This replaces 48 scalar
 *     operations (3 ABS + 3 compares x 16 pixels) with ~12 PIE insns.
 *
 *   Phase 2 (scalar): For each pixel that passes the threshold, apply
 *     the actual filter using scalar code. The filter arithmetic involves
 *     per-pixel conditional logic (p2/q2 sub-tests modify a local tmp
 *     variable) and clp[] table lookups (bS < 4 case), which cannot be
 *     efficiently vectorized.
 *
 * The early-out when mask==0 (no pixels need filtering) avoids all
 * per-pixel work, which is the common case for smooth regions.
 */

#include "h264_test_helpers.h"

void FilterHorLuma_simd(
  u8 *data,
  u32 bS,
  test_edge_threshold_t *thresholds,
  i32 imageWidth)
{
    u32 alpha = thresholds->alpha;
    u32 beta = thresholds->beta;

    /*
     * Phase 1: SIMD threshold mask computation.
     *
     * Load the 4 rows involved in the threshold test (p1, p0, q0, q1)
     * into aligned buffers, then use PIE to compute:
     *   mask[i] = (|p0-q0| < alpha) && (|p1-p0| < beta) && (|q1-q0| < beta)
     *
     * For unsigned absolute difference of u8 values a, b:
     *   |a - b| = max(a -sat b, b -sat a)
     * where -sat is saturating unsigned subtraction.
     *
     * PIE esp.vcmp.lt.u8 sets each byte to 0xFF if less-than, 0x00 otherwise.
     */
    u8 __attribute__((aligned(16))) row_p1[16];
    u8 __attribute__((aligned(16))) row_p0[16];
    u8 __attribute__((aligned(16))) row_q0[16];
    u8 __attribute__((aligned(16))) row_q1[16];
    u8 __attribute__((aligned(16))) bcast_alpha[16];
    u8 __attribute__((aligned(16))) bcast_beta[16];
    u8 __attribute__((aligned(16))) mask_bytes[16];

    /* Copy rows to aligned buffers (source rows may not be 16-byte aligned) */
    memcpy(row_p1, data - imageWidth * 2, 16);
    memcpy(row_p0, data - imageWidth, 16);
    memcpy(row_q0, data, 16);
    memcpy(row_q1, data + imageWidth, 16);

    /* Broadcast threshold values to all 16 lanes */
    memset(bcast_alpha, (u8)alpha, 16);
    memset(bcast_beta, (u8)beta, 16);

    {
        register u8 *ptr0 asm("a0") = row_p1;
        register u8 *ptr1 asm("a1") = row_p0;

        /* Load p1 and p0 */
        asm volatile(
            "esp.vld.128.ip q0, %[r0], 0\n"   /* q0 = p1 */
            "esp.vld.128.ip q1, %[r1], 0\n"   /* q1 = p0 */
            : [r0] "+r"(ptr0), [r1] "+r"(ptr1) :: "memory"
        );

        ptr0 = row_q0;
        ptr1 = row_q1;

        /* Load q0 and q1 */
        asm volatile(
            "esp.vld.128.ip q2, %[r0], 0\n"   /* q2 = q0 */
            "esp.vld.128.ip q3, %[r1], 0\n"   /* q3 = q1 */
            : [r0] "+r"(ptr0), [r1] "+r"(ptr1) :: "memory"
        );

        ptr0 = bcast_alpha;
        ptr1 = bcast_beta;

        /* Load alpha and beta broadcasts */
        asm volatile(
            "esp.vld.128.ip q4, %[r0], 0\n"   /* q4 = alpha */
            "esp.vld.128.ip q5, %[r1], 0\n"   /* q5 = beta  */
            : [r0] "+r"(ptr0), [r1] "+r"(ptr1) :: "memory"
        );

        /* Compute |p0 - q0| < alpha */
        asm volatile(
            "esp.vsub.u8 q6, q1, q2\n"        /* q6 = p0 -sat q0 */
            "esp.vsub.u8 q7, q2, q1\n"        /* q7 = q0 -sat p0 */
            "esp.vmax.u8 q6, q6, q7\n"        /* q6 = |p0 - q0|  */
            "esp.vcmp.lt.u8 q6, q6, q4\n"     /* q6 = mask: |p0-q0| < alpha */
            ::: "memory"
        );

        /* Compute |p1 - p0| < beta, AND with accumulated mask */
        asm volatile(
            "esp.vsub.u8 q7, q0, q1\n"        /* q7 = p1 -sat p0 */
            "esp.vsub.u8 q4, q1, q0\n"        /* q4 = p0 -sat p1 (reuse q4) */
            "esp.vmax.u8 q7, q7, q4\n"        /* q7 = |p1 - p0|  */
            "esp.vcmp.lt.u8 q7, q7, q5\n"     /* q7 = mask: |p1-p0| < beta */
            "esp.andq q6, q6, q7\n"           /* q6 &= q7 */
            ::: "memory"
        );

        /* Compute |q1 - q0| < beta, AND with accumulated mask */
        asm volatile(
            "esp.vsub.u8 q7, q3, q2\n"        /* q7 = q1 -sat q0 */
            "esp.vsub.u8 q4, q2, q3\n"        /* q4 = q0 -sat q1 */
            "esp.vmax.u8 q7, q7, q4\n"        /* q7 = |q1 - q0|  */
            "esp.vcmp.lt.u8 q7, q7, q5\n"     /* q7 = mask: |q1-q0| < beta */
            "esp.andq q6, q6, q7\n"           /* q6 = final mask  */
            ::: "memory"
        );

        /* Store the mask */
        ptr0 = mask_bytes;
        asm volatile(
            "esp.vst.128.ip q6, %[r0], 0\n"
            : [r0] "+r"(ptr0) :: "memory"
        );
    }

    /* Convert byte mask (0xFF/0x00 per lane) to bitmask */
    u32 mask = 0;
    for (int i = 0; i < 16; i++) {
        if (mask_bytes[i]) mask |= (1u << i);
    }

    /* Early-out: no pixels need filtering */
    if (mask == 0) return;

    /*
     * Phase 2: scalar filter application for active pixels.
     */
    if (bS < 4)
    {
        const u8 *clp = TEST_CLP;
        i32 tc = thresholds->tc0[bS - 1];

        u8 *dp = data;
        for (u32 i = 0; i < 16; i++, dp++)
        {
            if (!(mask & (1u << i))) continue;

            i32 p1 = dp[-imageWidth * 2];
            i32 p0 = dp[-imageWidth];
            i32 q0 = dp[0];
            i32 q1 = dp[imageWidth];
            i32 tmp = tc;
            i32 val;

            i32 p2 = dp[-imageWidth * 3];
            if ((u32)ABS(p2 - p0) < beta)
            {
                val = (p2 + ((p0 + q0 + 1) >> 1) - (p1 << 1)) >> 1;
                dp[-imageWidth * 2] = (u8)(p1 + CLIP3(-tc, tc, val));
                tmp++;
            }

            i32 q2 = dp[imageWidth * 2];
            if ((u32)ABS(q2 - q0) < beta)
            {
                val = (q2 + ((p0 + q0 + 1) >> 1) - (q1 << 1)) >> 1;
                dp[imageWidth] = (u8)(q1 + CLIP3(-tc, tc, val));
                tmp++;
            }

            val = ((((q0 - p0) << 2) + (p1 - q1) + 4) >> 3);
            i32 delta = CLIP3(-tmp, tmp, val);

            dp[-imageWidth] = clp[p0 + delta];
            dp[0] = clp[q0 - delta];
        }
    }
    else /* bS == 4 */
    {
        u8 *dp = data;
        for (u32 i = 0; i < 16; i++, dp++)
        {
            if (!(mask & (1u << i))) continue;

            i32 p1 = dp[-imageWidth * 2];
            i32 p0 = dp[-imageWidth];
            i32 q0 = dp[0];
            i32 q1 = dp[imageWidth];
            i32 tmp;

            u32 tmpFlag = ((u32)ABS(p0 - q0) < ((alpha >> 2) + 2))
                            ? HANTRO_TRUE : HANTRO_FALSE;

            i32 p2 = dp[-imageWidth * 3];
            i32 q2 = dp[imageWidth * 2];

            if (tmpFlag && (u32)ABS(p2 - p0) < beta)
            {
                tmp = p1 + p0 + q0;
                dp[-imageWidth] = (u8)((p2 + 2 * tmp + q1 + 4) >> 3);
                dp[-imageWidth * 2] = (u8)((p2 + tmp + 2) >> 2);
                dp[-imageWidth * 3] = (u8)((2 * dp[-imageWidth * 4] +
                                       3 * p2 + tmp + 4) >> 3);
            }
            else
                dp[-imageWidth] = (u8)((2 * p1 + p0 + q1 + 2) >> 2);

            if (tmpFlag && (u32)ABS(q2 - q0) < beta)
            {
                tmp = p0 + q0 + q1;
                dp[0] = (u8)((p1 + 2 * tmp + q2 + 4) >> 3);
                dp[imageWidth] = (u8)((tmp + q2 + 2) >> 2);
                dp[imageWidth * 2] = (u8)((2 * dp[imageWidth * 3] +
                                      3 * q2 + tmp + 4) >> 3);
            }
            else
                dp[0] = (2 * q1 + q0 + p1 + 2) >> 2;
        }
    }
}

/* ── Remaining functions: delegate to plain C ──────────────────── */

void FilterVerLumaEdge_simd(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        u32 imageWidth)
{
    FilterVerLumaEdge_plain(data, bS, thresholds, imageWidth);
}

void FilterHorLumaEdge_simd(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        i32 imageWidth)
{
    FilterHorLumaEdge_plain(data, bS, thresholds, imageWidth);
}

void FilterVerChromaEdge_simd(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        u32 width)
{
    FilterVerChromaEdge_plain(data, bS, thresholds, width);
}

void FilterHorChromaEdge_simd(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        i32 width)
{
    FilterHorChromaEdge_plain(data, bS, thresholds, width);
}

void FilterHorChroma_simd(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        i32 width)
{
    FilterHorChroma_plain(data, bS, thresholds, width);
}
