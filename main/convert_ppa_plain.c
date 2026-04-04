/*
 * Plain C implementation of h264bsdConvertImageToPpa.
 *
 * Extracted from h264bsd_image.c. Converts a complete I420 image to
 * PPA-packed YUV420 format (ESP32-P4 PPA hardware format).
 *
 * PPA format per 8 luma pixels (12 bytes):
 *   Even rows: [U0][Y0][Y1][U2][Y2][Y3][U4][Y4][Y5][U6][Y6][Y7]
 *   Odd rows:  [V0][Y0][Y1][V2][Y2][Y3][V4][Y4][Y5][V6][Y6][Y7]
 */

#include "h264_test_helpers.h"

void convert_ppa_plain(test_image_t *image)
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

        for (u32 x = 0; x + 7 < pixWidth; x += 8)
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
