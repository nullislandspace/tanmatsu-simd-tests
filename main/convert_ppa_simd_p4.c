/*
 * Optimized h264bsdConvertImageToPpa for ESP32-P4.
 *
 * Converts I420 planar YUV to PPA-packed YUV420 format.
 *
 * PPA format per 8 luma pixels (12 bytes):
 *   [C0][Y0][Y1][C1][Y2][Y3][C2][Y4][Y5][C3][Y6][Y7]
 *
 * Analysis: PIE SIMD is NOT well-suited to this function.
 * The 3:1 interleave pattern (1 chroma byte per 2 luma bytes) is
 * fundamentally incompatible with PIE's power-of-2 interleave
 * instructions (vzip.8 does 1:1 interleave only). PIE lacks a
 * general byte shuffle/permute instruction that could rearrange
 * bytes into the required [C,Y,Y] repeating pattern.
 *
 * Instead, this implementation optimizes at the RISC-V scalar level:
 * - Uses 32-bit word loads instead of individual byte loads
 * - Constructs output as 32-bit words using shifts/masks/ORs
 * - Writes 3 words (12 bytes) per 8 Y pixels instead of 12 byte stores
 * - Processes 16 Y + 8 C -> 24 bytes per inner iteration (unrolled 2x)
 *
 * On little-endian RISC-V, a 32-bit store of value
 *   (byte0) | (byte1 << 8) | (byte2 << 16) | (byte3 << 24)
 * writes byte0 to the lowest address.
 */

#include "h264_test_helpers.h"

void convert_ppa_simd(test_image_t *image)
{
    u32 picWidth, picHeight, picSize;

    if (!image->ppa_data)
        return;

    picWidth = image->width;
    picHeight = image->height;
    picSize = picWidth * picHeight;

    u32 pixWidth = picWidth * 16;
    u32 pixHeight = picHeight * 16;
    u32 uvStride = pixWidth / 2;

    u8 *yPlane = image->data;
    u8 *uPlane = image->data + picSize * 256;
    u8 *vPlane = uPlane + picSize * 64;

    u32 ppaStride = image->ppa_stride;

    for (u32 row = 0; row < pixHeight; row++)
    {
        const u8 *y_row = yPlane + row * pixWidth;
        const u8 *c_row = (row & 1)
            ? vPlane + (row >> 1) * uvStride
            : uPlane + (row >> 1) * uvStride;
        u8 *dst = image->ppa_data + row * ppaStride;

        /*
         * Process 16 Y + 8 C -> 24 output bytes per iteration.
         * Each group of 8 Y + 4 C produces 3 x 32-bit output words:
         *
         * word0 = C0 | (Y0 << 8) | (Y1 << 16) | (C1 << 24)
         * word1 = Y2 | (Y3 << 8) | (C2 << 16) | (Y4 << 24)
         * word2 = Y5 | (C3 << 8) | (Y6 << 16) | (Y7 << 24)
         *
         * We load Y as 32-bit words and C as 32-bit words, then
         * extract bytes using shifts and masks.
         */
        u32 x = 0;
        u32 bulk = pixWidth & ~(u32)15; /* round down to multiple of 16 */

        for (; x < bulk; x += 16)
        {
            u32 cx = x >> 1;

            /* Load 16 Y bytes as four 32-bit words */
            u32 y0123 = *(const u32 *)(y_row + x);
            u32 y4567 = *(const u32 *)(y_row + x + 4);
            u32 y89ab = *(const u32 *)(y_row + x + 8);
            u32 ycdef = *(const u32 *)(y_row + x + 12);

            /* Load 8 C bytes as two 32-bit words */
            u32 c0123 = *(const u32 *)(c_row + cx);
            u32 c4567 = *(const u32 *)(c_row + cx + 4);

            /* --- First group: Y[0..7] + C[0..3] -> 12 bytes --- */

            /* Extract individual bytes from Y word (little-endian):
             * y0123 = Y0 | (Y1<<8) | (Y2<<16) | (Y3<<24)
             * y4567 = Y4 | (Y5<<8) | (Y6<<16) | (Y7<<24)
             *
             * c0123 = C0 | (C1<<8) | (C2<<16) | (C3<<24)
             */

            /* word0 = C0 | (Y0 << 8) | (Y1 << 16) | (C1 << 24) */
            u32 w0 = (c0123 & 0xFF)
                   | ((y0123 & 0xFF) << 8)
                   | ((y0123 & 0xFF00) << 8)
                   | ((c0123 & 0xFF00) << 16);

            /* word1 = Y2 | (Y3 << 8) | (C2 << 16) | (Y4 << 24) */
            u32 w1 = ((y0123 >> 16) & 0xFF)
                   | ((y0123 >> 16) & 0xFF00)
                   | ((c0123 & 0xFF0000))
                   | ((y4567 & 0xFF) << 24);

            /* word2 = Y5 | (C3 << 8) | (Y6 << 16) | (Y7 << 24) */
            u32 w2 = ((y4567 >> 8) & 0xFF)
                   | ((c0123 >> 16) & 0xFF00)
                   | ((y4567 & 0xFF0000))
                   | (y4567 & 0xFF000000);

            *(u32 *)(dst + 0) = w0;
            *(u32 *)(dst + 4) = w1;
            *(u32 *)(dst + 8) = w2;

            /* --- Second group: Y[8..15] + C[4..7] -> 12 bytes --- */

            u32 w3 = (c4567 & 0xFF)
                   | ((y89ab & 0xFF) << 8)
                   | ((y89ab & 0xFF00) << 8)
                   | ((c4567 & 0xFF00) << 16);

            u32 w4 = ((y89ab >> 16) & 0xFF)
                   | ((y89ab >> 16) & 0xFF00)
                   | ((c4567 & 0xFF0000))
                   | ((ycdef & 0xFF) << 24);

            u32 w5 = ((ycdef >> 8) & 0xFF)
                   | ((c4567 >> 16) & 0xFF00)
                   | ((ycdef & 0xFF0000))
                   | (ycdef & 0xFF000000);

            *(u32 *)(dst + 12) = w3;
            *(u32 *)(dst + 16) = w4;
            *(u32 *)(dst + 20) = w5;

            dst += 24;
        }

        /* Handle remaining pixels (if pixWidth is not a multiple of 16) */
        for (; x + 7 < pixWidth; x += 8)
        {
            u32 cx = x >> 1;
            dst[0]  = c_row[cx + 0];
            dst[1]  = y_row[x + 0];
            dst[2]  = y_row[x + 1];
            dst[3]  = c_row[cx + 1];
            dst[4]  = y_row[x + 2];
            dst[5]  = y_row[x + 3];
            dst[6]  = c_row[cx + 2];
            dst[7]  = y_row[x + 4];
            dst[8]  = y_row[x + 5];
            dst[9]  = c_row[cx + 3];
            dst[10] = y_row[x + 6];
            dst[11] = y_row[x + 7];
            dst += 12;
        }
    }
}
