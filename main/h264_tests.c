/*
 * H264 SIMD function test orchestrator.
 *
 * Contains the clipping table, test data generators, correctness tests,
 * and performance benchmarks for all H264 function categories.
 */

#include "h264_tests.h"
#include "h264_test_helpers.h"
#include "esp_timer.h"
#include <stdlib.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════
 * Clipping table: 512 zeros, identity 0..255, 512 x 255
 * Copied from h264bsd_intra_prediction.c
 * ═══════════════════════════════════════════════════════════════════ */

const u8 h264bsd_test_clip[1280] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
    32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
    48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
    64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
    80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
    96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,
    112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
    176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
    192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
    208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
    224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
    240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
};

/* ═══════════════════════════════════════════════════════════════════
 * SRAM-resident clipping table for fast access
 * ═══════════════════════════════════════════════════════════════════ */

u8 *h264bsd_test_clip_sram = NULL;

void h264_test_init(void)
{
    if (h264bsd_test_clip_sram) return; /* already initialized */

    /* Allocate 16-byte aligned copy in SRAM */
    h264bsd_test_clip_sram = (u8 *)aligned_alloc(16, 1280);
    memcpy(h264bsd_test_clip_sram, h264bsd_test_clip, 1280);
}

/* ═══════════════════════════════════════════════════════════════════
 * Helper: allocate and initialize a test I420 image
 * ═══════════════════════════════════════════════════════════════════ */

static test_image_t *alloc_test_image(u32 mbWidth, u32 mbHeight, int with_ppa)
{
    test_image_t *img = calloc(1, sizeof(test_image_t));
    u32 picSize = mbWidth * mbHeight;
    u32 pixWidth = mbWidth * 16;
    /* I420: Y + U + V = 1.5 * width*height pixels */
    u32 dataSize = picSize * 256 + picSize * 64 * 2;

    img->width = mbWidth;
    img->height = mbHeight;
    img->data = aligned_alloc(16, dataSize);
    memset(img->data, 0, dataSize);
    img->luma = img->data;
    img->cb = img->data + picSize * 256;
    img->cr = img->cb + picSize * 64;

    if (with_ppa) {
        u32 ppaStride = pixWidth * 3 / 2;
        u32 pixHeight = mbHeight * 16;
        img->ppa_stride = ppaStride;
        img->ppa_data = aligned_alloc(16, ppaStride * pixHeight);
        memset(img->ppa_data, 0, ppaStride * pixHeight);
    }

    return img;
}

static void free_test_image(test_image_t *img)
{
    if (img) {
        free(img->data);
        free(img->ppa_data);
        free(img);
    }
}

/* ═══════════════════════════════════════════════════════════════════
 * Test 1: h264bsdConvertImageToPpa
 * ═══════════════════════════════════════════════════════════════════ */

static void test_convert_ppa_at_size(pax_buf_t *fb, void (*blit)(void), int *line,
        u32 mbW, u32 mbH, int loops)
{
    char buf[120];
    u32 pixW = mbW * 16, pixH = mbH * 16;
    u32 picSize = mbW * mbH;
    u32 ppaStride = pixW * 3 / 2;
    u32 ppaSize = ppaStride * pixH;

    test_image_t *img_plain = alloc_test_image(mbW, mbH, 1);
    test_image_t *img_simd  = alloc_test_image(mbW, mbH, 1);

    /* Fill Y plane with gradient, U/V with offset patterns */
    for (u32 i = 0; i < picSize * 256; i++) {
        u8 val = (u8)(i & 0xFF);
        img_plain->data[i] = val;
        img_simd->data[i] = val;
    }
    u8 *uBase_p = img_plain->data + picSize * 256;
    u8 *uBase_s = img_simd->data + picSize * 256;
    for (u32 i = 0; i < picSize * 64; i++) {
        u8 uval = (u8)((i + 100) & 0xFF);
        u8 vval = (u8)((i + 200) & 0xFF);
        uBase_p[i] = uval;
        uBase_s[i] = uval;
        uBase_p[picSize * 64 + i] = vval;
        uBase_s[picSize * 64 + i] = vval;
    }

    /* Run both versions */
    convert_ppa_plain(img_plain);
    convert_ppa_simd(img_simd);

    /* Compare PPA output */
    int ok = (memcmp(img_plain->ppa_data, img_simd->ppa_data, ppaSize) == 0);

    /* Benchmark */
    int64_t start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        convert_ppa_plain(img_plain);
    int64_t plain_ns = (esp_timer_get_time() - start) * 1000 / loops;

    start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        convert_ppa_simd(img_simd);
    int64_t simd_ns = (esp_timer_get_time() - start) * 1000 / loops;

    snprintf(buf, sizeof(buf), "ConvertPPA %dx%d: %s plain:%lld simd:%lld ns (%.2fx)",
            (int)pixW, (int)pixH,
            ok ? "PASS" : "FAIL",
            (long long)plain_ns, (long long)simd_ns,
            simd_ns > 0 ? (double)plain_ns / (double)simd_ns : 0.0);
    report(fb, blit, (*line)++, buf);

    free_test_image(img_plain);
    free_test_image(img_simd);
}

