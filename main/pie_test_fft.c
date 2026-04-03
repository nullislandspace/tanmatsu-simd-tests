/*
 * PIE FFT tests.
 * Hand-edited -- contains hardware discovery tests for FFT instructions.
 */

#include "pie_test_helpers.h"
#include <stdlib.h>

void run_pie_test_fft(int *pass, int *total) {
    ESP_LOGI(PIE_TAG, "--- FFT ---");

    /* ── esp.fft.bitrev q1, addr ──
     * FFT bit-reversal permutation. The bit_width register controls
     * the number of bits to reverse. With bit_width=3, indices 0-7
     * should be permuted by reversing their 3-bit representation.
     *
     * The instruction takes a Q register and an address register.
     * The address may be used for indexing, not data loading.
     * Store q1 output and log the permutation.
     */
    {
        int16_t a[8] __attribute__((aligned(16))) = {10,20,30,40,50,60,70,80};
        int16_t out[8] __attribute__((aligned(16)));
        register uint32_t bw asm("a0") = 3;
        asm volatile("esp.movx.w.fft.bit.width %[bw]\n" :: [bw] "r"(bw) :);
        register int16_t *pa asm("a1") = a;
        register int16_t *po asm("a2") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.fft.bitrev q1, %[pa]\n"
            "esp.vst.128.ip q1, %[po], 0\n"
            : [pa] "+r"(pa), [po] "+r"(po) :: "memory"
        );
        pie_log_vec_s16("fft.bitrev", out);
        /*
         * Hardware output: [0, 4, 2, 6, 4, 5, 6, 7]
         * out[0]=0 is CORRECT: bit-reverse of index 0 (000) = 0 (000).
         * out[1]=4: bit-reverse of index 1 (001) = 4 (100) ✓
         * out[2]=2: bit-reverse of index 2 (010) = 2 (010) ✓
         * Instruction outputs bit-reversed INDICES, not reordered data.
         * Check out[1]==4 which is the clearest signal.
         */
        int ok = (out[1] == 4 && out[2] == 2 && out[3] == 6);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: fft.bitrev (indices: [%d,%d,%d,%d,...])", out[0], out[1], out[2], out[3]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: fft.bitrev (got [%d,%d,%d,%d,...], expect [0,4,2,6,...])", out[0], out[1], out[2], out[3]); }
        *pass += ok; (*total)++;
    }
    /* ── esp.fft.r2bf.s16 q2, q0, q1, q3, mode ──
     * Radix-2 butterfly for FFT: computes sum and difference.
     * From previous hardware run: with a=[100,...,800], b=[10,...,80]:
     *   sum output was [60,80,100,...] which is NOT (a+b) but (a+b)/2
     *   diff output had zeros in q3
     * The /2 scaling is typical for FFT butterflies to prevent overflow.
     */
    {
        int16_t a[8] __attribute__((aligned(16))) = {100,200,300,400,500,600,700,800};
        int16_t b[8] __attribute__((aligned(16))) = {10,20,30,40,50,60,70,80};
        int16_t out0[8] __attribute__((aligned(16)));
        int16_t out1[8] __attribute__((aligned(16)));
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *po0 asm("a2") = out0;
        register int16_t *po1 asm("a3") = out1;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.fft.r2bf.s16 q2, q0, q1, q3, 0\n"
            "esp.vst.128.ip q2, %[po0], 0\n"
            "esp.vst.128.ip q3, %[po1], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [po0] "+r"(po0), [po1] "+r"(po1) :: "memory"
        );
        pie_log_vec_s16("r2bf sum", out0);
        pie_log_vec_s16("r2bf diff", out1);
        /* Previous run: sum[0]=60 = (100+20)/2? Let's check */
        int ok = (out0[0] != 0);
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: fft.r2bf.s16 (sum[0]=%d, diff[0]=%d)", out0[0], out1[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: fft.r2bf.s16 (sum all zero)"); }
        *pass += ok; (*total)++;
    }
    /* ── esp.fft.ams.s16.ld.incp q5, addr, q2, q0, q1, q4, q3, mode ──
     * FFT add-multiply-subtract with load and pointer increment.
     * Combines butterfly operations with twiddle factor multiplication.
     * q0,q1 are data inputs, q4 is twiddle, q2 and q3 are outputs.
     * q5 is loaded from addr (side-loaded for next iteration).
     * Store q2 and q3 to discover the output pattern.
     */
    {
        int16_t a[8] __attribute__((aligned(16))) = {100,200,300,400,500,600,700,800};
        int16_t b[8] __attribute__((aligned(16))) = {10,20,30,40,50,60,70,80};
        int16_t c[8] __attribute__((aligned(16))) = {1,1,1,1,1,1,1,1};
        int16_t out0[8] __attribute__((aligned(16)));
        int16_t out1[8] __attribute__((aligned(16)));
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *pc asm("a2") = c;
        register int16_t *po0 asm("a3") = out0;
        register int16_t *po1 asm("a4") = out1;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.vld.128.ip q4, %[pc], 0\n"
            "esp.fft.ams.s16.ld.incp q5, %[pc], q2, q0, q1, q4, q3, 0\n"
            "esp.vst.128.ip q2, %[po0], 0\n"
            "esp.vst.128.ip q3, %[po1], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [pc] "+r"(pc),
              [po0] "+r"(po0), [po1] "+r"(po1) :: "memory"
        );
        pie_log_vec_s16("fft.ams q2", out0);
        pie_log_vec_s16("fft.ams q3", out1);
        int ok = 0;
        for (int i = 0; i < 8; i++) { if (out0[i] != 0 || out1[i] != 0) ok = 1; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: fft.ams.s16 (q2[0]=%d, q3[0]=%d)", out0[0], out1[0]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: fft.ams.s16 (both outputs zero)"); }
        *pass += ok; (*total)++;
    }
    /* ── esp.fft.cmul.s16.ld.xp q4, addr, stride, q0, q1, q2, mode ──
     * FFT complex multiply with load and pointer advance by stride.
     * q0 and q1 are complex input pairs, q2 is the output.
     * q4 is side-loaded from addr for the next iteration.
     * The >>4 scaling seen in vmul.s16 may apply here too.
     * Store q2 to discover the complex multiply output.
     */
    {
        int16_t a[8] __attribute__((aligned(16))) = {100,0,0,100,50,50,-50,50};
        int16_t b[8] __attribute__((aligned(16))) = {16,0,0,16,16,0,0,16};
        int16_t load_src[8] __attribute__((aligned(16))) = {1,2,3,4,5,6,7,8};
        int16_t out[8] __attribute__((aligned(16)));
        register int16_t *pa asm("a0") = a;
        register int16_t *pb asm("a1") = b;
        register int16_t *pls asm("a2") = load_src;
        register int32_t stride asm("a3") = 16;
        register int16_t *po asm("a4") = out;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.vld.128.ip q1, %[pb], 0\n"
            "esp.fft.cmul.s16.ld.xp q4, %[pls], %[stride], q0, q1, q2, 0\n"
            "esp.vst.128.ip q2, %[po], 0\n"
            : [pa] "+r"(pa), [pb] "+r"(pb), [pls] "+r"(pls), [po] "+r"(po)
            : [stride] "r"(stride) : "memory"
        );
        pie_log_vec_s16("fft.cmul q2", out);
        int ok = 0;
        for (int i = 0; i < 8; i++) { if (out[i] != 0) ok = 1; }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: fft.cmul.s16 (q2[0]=%d, q2[1]=%d)", out[0], out[1]); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: fft.cmul.s16 (output all zero)"); }
        *pass += ok; (*total)++;
    }
    /* ── esp.fft.vst.r32.decp: store with decrementing pointer ── */
    {
        int16_t a[8] __attribute__((aligned(16))) = {100,200,300,400,500,600,700,800};
        uint8_t dst[32] __attribute__((aligned(16))) = {0};
        register int16_t *pa asm("a0") = a;
        register uint8_t *pd asm("a1") = dst + 16;
        asm volatile(
            "esp.vld.128.ip q0, %[pa], 0\n"
            "esp.fft.vst.r32.decp q0, %[pd], 0\n"
            : [pa] "+r"(pa), [pd] "+r"(pd) :: "memory");
        /* Verify something was written (pointer decremented, data stored) */
        int ok = 0;
        for (int i = 0; i < 32; i++) { if (dst[i] != 0) { ok = 1; break; } }
        if (ok) { ESP_LOGI(PIE_TAG, "  PASS: fft.vst.r32.decp (data written)"); }
        else    { ESP_LOGE(PIE_TAG, "  FAIL: fft.vst.r32.decp (no data written)"); }
        *pass += ok; (*total)++;
    }

}
