/*
 * SIMD (PIE) implementation of h264bsdWriteOutputBlocks for ESP32-P4.
 *
 * Per-pixel operation: output = clamp(pred_u8 + residual_i16, 0, 255)
 *
 * Data layout (row-ordered, NOT 4x4 block-ordered):
 *   - pResidual->lumaRows[16][16]: i16, 16-byte aligned, 32 bytes/row
 *   - pResidual->chromaRows[2][8][8]: i16, 16-byte aligned, 16 bytes/row
 *   - pred (data): contiguous u8, 4-byte aligned, stride 16 (luma) / 8 (chroma)
 *   - output (imgRow): u8 at stride picWidth (NOT aligned)
 *
 * SIMD approach per luma row (16 pixels):
 *   1. Load 16 u8 predictions (usar pattern for unaligned)
 *   2. Widen u8->i16 via vzip.8 with zero register
 *   3. Load 2x 128-bit aligned i16 residuals
 *   4. vadd.s16 (saturating add, fine for [-512..766] range)
 *   5. vmax.s16 with 0, vmin.s16 with 255 to clamp
 *   6. Narrow i16->u8 via vunzip.8 (high bytes are 0 after clamp)
 *   7. Store 16 u8 to aligned temp, memcpy to unaligned output
 */

#include "h264_test_helpers.h"
#include <string.h>

/*
 * Process one luma row: 16 pixels.
 *
 * predRow: 16 x u8, possibly unaligned (4-byte aligned base + row*16)
 * resRow:  16 x i16, 16-byte aligned (32 bytes = 2 x 128-bit)
 * imgRow:  16 x u8 output, unaligned (arbitrary stride offset)
 */
/* Constant vector: 8 x i16 = 255 for clamping */
static const i16 vec_255[8] __attribute__((aligned(16))) = {255,255,255,255,255,255,255,255};

static inline void process_luma_row(u8 *predRow, i16 *resRow, u8 *imgRow)
{
    u8 tmp[16] __attribute__((aligned(16)));

    register u8 *pred_ptr asm("a0") = predRow;
    register i16 *res_ptr asm("a1") = resRow;
    register u8 *tmp_ptr  asm("a2") = tmp;
    register const i16 *v255_ptr asm("a3") = vec_255;

    /*
     * Strategy for unaligned prediction load:
     * Rather than usar (which needs 2 loads + src.q and careful state mgmt),
     * we use two esp.vld.l.64.ip / esp.vld.h.64.ip loads.
     * BUT those also require 16-byte alignment.
     *
     * Simplest correct approach: copy pred to aligned temp, then load.
     * Actually even simpler: use esp.vldext.u8.ip to load 8 bytes with
     * widening (u8->u16) directly. But that also needs alignment.
     *
     * Most robust: memcpy pred to aligned buffer, then aligned load.
     * Cost: 16-byte memcpy is cheap (4 x u32 stores from registers).
     *
     * Alternative: use usar pattern which handles any alignment.
     * The usar pattern loads 16 bytes at aligned-down address, then
     * another 16 at next aligned boundary, then src.q extracts the
     * correct 16 bytes. But we'd need 32 bytes readable past predRow.
     * Since pred is 384 bytes contiguous and we read at most pred+15*16+15
     * = pred+255, and the buffer is 384 bytes, we have room.
     *
     * Let's use the usar pattern — it avoids the temp copy entirely.
     */

    asm volatile(
        /* --- Load 16 u8 predictions (unaligned) --- */
        /* usar loads: load at predRow, USAR captures alignment offset */
        "esp.ld.128.usar.ip q0, %[pred], 16\n"   /* q0 = 16 bytes at aligned_down(pred); pred += 16 */
        "esp.ld.128.usar.ip q1, %[pred], 0\n"     /* q1 = 16 bytes at next boundary */
        "esp.src.q q2, q0, q1\n"                   /* q2 = extract aligned 16 bytes using USAR state */

        /* --- Zero register for widening --- */
        "esp.zero.q q3\n"                          /* q3 = all zeros */

        /* --- Widen u8 to i16: vzip.8 interleaves q2 bytes with q3 zeros --- */
        /* Before: q2 = [b0..b15], q3 = [0..0] */
        /* After:  q2 = [b0,0,b1,0,...,b7,0] (low 8 pixels as u16) */
        /*         q3 = [b8,0,b9,0,...,b15,0] (high 8 pixels as u16) */
        "esp.vzip.8 q2, q3\n"

        /* --- Load 16 i16 residuals (aligned, 32 bytes = 2 x 128-bit) --- */
        "esp.vld.128.ip q4, %[res], 16\n"         /* q4 = resRow[0..7] as i16 */
        "esp.vld.128.ip q5, %[res], 0\n"          /* q5 = resRow[8..15] as i16 */

        /* --- Add: pred_i16 + residual_i16 (saturating) --- */
        "esp.vadd.s16 q2, q2, q4\n"               /* q2 = pred_lo + res_lo */
        "esp.vadd.s16 q3, q3, q5\n"               /* q3 = pred_hi + res_hi */

        /* --- Clamp to [0, 255] --- */
        /* First: max with 0 (clamp negative to 0) */
        "esp.zero.q q6\n"                          /* q6 = 0 vector */
        "esp.vmax.s16 q2, q2, q6\n"               /* q2 = max(q2, 0) */
        "esp.vmax.s16 q3, q3, q6\n"               /* q3 = max(q3, 0) */

        /* Second: min with 255 (clamp above 255) */
        /* Load 255 vector from memory (guaranteed correct) */
        "esp.vld.128.ip q7, %[v255], 0\n"         /* q7 = [255, 255, ...] (8 x i16) */
        "esp.vmin.s16 q2, q2, q7\n"               /* q2 = min(q2, 255) */
        "esp.vmin.s16 q3, q3, q7\n"               /* q3 = min(q3, 255) */

        /* --- Narrow i16 to u8 via vunzip.8 --- */
        /* q2 bytes: [v0, 0, v1, 0, ..., v7, 0] */
        /* q3 bytes: [v8, 0, v9, 0, ..., v15, 0] */
        /* vunzip.8 q2, q3: */
        /*   q2 gets even-indexed bytes: [v0,v1,...,v7, v8,v9,...,v15] */
        /*   q3 gets odd-indexed bytes: [0, 0, ..., 0] */
        "esp.vunzip.8 q2, q3\n"

        /* --- Store 16 u8 result to aligned temp --- */
        "esp.vst.128.ip q2, %[tmp], 0\n"

        : [pred] "+r"(pred_ptr), [res] "+r"(res_ptr), [tmp] "+r"(tmp_ptr),
          [v255] "+r"(v255_ptr)
        :
        : "memory"
    );

    /* Copy 16 bytes from aligned temp to unaligned output */
    memcpy(imgRow, tmp, 16);
}