static void test_convert_ppa(pax_buf_t *fb, void (*blit)(void), int *line)
{
    /* Small: 48x32 (fits in cache) */
    test_convert_ppa_at_size(fb, blit, line, 3, 2, 10000);
    /* Large: 400x160 (realistic video size, exceeds L1 cache) */
    test_convert_ppa_at_size(fb, blit, line, 25, 10, 100);
}

/* ═══════════════════════════════════════════════════════════════════
 * Test 2: h264bsdWriteOutputBlocks
 * ═══════════════════════════════════════════════════════════════════ */

static void test_write_output_blocks(pax_buf_t *fb, void (*blit)(void), int *line)
{
    char buf[120];
    u32 mbW = 3, mbH = 2;
    u32 picSize = mbW * mbH;

    test_image_t *img_plain = alloc_test_image(mbW, mbH, 0);
    test_image_t *img_simd  = alloc_test_image(mbW, mbH, 0);

    /* Prediction data: 256 luma + 64 Cb + 64 Cr = 384 bytes */
    u8 *pred = aligned_alloc(16, 384);

    /* Residual data */
    test_residual_t *res = aligned_alloc(16, sizeof(test_residual_t));

    srand(42);

    /* Fill prediction with random u8 values */
    for (int i = 0; i < 384; i++)
        pred[i] = (u8)(rand() & 0xFF);

    /* Fill residuals with random i16 in [-256, 255] */
    for (int r = 0; r < 16; r++)
        for (int c = 0; c < 16; c++)
            res->lumaRows[r][c] = (i16)((rand() % 512) - 256);
    for (int p = 0; p < 2; p++)
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                res->chromaRows[p][r][c] = (i16)((rand() % 512) - 256);

    /* Test macroblock 0 */
    u32 mbNum = 0;
    write_output_blocks_plain(img_plain, mbNum, pred, res);
    write_output_blocks_simd(img_simd, mbNum, pred, res);

    /* Compare image data */
    u32 dataSize = picSize * 256 + picSize * 64 * 2;
    int ok = (memcmp(img_plain->data, img_simd->data, dataSize) == 0);

    /* Benchmark */
    int loops = H264_BENCH_LOOPS;
    int64_t start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        write_output_blocks_plain(img_plain, mbNum, pred, res);
    int64_t plain_ns = (esp_timer_get_time() - start) * 1000 / loops;

    start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        write_output_blocks_simd(img_simd, mbNum, pred, res);
    int64_t simd_ns = (esp_timer_get_time() - start) * 1000 / loops;

    snprintf(buf, sizeof(buf), "WriteOutBlk: %s plain:%lld simd:%lld ns (%.2fx)",
            ok ? "PASS" : "FAIL",
            (long long)plain_ns, (long long)simd_ns,
            simd_ns > 0 ? (double)plain_ns / (double)simd_ns : 0.0);
    report(fb, blit, (*line)++, buf);

    free_test_image(img_plain);
    free_test_image(img_simd);
    free(pred);
    free(res);
}

/* ═══════════════════════════════════════════════════════════════════
 * Test 3: Deblocking filter (FilterHorLuma as representative)
 * ═══════════════════════════════════════════════════════════════════ */

/* tc0 table entries for testing (from the standard, indexA=25) */
static const u8 test_tc0[3] = {1, 1, 1};

/*
 * Helper: run FilterHorLuma correctness test with given parameters.
 * Returns 1 on pass, 0 on fail. Optionally reports per-byte mismatch.
 */
