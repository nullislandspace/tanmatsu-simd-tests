/*
 * Shared types, macros and helpers for H264 SIMD function tests.
 *
 * Provides the basetype typedefs, clipping table, test-local structures,
 * and benchmark macros used by all h264 test files.
 */

#pragma once

#include "test_runner.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* ── basetype.h equivalents ─────────────────────────────────── */

typedef unsigned char   u8;
typedef signed char     i8;
typedef unsigned short  u16;
typedef signed short    i16;
typedef unsigned int    u32;
typedef signed int      i32;

/* ── Macros from h264bsd_util.h ─────────────────────────────── */

#define ABS(a)       (((a) < 0) ? -(a) : (a))
#define CLIP3(x,y,z) (((z) < (x)) ? (x) : (((z) > (y)) ? (y) : (z)))
#define CLIP1(z)     (((z) < 0) ? 0 : (((z) > 255) ? 255 : (z)))
#define HANTRO_TRUE  1
#define HANTRO_FALSE 0
#define ASSERT(x)    ((void)0)

/* ── Clipping table (512 zeros, 0..255 identity, 512 x 255) ── */

/* Flash-resident source table */
extern const u8 h264bsd_test_clip[1280];

/* SRAM-resident copy for fast access — call h264_test_init() before use */
extern u8 *h264bsd_test_clip_sram;
#define TEST_CLP (h264bsd_test_clip_sram + 512)

/* Initialize SRAM-resident tables. Must be called once at startup. */
void h264_test_init(void);

/* ── Test-local image struct ────────────────────────────────── */

typedef struct {
    u8 *data;       /* I420 planar buffer (Y + U + V contiguous) */
    u32 width;      /* width in macroblocks */
    u32 height;     /* height in macroblocks */
    u8 *luma;       /* pointer into data: start of Y plane */
    u8 *cb;         /* pointer into data: start of Cb plane */
    u8 *cr;         /* pointer into data: start of Cr plane */
    u8 *ppa_data;   /* PPA-packed output buffer */
    u32 ppa_stride; /* bytes per PPA row */
} test_image_t;

/* ── Test-local residual struct (row-ordered fields only) ──── */

typedef struct {
    i16 lumaRows[16][16]       __attribute__((aligned(16)));
    i16 chromaRows[2][8][8]    __attribute__((aligned(16)));
} test_residual_t;

/* ── Deblocking threshold struct ────────────────────────────── */

typedef struct {
    const u8 *tc0;
    u32 alpha;
    u32 beta;
} test_edge_threshold_t;

/* ── Benchmark helpers ──────────────────────────────────────── */

#define H264_BENCH_LOOPS 10000

static const char H264_TAG[] = "h264_test";

/* ── Function prototypes: plain implementations ─────────────── */

/* convert_ppa */
void convert_ppa_plain(test_image_t *image);
void convert_ppa_simd(test_image_t *image);

/* write_output_blocks */
void write_output_blocks_plain(test_image_t *image, u32 mbNum, u8 *data,
        test_residual_t *pResidual);
void write_output_blocks_simd(test_image_t *image, u32 mbNum, u8 *data,
        test_residual_t *pResidual);

/* deblocking filter functions */
void FilterVerLumaEdge_plain(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        u32 imageWidth);
void FilterHorLumaEdge_plain(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        i32 imageWidth);
void FilterHorLuma_plain(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        i32 imageWidth);
void FilterVerChromaEdge_plain(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        u32 width);
void FilterHorChromaEdge_plain(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        i32 width);
void FilterHorChroma_plain(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        i32 width);

void FilterVerLumaEdge_simd(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        u32 imageWidth);
void FilterHorLumaEdge_simd(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        i32 imageWidth);
void FilterHorLuma_simd(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        i32 imageWidth);
void FilterVerChromaEdge_simd(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        u32 width);
void FilterHorChromaEdge_simd(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        i32 width);
void FilterHorChroma_simd(u8 *data, u32 bS, test_edge_threshold_t *thresholds,
        i32 width);

/* interpolation: FillBlock helper */
void h264bsdFillBlock_test(u8 *ref, u8 *fill, i32 x0, i32 y0,
        u32 width, u32 height, u32 blockWidth, u32 blockHeight,
        u32 fillScanLength);

/* luma interpolation */
void interp_hor_half_plain(u8 *ref, u8 *mb, i32 x0, i32 y0,
        u32 width, u32 height, u32 partWidth, u32 partHeight);
void interp_hor_half_simd(u8 *ref, u8 *mb, i32 x0, i32 y0,
        u32 width, u32 height, u32 partWidth, u32 partHeight);

/* chroma interpolation */
void interp_chroma_hor_plain(u8 *pRef, u8 *predPartChroma, i32 x0, i32 y0,
        u32 width, u32 height, u32 xFrac, u32 chromaPartWidth,
        u32 chromaPartHeight);
void interp_chroma_hor_simd(u8 *pRef, u8 *predPartChroma, i32 x0, i32 y0,
        u32 width, u32 height, u32 xFrac, u32 chromaPartWidth,
        u32 chromaPartHeight);
