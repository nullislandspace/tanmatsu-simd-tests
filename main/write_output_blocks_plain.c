/*
 * Plain C implementation of h264bsdWriteOutputBlocks.
 *
 * Extracted from h264bsd_image.c. Adds i16 row-ordered residuals to
 * u8 predictions, clips to [0,255], writes to image planes.
 */

#include "h264_test_helpers.h"

void write_output_blocks_plain(test_image_t *image, u32 mbNum, u8 *data,
        test_residual_t *pResidual)
{
    u32 picWidth, picSize;
    u8 *lum, *cb, *cr;
    u32 row, col, comp;
    const u8 *clp = TEST_CLP;

    ASSERT(image);
    ASSERT(data);
    ASSERT(mbNum < image->width * image->height);

    picWidth = image->width;
    picSize = picWidth * image->height;
    row = mbNum / picWidth;
    col = mbNum % picWidth;

    /* Output macroblock position in output picture */
    lum = (image->data + row * picWidth * 256 + col * 16);
    cb = (image->data + picSize * 256 + row * picWidth * 64 + col * 8);
    cr = (cb + picSize * 64);

    picWidth *= 16;

    /* Luma: 16 rows of 16 pixels, reading from contiguous row-ordered i16 */
    for (row = 0; row < 16; row++)
    {
        u8 *predRow = data + row * 16;
        u8 *imgRow = lum + row * picWidth;
        i16 *resRow = pResidual->lumaRows[row];

        for (col = 0; col < 16; col++)
            imgRow[col] = clp[predRow[col] + resRow[col]];
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

            for (col = 0; col < 8; col++)
                imgRow[col] = clp[predRow[col] + resRow[col]];
        }
    }
}