static int test_filter_hor_luma_case(
    u32 imgWidth, u32 margin, u32 bS,
    test_edge_threshold_t *thresh,
    int use_gradient, unsigned seed)
{
    u32 imgHeight = 32;
    u32 bufSize = imgWidth * (imgHeight + margin * 2);

    u8 *buf_plain = aligned_alloc(16, bufSize);
    u8 *buf_simd  = aligned_alloc(16, bufSize);

    if (use_gradient) {
        /*
         * Gradient pattern: smooth rows with a small step at the edge.
         * This ensures many pixels pass the threshold test, exercising
         * the filter arithmetic rather than just the early-out path.
         */
        for (u32 row = 0; row < imgHeight + margin * 2; row++) {
            for (u32 col = 0; col < imgWidth; col++) {
                u8 val;
                if (row < margin) {
                    /* Above edge: gradient 120..124 */
                    val = (u8)(120 + (row & 3) + (col & 1));
                } else if (row < margin + 1) {
                    /* Edge row: slight step */
                    val = (u8)(126 + (col & 3));
                } else {
                    /* Below edge: gradient 128..132 */
                    val = (u8)(128 + (row & 3) + (col & 1));
                }
                buf_plain[row * imgWidth + col] = val;
                buf_simd[row * imgWidth + col] = val;
            }
        }
    } else {
        srand(seed);
        for (u32 i = 0; i < bufSize; i++) {
            u8 val = (u8)(rand() & 0xFF);
            buf_plain[i] = val;
            buf_simd[i] = val;
        }
    }

    u8 *edge_plain = buf_plain + margin * imgWidth;
    u8 *edge_simd  = buf_simd  + margin * imgWidth;

    FilterHorLuma_plain(edge_plain, bS, thresh, (i32)imgWidth);
    FilterHorLuma_simd(edge_simd, bS, thresh, (i32)imgWidth);

    int ok = (memcmp(buf_plain, buf_simd, bufSize) == 0);

    if (!ok) {
        /* Log first few mismatches for debugging */
        int shown = 0;
        for (u32 i = 0; i < bufSize && shown < 8; i++) {
            if (buf_plain[i] != buf_simd[i]) {
                u32 row = i / imgWidth;
                u32 col = i % imgWidth;
                ESP_LOGE(H264_TAG,
                    "HorLuma bS=%d mismatch at row=%d col=%d: plain=%d simd=%d",
                    (int)bS, (int)row, (int)col,
                    (int)buf_plain[i], (int)buf_simd[i]);
                shown++;
            }
        }
    }

    free(buf_plain);
    free(buf_simd);
    return ok;
}