/*
 * Process one chroma row: 8 pixels.
 *
 * predRow: 8 x u8, possibly unaligned
 * resRow:  8 x i16, 16-byte aligned (16 bytes = 1 x 128-bit)
 * imgRow:  8 x u8 output, unaligned
 */
static inline void process_chroma_row(u8 *predRow, i16 *resRow, u8 *imgRow)
{
    u8 tmp[16] __attribute__((aligned(16)));

    /* For chroma, we only have 8 u8 prediction bytes.
     * Load them into the lower half of a Q register, zero the upper half,
     * then widen to i16. Only one register pair needed.
     *
     * Since pred may not be aligned and we only need 8 bytes,
     * safest to memcpy to aligned temp first (8 bytes is trivial).
     */
    u8 pred_aligned[16] __attribute__((aligned(16)));
    memcpy(pred_aligned, predRow, 8);
    memset(pred_aligned + 8, 0, 8);  /* zero upper half */

    register u8 *pa_ptr   asm("a0") = pred_aligned;
    register i16 *res_ptr asm("a1") = resRow;
    register u8 *tmp_ptr  asm("a2") = tmp;
    register const i16 *v255_ptr asm("a3") = vec_255;

    asm volatile(
        /* Load 8 u8 predictions (aligned, padded to 16 bytes) */
        "esp.vld.128.ip q0, %[pa], 0\n"           /* q0 = [p0..p7, 0..0] */

        /* Zero register for widening */
        "esp.zero.q q1\n"

        /* Widen u8 to i16 via vzip.8 */
        /* q0 = [p0,0,p1,0,...,p7,0] (8 pixels as u16) */
        /* q1 = [0,0,...,0] (upper half, all zero since input was padded) */
        "esp.vzip.8 q0, q1\n"

        /* Load 8 i16 residuals (aligned, 16 bytes) */
        "esp.vld.128.ip q2, %[res], 0\n"

        /* Add: pred + residual (saturating s16) */
        "esp.vadd.s16 q0, q0, q2\n"

        /* Clamp to [0, 255] */
        "esp.zero.q q3\n"
        "esp.vmax.s16 q0, q0, q3\n"               /* max with 0 */
        "esp.vld.128.ip q4, %[v255], 0\n"         /* q4 = [255 x8] */
        "esp.vmin.s16 q0, q0, q4\n"               /* min with 255 */

        /* Narrow i16 to u8 via vunzip.8 */
        "esp.zero.q q1\n"
        "esp.vunzip.8 q0, q1\n"

        /* Store result — we only need the lower 8 bytes (bytes 0-7 of q0) */
        "esp.vst.128.ip q0, %[tmp], 0\n"

        : [pa] "+r"(pa_ptr), [res] "+r"(res_ptr), [tmp] "+r"(tmp_ptr),
          [v255] "+r"(v255_ptr)
        :
        : "memory"
    );

    /* Copy only 8 bytes to unaligned output */
    memcpy(imgRow, tmp, 8);
}

void write_output_blocks_simd(test_image_t *image, u32 mbNum, u8 *data,
        test_residual_t *pResidual)
{
    u32 picWidth, picSize;
    u8 *lum, *cb, *cr;
    u32 row, comp;

    picWidth = image->width;
    picSize = picWidth * image->height;
    row = mbNum / picWidth;
    u32 col = mbNum % picWidth;

    lum = (image->data + row * picWidth * 256 + col * 16);
    cb = (image->data + picSize * 256 + row * picWidth * 64 + col * 8);
    cr = (cb + picSize * 64);

    picWidth *= 16;

    /* Luma: 16 rows of 16 pixels */
    for (row = 0; row < 16; row++)
    {
        u8 *predRow = data + row * 16;
        u8 *imgRow = lum + row * picWidth;
        i16 *resRow = pResidual->lumaRows[row];

        process_luma_row(predRow, resRow, imgRow);
    }

    /* Chroma: 2 planes (Cb, Cr), 8 rows of 8 pixels each */
    picWidth /= 2;

    for (comp = 0; comp < 2; comp++)
    {
        u8 *imgPlane = (comp == 0) ? cb : cr;
        u8 *predBase = data + 256 + comp * 64;

        for (row = 0; row < 8; row++)
        {
            u8 *predRow = predBase + row * 8;
            u8 *imgRow = imgPlane + row * picWidth;
            i16 *resRow = pResidual->chromaRows[comp][row];

            process_chroma_row(predRow, resRow, imgRow);
        }
    }
}
