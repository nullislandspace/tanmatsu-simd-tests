/*
 * Optimized h264bsdInterpolateHorHalf for ESP32-P4.
 *
 * H.264 horizontal half-pixel 6-tap FIR filter:
 *   out = clp[(A - 5*B + 20*C + 20*D - 5*E + F + 16) >> 5]
 *
 * Strategy: per-row, widen 16 input u8 bytes to i16 into a small
 * aligned scratch buffer (32 bytes). Then load 6 shifted i16 views
 * via usar from this hot-in-cache buffer. The widen cost is paid
 * once per row (5 PIE insns + 2 stores), and the 6 shifted loads
 * are 3 usar insns each (18 total) but with no widen step.
 *
 * Total per 8 output pixels: 5 (widen) + 2 (store) + 18 (6 usar) +
 * 3 (pair-adds) + 8 (multiply chains) + 2 (combine) = 38 PIE insns.
 * Plus 8 scalar clp[] for >>5 + narrow.
 *
 * For partWidth < 8: scalar fallback.
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
    const u8 *clp = TEST_CLP;

    /* Boundary handling */
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

    /* Scalar fallback for partWidth < 8 */
    if (partWidth < 8) {
        u8 *ptrJ = ref + 5;
        for (u32 y = partHeight; y; y--) {
            i32 t6 = *(ptrJ-5), t5 = *(ptrJ-4), t4 = *(ptrJ-3);
            i32 t3 = *(ptrJ-2), t2 = *(ptrJ-1);
            for (u32 x = (partWidth >> 2); x; x--) {
                i32 t7, t1;
                t6 += 16; t7 = t3+t4; t6 += (t7<<4); t6 += (t7<<2);
                t7 = t2+t5; t1 = *ptrJ++; t6 -= (t7<<2); t6 -= t7; t6 += t1;
                t6 = clp[t6>>5];
                t5 += 16; t7 = t2+t3; *mb++ = (u8)t6; t5 += (t7<<4); t5 += (t7<<2);
                t7 = t1+t4; t6 = *ptrJ++; t5 -= (t7<<2); t5 -= t7; t5 += t6;
                t5 = clp[t5>>5];
                t4 += 16; t7 = t1+t2; *mb++ = (u8)t5; t4 += (t7<<4); t4 += (t7<<2);
                t7 = t6+t3; t5 = *ptrJ++; t4 -= (t7<<2); t4 -= t7; t4 += t5;
                t4 = clp[t4>>5];
                t3 += 16; t7 = t6+t1; *mb++ = (u8)t4; t3 += (t7<<4); t3 += (t7<<2);
                t7 = t5+t2; t4 = *ptrJ++; t3 -= (t7<<2); t3 -= t7; t3 += t4;
                t3 = clp[t3>>5];
                t7 = t4; t4 = t6; t6 = t2; t2 = t7;
                *mb++ = (u8)t3; t3 = t5; t5 = t1;
            }
            ptrJ += width - partWidth;
            mb += 16 - partWidth;
        }
        return;
    }

    /*
     * SIMD path for partWidth >= 8.
     *
     * Per 8 output pixels, we need input bytes [x..x+12] (13 bytes).
     * We load 16, widen to i16, store to a 32-byte scratch buffer,
     * then extract 6 shifted views from it.
     */
    for (u32 y = 0; y < partHeight; y++)
    {
        const u8 *srcRow = ref + y * width;
        u8 *outRow = mb + y * 16;

        for (u32 x = 0; x < partWidth; x += 8)
        {
            const u8 *src = srcRow + x;

            /*
             * Widen 16 u8 bytes to i16, store to aligned scratch.
             * Scratch: wide[0..7] in first 16 bytes, wide[8..15] in next 16.
             */
            i16 __attribute__((aligned(16))) wide[16];

            {
                register const u8 *rs asm("a0") = src;
                register i16 *rd asm("a1") = wide;
                asm volatile(
                    "esp.ld.128.usar.ip q0, %[s], 16\n"
                    "esp.ld.128.usar.ip q1, %[s], 0\n"
                    "esp.src.q q0, q0, q1\n"
                    "esp.zero.q q1\n"
                    "esp.vzip.8 q0, q1\n"
                    "esp.vst.128.ip q0, %[d], 16\n"
                    "esp.vst.128.ip q1, %[d], 0\n"
                    : [s] "+r"(rs), [d] "+r"(rd) :: "memory"
                );
            }

            /*
             * Load 6 shifted i16 views from scratch and compute filter.
             * wide is 16-byte aligned. Offsets in bytes:
             *   A: wide+0  (aligned)
             *   B: wide+2  (unaligned)
             *   C: wide+4  (unaligned)
             *   D: wide+6  (unaligned)
             *   E: wide+8  (unaligned)
             *   F: wide+10 (unaligned)
             *
             * For A at offset 0: can use aligned vld.128.
             * For B-F: use usar loads (3 insns each × 5 = 15 insns).
             * Total loads: 1 aligned + 5 usar = 16 insns (was 18 for 6 usar).
             */
            {
                register i16 *rA asm("a0") = wide;
                register i16 *rB asm("a1") = wide + 1;
                register i16 *rC asm("a2") = wide + 2;
                register i16 *rD asm("a3") = wide + 3;
                register i16 *rE asm("a4") = wide + 4;
                register i16 *rF asm("a5") = wide + 5;

                asm volatile(
                    /* Load A (aligned) */
                    "esp.vld.128.ip q2, %[rA], 0\n"

                    /* Load F (usar) and compute A+F */
                    "esp.ld.128.usar.ip q5, %[rF], 16\n"
                    "esp.ld.128.usar.ip q7, %[rF], 0\n"
                    "esp.src.q q5, q5, q7\n"
                    "esp.vadd.s16 q2, q2, q5\n"     /* q2 = A+F */

                    /* Load B (usar) */
                    "esp.ld.128.usar.ip q5, %[rB], 16\n"
                    "esp.ld.128.usar.ip q7, %[rB], 0\n"
                    "esp.src.q q5, q5, q7\n"

                    /* Load E (usar) and compute B+E */
                    "esp.ld.128.usar.ip q6, %[rE], 16\n"
                    "esp.ld.128.usar.ip q7, %[rE], 0\n"
                    "esp.src.q q6, q6, q7\n"
                    "esp.vadd.s16 q3, q5, q6\n"     /* q3 = B+E */

                    /* Load C (usar) */
                    "esp.ld.128.usar.ip q5, %[rC], 16\n"
                    "esp.ld.128.usar.ip q7, %[rC], 0\n"
                    "esp.src.q q5, q5, q7\n"

                    /* Load D (usar) and compute C+D */
                    "esp.ld.128.usar.ip q6, %[rD], 16\n"
                    "esp.ld.128.usar.ip q7, %[rD], 0\n"
                    "esp.src.q q6, q6, q7\n"
                    "esp.vadd.s16 q4, q5, q6\n"     /* q4 = C+D */

                    /* 5*(B+E) */
                    "esp.vadd.s16 q5, q3, q3\n"
                    "esp.vadd.s16 q5, q5, q5\n"
                    "esp.vadd.s16 q3, q5, q3\n"

                    /* 20*(C+D) */
                    "esp.vadd.s16 q5, q4, q4\n"
                    "esp.vadd.s16 q5, q5, q5\n"
                    "esp.vadd.s16 q4, q5, q4\n"
                    "esp.vadd.s16 q4, q4, q4\n"
                    "esp.vadd.s16 q4, q4, q4\n"

                    /* result = (A+F) + 20*(C+D) - 5*(B+E) */
                    "esp.vadd.s16 q5, q2, q4\n"
                    "esp.vsub.s16 q5, q5, q3\n"

                    : [rA] "+r"(rA), [rB] "+r"(rB), [rC] "+r"(rC),
                      [rD] "+r"(rD), [rE] "+r"(rE), [rF] "+r"(rF)
                    :: "memory"
                );

                /* Store result to scratch (reuse wide buffer) */
                rA = wide;
                asm volatile(
                    "esp.vst.128.ip q5, %[r], 0\n"
                    : [r] "+r"(rA) :: "memory"
                );
            }

            /* Scalar >>5 + clip */
            for (int i = 0; i < 8; i++)
                outRow[x + i] = clp[(wide[i] + 16) >> 5];
        }
    }
}