static void test_deblock_filter(pax_buf_t *fb, void (*blit)(void), int *line)
{
    char buf[120];
    u32 imgWidth = 64;
    u32 margin = 4;
    u32 bufSize = imgWidth * (32 + margin * 2);

    test_edge_threshold_t thresh;
    thresh.tc0 = test_tc0;
    thresh.alpha = 20;
    thresh.beta = 8;

    /* ---- FilterHorLuma bS=2 (weak filter, random data) ---- */
    int ok_bs2_rand = test_filter_hor_luma_case(
        imgWidth, margin, 2, &thresh, 0, 99);

    /* ---- FilterHorLuma bS=2 (weak filter, gradient data) ---- */
    int ok_bs2_grad = test_filter_hor_luma_case(
        imgWidth, margin, 2, &thresh, 1, 0);

    /* ---- FilterHorLuma bS=4 (strong filter, random data) ---- */
    int ok_bs4_rand = test_filter_hor_luma_case(
        imgWidth, margin, 4, &thresh, 0, 99);

    /* ---- FilterHorLuma bS=4 (strong filter, gradient data) ---- */
    /*
     * Use higher alpha for bS=4 to exercise the strong filter path.
     * With alpha=40, more pixels pass the threshold and the tmpFlag
     * sub-condition (|p0-q0| < alpha/4+2 = 12) is also likely to pass.
     */
    test_edge_threshold_t thresh_strong;
    thresh_strong.tc0 = test_tc0;
    thresh_strong.alpha = 40;
    thresh_strong.beta = 18;

    int ok_bs4_grad = test_filter_hor_luma_case(
        imgWidth, margin, 4, &thresh_strong, 1, 0);

    int ok_hor = ok_bs2_rand && ok_bs2_grad && ok_bs4_rand && ok_bs4_grad;

    /* Benchmark: bS=2 with random data */
    u8 *buf_plain = aligned_alloc(16, bufSize);
    u8 *buf_simd  = aligned_alloc(16, bufSize);

    srand(99);
    for (u32 i = 0; i < bufSize; i++) {
        u8 val = (u8)(rand() & 0xFF);
        buf_plain[i] = val;
        buf_simd[i] = val;
    }

    u8 *edge_plain = buf_plain + margin * imgWidth;
    u8 *edge_simd  = buf_simd  + margin * imgWidth;

    int loops = H264_BENCH_LOOPS;
    int64_t start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        FilterHorLuma_plain(edge_plain, 2, &thresh, (i32)imgWidth);
    int64_t plain_ns = (esp_timer_get_time() - start) * 1000 / loops;

    start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        FilterHorLuma_simd(edge_simd, 2, &thresh, (i32)imgWidth);
    int64_t simd_ns = (esp_timer_get_time() - start) * 1000 / loops;

    snprintf(buf, sizeof(buf), "DeblockHorL: %s plain:%lld simd:%lld ns (%.2fx)",
            ok_hor ? "PASS" : "FAIL",
            (long long)plain_ns, (long long)simd_ns,
            simd_ns > 0 ? (double)plain_ns / (double)simd_ns : 0.0);
    report(fb, blit, (*line)++, buf);

    if (!ok_hor) {
        snprintf(buf, sizeof(buf), "  bs2r=%d bs2g=%d bs4r=%d bs4g=%d",
                ok_bs2_rand, ok_bs2_grad, ok_bs4_rand, ok_bs4_grad);
        report(fb, blit, (*line)++, buf);
    }

    /* Test FilterVerLumaEdge */
    srand(99);
    for (u32 i = 0; i < bufSize; i++) {
        u8 val = (u8)(rand() & 0xFF);
        buf_plain[i] = val;
        buf_simd[i] = val;
    }

    u8 *vedge_plain = buf_plain + margin * imgWidth + 8;
    u8 *vedge_simd  = buf_simd  + margin * imgWidth + 8;

    FilterVerLumaEdge_plain(vedge_plain, 2, &thresh, imgWidth);
    FilterVerLumaEdge_simd(vedge_simd, 2, &thresh, imgWidth);

    int ok = (memcmp(buf_plain, buf_simd, bufSize) == 0);

    start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        FilterVerLumaEdge_plain(vedge_plain, 2, &thresh, imgWidth);
    plain_ns = (esp_timer_get_time() - start) * 1000 / loops;

    start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        FilterVerLumaEdge_simd(vedge_simd, 2, &thresh, imgWidth);
    simd_ns = (esp_timer_get_time() - start) * 1000 / loops;

    snprintf(buf, sizeof(buf), "DeblockVerL: %s plain:%lld simd:%lld ns (%.2fx)",
            ok ? "PASS" : "FAIL",
            (long long)plain_ns, (long long)simd_ns,
            simd_ns > 0 ? (double)plain_ns / (double)simd_ns : 0.0);
    report(fb, blit, (*line)++, buf);

    free(buf_plain);
    free(buf_simd);
}

/* ═══════════════════════════════════════════════════════════════════
 * Test 4: h264bsdInterpolateHorHalf
 * ═══════════════════════════════════════════════════════════════════ */

/* Helper: benchmark one block size of interp_hor_half */
static void bench_interp_hor_half_size(pax_buf_t *fb, void (*blit)(void), int *line,
        u8 *ref, u32 refW, u32 refH, u32 partW, u32 partH, int loops)
{
    char buf[120];
    u8 *mb_plain = aligned_alloc(16, 16 * 16);
    u8 *mb_simd  = aligned_alloc(16, 16 * 16);
    i32 x0 = 4, y0 = 4;

    /* Correctness */
    memset(mb_plain, 0, 16 * 16);
    memset(mb_simd, 0, 16 * 16);
    interp_hor_half_plain(ref, mb_plain, x0, y0, refW, refH, partW, partH);
    interp_hor_half_simd(ref, mb_simd, x0, y0, refW, refH, partW, partH);
    int ok = (memcmp(mb_plain, mb_simd, 16 * 16) == 0);

    /* Benchmark */
    int64_t start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        interp_hor_half_plain(ref, mb_plain, x0, y0, refW, refH, partW, partH);
    int64_t plain_ns = (esp_timer_get_time() - start) * 1000 / loops;

    start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        interp_hor_half_simd(ref, mb_simd, x0, y0, refW, refH, partW, partH);
    int64_t simd_ns = (esp_timer_get_time() - start) * 1000 / loops;

    snprintf(buf, sizeof(buf), "InterpHH %dx%d: %s p:%lld s:%lld ns (%.2fx)",
            (int)partW, (int)partH,
            ok ? "OK" : "FAIL",
            (long long)plain_ns, (long long)simd_ns,
            simd_ns > 0 ? (double)plain_ns / (double)simd_ns : 0.0);
    report(fb, blit, (*line)++, buf);

    free(mb_plain);
    free(mb_simd);
}

static void test_interp_hor_half(pax_buf_t *fb, void (*blit)(void), int *line)
{
    u32 refW = 64, refH = 64;
    u8 *ref = aligned_alloc(16, refW * refH);
    for (u32 i = 0; i < refW * refH; i++)
        ref[i] = (u8)(i & 0xFF);

    /* Benchmark all common H.264 partition sizes */
    bench_interp_hor_half_size(fb, blit, line, ref, refW, refH,  4,  4, 10000);
    bench_interp_hor_half_size(fb, blit, line, ref, refW, refH,  4,  8, 10000);
    bench_interp_hor_half_size(fb, blit, line, ref, refW, refH,  8,  4, 10000);
    bench_interp_hor_half_size(fb, blit, line, ref, refW, refH,  8,  8, 10000);
    bench_interp_hor_half_size(fb, blit, line, ref, refW, refH,  8, 16, 10000);
    bench_interp_hor_half_size(fb, blit, line, ref, refW, refH, 16,  8, 10000);
    bench_interp_hor_half_size(fb, blit, line, ref, refW, refH, 16, 16, 10000);

    free(ref);
}

/* ═══════════════════════════════════════════════════════════════════
 * Test 5: h264bsdInterpolateChromaHor
 * ═══════════════════════════════════════════════════════════════════ */

static void test_interp_chroma_hor(pax_buf_t *fb, void (*blit)(void), int *line)
{
    char buf[120];
    /* Reference frame for chroma: Cb+Cr planes, each 32x32 */
    u32 refW = 32, refH = 32;
    u32 chromaPartW = 8, chromaPartH = 8;

    /* Allocate ref with both Cb and Cr planes contiguous */
    u8 *ref = aligned_alloc(16, refW * refH * 2);
    for (u32 i = 0; i < refW * refH * 2; i++)
        ref[i] = (u8)((i * 7 + 13) & 0xFF);

    /* Output: 2 planes x 8x8 = 128 bytes */
    u8 *out_plain = aligned_alloc(16, 128);
    u8 *out_simd  = aligned_alloc(16, 128);

    int all_ok = 1;

    /* Test with various xFrac values */
    for (u32 xFrac = 1; xFrac < 8; xFrac++)
    {
        memset(out_plain, 0, 128);
        memset(out_simd, 0, 128);

        interp_chroma_hor_plain(ref, out_plain, 4, 4, refW, refH,
                xFrac, chromaPartW, chromaPartH);
        interp_chroma_hor_simd(ref, out_simd, 4, 4, refW, refH,
                xFrac, chromaPartW, chromaPartH);

        if (memcmp(out_plain, out_simd, 128) != 0) {
            all_ok = 0;
            ESP_LOGE(H264_TAG, "ChromaHor FAIL at xFrac=%d", (int)xFrac);
        }
    }

    /* Benchmark with xFrac=3 (typical) */
    int loops = H264_BENCH_LOOPS;
    int64_t start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        interp_chroma_hor_plain(ref, out_plain, 4, 4, refW, refH,
                3, chromaPartW, chromaPartH);
    int64_t plain_ns = (esp_timer_get_time() - start) * 1000 / loops;

    start = esp_timer_get_time();
    for (int i = 0; i < loops; i++)
        interp_chroma_hor_simd(ref, out_simd, 4, 4, refW, refH,
                3, chromaPartW, chromaPartH);
    int64_t simd_ns = (esp_timer_get_time() - start) * 1000 / loops;

    snprintf(buf, sizeof(buf), "ChromaHor 8x8: %s plain:%lld simd:%lld ns (%.2fx)",
            all_ok ? "PASS" : "FAIL",
            (long long)plain_ns, (long long)simd_ns,
            simd_ns > 0 ? (double)plain_ns / (double)simd_ns : 0.0);
    report(fb, blit, (*line)++, buf);

    free(ref);
    free(out_plain);
    free(out_simd);
}

/* ═══════════════════════════════════════════════════════════════════
 * Orchestrator
 * ═══════════════════════════════════════════════════════════════════ */

void run_h264_tests(pax_buf_t *fb, void (*blit)(void), int *line)
{
    h264_test_init();

    report(fb, blit, (*line)++, "--- H264 Function Tests ---");

    test_convert_ppa(fb, blit, line);
    test_write_output_blocks(fb, blit, line);
    test_deblock_filter(fb, blit, line);
    test_interp_hor_half(fb, blit, line);
    test_interp_chroma_hor(fb, blit, line);

    report(fb, blit, (*line)++, "H264 tests complete.");
}
