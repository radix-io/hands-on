/*
 * bench_minifloat.c
 *
 * Demonstrates registering fast converters for sub-bfloat16 types via
 * H5Tregister(), benchmarking them against the HDF5 general loop.
 *
 * Types covered:
 *   FP8  E4M3  (H5T_FLOAT_F8E4M3)   -- AI inference (NVIDIA H100, etc.)
 *   FP8  E5M2  (H5T_FLOAT_F8E5M2)   -- AI training gradients
 *   FP6  E3M2  (H5T_FLOAT_F6E3M2)   -- NVIDIA Blackwell MX format
 *   FP4  E2M1  (H5T_FLOAT_F4E2M1)   -- extreme quantization
 *
 * Strategy:
 *   Widening  (FP* -> float): precomputed lookup table (max 256 entries).
 *   Narrowing (float -> FP*): bit-manipulation with exponent rebias and clamp.
 *
 * Build:
 *   gcc  -O3 -march=native -I<repo>/src -I<repo>/src/H5FDsubfiling \
 *        -I<build>/src bench_minifloat.c -L<build>/bin -lhdf5 \
 *        -Wl,-rpath,<build>/bin -lm -o bench_minifloat
 *
 * Note: the FP8/FP6/FP4 types in HDF5 carry a header warning that the
 * general type-conversion loop has known issues for these formats.
 * The fast converters registered here bypass the general loop entirely.
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "H5Tdevelop.h"
#include "hdf5.h"

#define N    (50 * 1000 * 1000)
#define REPS 3

/* -------------------------------------------------------------------------
 * Timing
 * ---------------------------------------------------------------------- */
static double
now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static double
best_convert(hid_t src, hid_t dst, void *buf, void *scratch, size_t src_bytes)
{
    double best = 1e9;
    for (int r = 0; r < REPS; r++) {
        memcpy(buf, scratch, src_bytes);
        double t0 = now_sec();
        if (H5Tconvert(src, dst, N, buf, NULL, H5P_DEFAULT) < 0) {
            fprintf(stderr, "H5Tconvert failed\n");
            exit(1);
        }
        double t1 = now_sec() - t0;
        if (t1 < best)
            best = t1;
    }
    return best;
}

/* =========================================================================
 * Lookup-table builder for widening: FP* -> float32
 *
 * Given format parameters, decode every possible N-bit code to a float32.
 * FP8: 256 entries, FP6: 64 entries (high 2 bits zero), FP4: 16 entries.
 * ========================================================================= */

typedef struct {
    int sign_bit; /* bit position of sign */
    int epos;     /* bit position of LSB of exponent */
    int esize;    /* exponent width in bits */
    int ebias;    /* exponent bias */
    int mpos;     /* bit position of LSB of mantissa (always 0 here) */
    int msize;    /* mantissa width in bits */
    int prec;     /* total significant bits (sign+exp+mant) */
    /* special: for E4M3fn, exp==max means NaN (no Inf) */
    int nan_all_exp_max; /* 1 = E4M3-fn style: all values with exp=max are NaN */
} fpfmt_t;

static void
build_widen_lut(float *lut, int n_entries, const fpfmt_t *f)
{
    int exp_max = (1 << f->esize) - 1;

    for (int code = 0; code < n_entries; code++) {
        int sign = (code >> f->sign_bit) & 1;
        int exp  = (code >> f->epos) & exp_max;
        int mant = (code >> f->mpos) & ((1 << f->msize) - 1);

        float val;
        if (f->nan_all_exp_max && exp == exp_max) {
            val = (float)NAN;
        }
        else if (!f->nan_all_exp_max && exp == exp_max) {
            /* IEEE-like: Inf if mant==0, NaN otherwise */
            val = mant == 0 ? (float)INFINITY : (float)NAN;
        }
        else if (exp == 0) {
            /* Subnormal (or zero if mant==0) */
            val = ldexpf((float)mant / (1 << f->msize), 1 - f->ebias);
        }
        else {
            /* Normal */
            float m = 1.0f + (float)mant / (1 << f->msize);
            val     = ldexpf(m, exp - f->ebias);
        }
        lut[code] = sign ? -val : val;
    }
}

/* =========================================================================
 * Narrowing helpers: float32 -> FP* via bit manipulation
 *
 * These handle:
 *   - NaN/Inf input
 *   - Overflow (clamp to max or NaN depending on format)
 *   - Underflow (zero; subnormals approximated)
 *   - Normal values with round-half-up
 * ========================================================================= */

/* float32 -> FP8 E4M3 (bias=7, sign@7, exp@[6:3], mant@[2:0])
 * E4M3fn: exponent 0xF is NaN; no Infinity representation. */
static inline uint8_t
f32_to_f8e4m3(uint32_t u)
{
    uint32_t sign   = u >> 31;
    uint32_t exp32  = (u >> 23) & 0xFF;
    uint32_t mant32 = u & 0x7FFFFF;

    /* NaN or Inf -> E4M3 NaN (S.1111.111) */
    if (exp32 == 0xFF)
        return (uint8_t)((sign << 7) | 0x7F);

    /* Rebias: E4M3 bias=7, float32 bias=127; delta=120 */
    int adj_exp = (int)exp32 - 120;

    if (adj_exp >= 15) {
        /* Overflow: saturate to max normal S.1110.111 (±240) */
        return (uint8_t)((sign << 7) | 0x77);
    }

    if (adj_exp <= 0) {
        /* Zero or subnormal result */
        if (exp32 == 0 || adj_exp < -3)
            return (uint8_t)(sign << 7); /* zero */
        /* Rough subnormal: shift mantissa (1|mant) right */
        uint32_t sm = (mant32 | 0x800000u) >> (21 - adj_exp);
        return (uint8_t)((sign << 7) | (sm & 0x7u));
    }

    /* Normal: round mantissa from 23 bits to 3 bits (round-half-up) */
    uint32_t rm = (mant32 + (1u << 19)) >> 20; /* keep 3 bits + carry */
    int      m3 = (int)(rm & 0x7);
    int      e8 = adj_exp + (int)(rm >> 3); /* absorb carry into exponent */

    if (e8 >= 15)
        return (uint8_t)((sign << 7) | 0x77); /* saturate */

    return (uint8_t)((sign << 7) | ((e8 & 0xF) << 3) | m3);
}

/* float32 -> FP8 E5M2 (bias=15, sign@7, exp@[6:2], mant@[1:0])
 * IEEE-like: exp=0x1F, mant=0 -> Inf; mant!=0 -> NaN. */
static inline uint8_t
f32_to_f8e5m2(uint32_t u)
{
    uint32_t sign   = u >> 31;
    uint32_t exp32  = (u >> 23) & 0xFF;
    uint32_t mant32 = u & 0x7FFFFF;

    if (exp32 == 0xFF) {
        /* NaN or Inf -> propagate */
        if (mant32)
            return (uint8_t)((sign << 7) | 0x7F); /* NaN */
        return (uint8_t)((sign << 7) | 0x7C);     /* Inf: S.11111.00 */
    }

    /* Rebias: E5M2 bias=15, float32 bias=127; delta=112 */
    int adj_exp = (int)exp32 - 112;

    if (adj_exp >= 31) {
        /* Overflow -> Inf */
        return (uint8_t)((sign << 7) | 0x7C);
    }

    if (adj_exp <= 0) {
        if (exp32 == 0 || adj_exp < -2)
            return (uint8_t)(sign << 7); /* zero */
        uint32_t sm = (mant32 | 0x800000u) >> (22 - adj_exp);
        return (uint8_t)((sign << 7) | (sm & 0x3u));
    }

    /* Normal: round mantissa from 23 bits to 2 bits */
    uint32_t rm = (mant32 + (1u << 20)) >> 21;
    int      m2 = (int)(rm & 0x3);
    int      e8 = adj_exp + (int)(rm >> 2);

    if (e8 >= 31)
        return (uint8_t)((sign << 7) | 0x7C); /* overflow -> Inf */

    return (uint8_t)((sign << 7) | ((e8 & 0x1F) << 2) | m2);
}

/* float32 -> FP6 E3M2 (bias=3, sign@5, exp@[4:2], mant@[1:0], prec=6)
 * Stored in low 6 bits of a byte. No Infinity (max-exp saturates). */
static inline uint8_t
f32_to_f6e3m2(uint32_t u)
{
    uint32_t sign   = u >> 31;
    uint32_t exp32  = (u >> 23) & 0xFF;
    uint32_t mant32 = u & 0x7FFFFF;

    if (exp32 == 0xFF)
        return (uint8_t)((sign << 5) | 0x1F); /* NaN/Inf -> max */

    /* Rebias: E3M2 bias=3, float32 bias=127; delta=124 */
    int adj_exp = (int)exp32 - 124;

    if (adj_exp >= 7) {
        return (uint8_t)((sign << 5) | 0x1E); /* saturate S.111.11 */
    }

    if (adj_exp <= 0) {
        if (exp32 == 0 || adj_exp < -2)
            return (uint8_t)(sign << 5);
        uint32_t sm = (mant32 | 0x800000u) >> (22 - adj_exp);
        return (uint8_t)((sign << 5) | (sm & 0x3u));
    }

    /* Normal: round mantissa from 23 bits to 2 bits */
    uint32_t rm = (mant32 + (1u << 20)) >> 21;
    int      m2 = (int)(rm & 0x3);
    int      e6 = adj_exp + (int)(rm >> 2);

    if (e6 >= 7)
        return (uint8_t)((sign << 5) | 0x1E);

    return (uint8_t)((sign << 5) | ((e6 & 0x7) << 2) | m2);
}

/* float32 -> FP4 E2M1 (bias=1, sign@3, exp@[2:1], mant@[0], prec=4)
 * Stored in low 4 bits of a byte. Max value: 6.0 */
static inline uint8_t
f32_to_f4e2m1(uint32_t u)
{
    uint32_t sign   = u >> 31;
    uint32_t exp32  = (u >> 23) & 0xFF;
    uint32_t mant32 = u & 0x7FFFFF;

    if (exp32 == 0xFF)
        return (uint8_t)((sign << 3) | 0x7); /* NaN/Inf -> max */

    /* Rebias: E2M1 bias=1, float32 bias=127; delta=126 */
    int adj_exp = (int)exp32 - 126;

    if (adj_exp >= 3) {
        return (uint8_t)((sign << 3) | 0x7); /* saturate S.11.1 = ±6 */
    }

    if (adj_exp <= 0) {
        if (exp32 == 0 || adj_exp < -1)
            return (uint8_t)(sign << 3);
        uint32_t sm = (mant32 | 0x800000u) >> (23 - adj_exp);
        return (uint8_t)((sign << 3) | (sm & 0x1u));
    }

    /* Normal: round mantissa from 23 bits to 1 bit */
    uint32_t rm = (mant32 + (1u << 21)) >> 22;
    int      m1 = (int)(rm & 0x1);
    int      e4 = adj_exp + (int)(rm >> 1);

    if (e4 >= 3)
        return (uint8_t)((sign << 3) | 0x7);

    return (uint8_t)((sign << 3) | ((e4 & 0x3) << 1) | m1);
}

/* =========================================================================
 * H5T_conv_t wrapper functions
 * Each handles INIT / FREE / CONV commands.
 * ========================================================================= */

/* --- FP8 E4M3 widening: FP8 -> float (uses pre-built LUT) --- */

static float g_lut_f8e4m3[256];

static herr_t
conv_f8e4m3_float(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata, size_t nelmts,
                  size_t buf_stride, size_t bkg_stride, void *buf, void *bkg, hid_t dxpl)
{
    switch (cdata->command) {
        case H5T_CONV_INIT:
            cdata->need_bkg = H5T_BKG_NO;
            return 0;
        case H5T_CONV_FREE:
            return 0;
        case H5T_CONV_CONV:
            break;
        default:
            return -1;
    }
    if (buf_stride == 0 && nelmts > 0) {
        const uint8_t *src = (const uint8_t *)buf + (nelmts - 1);
        float         *dst = (float *)buf + (nelmts - 1);
        for (size_t i = nelmts; i > 0; i--, src--, dst--)
            *dst = g_lut_f8e4m3[*src];
        return 0;
    }
    return H5Tconvert(src_id, dst_id, nelmts, buf, bkg, H5P_DEFAULT);
}

/* --- FP8 E4M3 narrowing: float -> FP8 --- */

static herr_t
conv_float_f8e4m3(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata, size_t nelmts,
                  size_t buf_stride, size_t bkg_stride, void *buf, void *bkg, hid_t dxpl)
{
    switch (cdata->command) {
        case H5T_CONV_INIT:
            cdata->need_bkg = H5T_BKG_NO;
            return 0;
        case H5T_CONV_FREE:
            return 0;
        case H5T_CONV_CONV:
            break;
        default:
            return -1;
    }
    if (buf_stride == 0) {
        const float *src = (const float *)buf;
        uint8_t     *dst = (uint8_t *)buf;
        for (size_t i = 0; i < nelmts; i++) {
            uint32_t u;
            memcpy(&u, &src[i], 4);
            dst[i] = f32_to_f8e4m3(u);
        }
        return 0;
    }
    return H5Tconvert(src_id, dst_id, nelmts, buf, bkg, H5P_DEFAULT);
}

/* --- FP8 E5M2 widening: FP8 -> float (LUT) --- */

static float g_lut_f8e5m2[256];

static herr_t
conv_f8e5m2_float(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata, size_t nelmts,
                  size_t buf_stride, size_t bkg_stride, void *buf, void *bkg, hid_t dxpl)
{
    switch (cdata->command) {
        case H5T_CONV_INIT:
            cdata->need_bkg = H5T_BKG_NO;
            return 0;
        case H5T_CONV_FREE:
            return 0;
        case H5T_CONV_CONV:
            break;
        default:
            return -1;
    }
    if (buf_stride == 0 && nelmts > 0) {
        const uint8_t *src = (const uint8_t *)buf + (nelmts - 1);
        float         *dst = (float *)buf + (nelmts - 1);
        for (size_t i = nelmts; i > 0; i--, src--, dst--)
            *dst = g_lut_f8e5m2[*src];
        return 0;
    }
    return H5Tconvert(src_id, dst_id, nelmts, buf, bkg, H5P_DEFAULT);
}

/* --- FP8 E5M2 narrowing: float -> FP8 --- */

static herr_t
conv_float_f8e5m2(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata, size_t nelmts,
                  size_t buf_stride, size_t bkg_stride, void *buf, void *bkg, hid_t dxpl)
{
    switch (cdata->command) {
        case H5T_CONV_INIT:
            cdata->need_bkg = H5T_BKG_NO;
            return 0;
        case H5T_CONV_FREE:
            return 0;
        case H5T_CONV_CONV:
            break;
        default:
            return -1;
    }
    if (buf_stride == 0) {
        const float *src = (const float *)buf;
        uint8_t     *dst = (uint8_t *)buf;
        for (size_t i = 0; i < nelmts; i++) {
            uint32_t u;
            memcpy(&u, &src[i], 4);
            dst[i] = f32_to_f8e5m2(u);
        }
        return 0;
    }
    return H5Tconvert(src_id, dst_id, nelmts, buf, bkg, H5P_DEFAULT);
}

/* --- FP6 E3M2 widening: FP6 -> float (64-entry LUT) --- */

static float g_lut_f6e3m2[64];

static herr_t
conv_f6e3m2_float(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata, size_t nelmts,
                  size_t buf_stride, size_t bkg_stride, void *buf, void *bkg, hid_t dxpl)
{
    switch (cdata->command) {
        case H5T_CONV_INIT:
            cdata->need_bkg = H5T_BKG_NO;
            return 0;
        case H5T_CONV_FREE:
            return 0;
        case H5T_CONV_CONV:
            break;
        default:
            return -1;
    }
    if (buf_stride == 0 && nelmts > 0) {
        const uint8_t *src = (const uint8_t *)buf + (nelmts - 1);
        float         *dst = (float *)buf + (nelmts - 1);
        for (size_t i = nelmts; i > 0; i--, src--, dst--)
            *dst = g_lut_f6e3m2[*src & 0x3F];
        return 0;
    }
    return H5Tconvert(src_id, dst_id, nelmts, buf, bkg, H5P_DEFAULT);
}

/* --- FP6 E3M2 narrowing: float -> FP6 --- */

static herr_t
conv_float_f6e3m2(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata, size_t nelmts,
                  size_t buf_stride, size_t bkg_stride, void *buf, void *bkg, hid_t dxpl)
{
    switch (cdata->command) {
        case H5T_CONV_INIT:
            cdata->need_bkg = H5T_BKG_NO;
            return 0;
        case H5T_CONV_FREE:
            return 0;
        case H5T_CONV_CONV:
            break;
        default:
            return -1;
    }
    if (buf_stride == 0) {
        const float *src = (const float *)buf;
        uint8_t     *dst = (uint8_t *)buf;
        for (size_t i = 0; i < nelmts; i++) {
            uint32_t u;
            memcpy(&u, &src[i], 4);
            dst[i] = f32_to_f6e3m2(u);
        }
        return 0;
    }
    return H5Tconvert(src_id, dst_id, nelmts, buf, bkg, H5P_DEFAULT);
}

/* --- FP4 E2M1 widening: FP4 -> float (16-entry LUT) --- */

static float g_lut_f4e2m1[16];

static herr_t
conv_f4e2m1_float(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata, size_t nelmts,
                  size_t buf_stride, size_t bkg_stride, void *buf, void *bkg, hid_t dxpl)
{
    switch (cdata->command) {
        case H5T_CONV_INIT:
            cdata->need_bkg = H5T_BKG_NO;
            return 0;
        case H5T_CONV_FREE:
            return 0;
        case H5T_CONV_CONV:
            break;
        default:
            return -1;
    }
    if (buf_stride == 0 && nelmts > 0) {
        const uint8_t *src = (const uint8_t *)buf + (nelmts - 1);
        float         *dst = (float *)buf + (nelmts - 1);
        for (size_t i = nelmts; i > 0; i--, src--, dst--)
            *dst = g_lut_f4e2m1[*src & 0xF];
        return 0;
    }
    return H5Tconvert(src_id, dst_id, nelmts, buf, bkg, H5P_DEFAULT);
}

/* --- FP4 E2M1 narrowing: float -> FP4 --- */

static herr_t
conv_float_f4e2m1(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata, size_t nelmts,
                  size_t buf_stride, size_t bkg_stride, void *buf, void *bkg, hid_t dxpl)
{
    switch (cdata->command) {
        case H5T_CONV_INIT:
            cdata->need_bkg = H5T_BKG_NO;
            return 0;
        case H5T_CONV_FREE:
            return 0;
        case H5T_CONV_CONV:
            break;
        default:
            return -1;
    }
    if (buf_stride == 0) {
        const float *src = (const float *)buf;
        uint8_t     *dst = (uint8_t *)buf;
        for (size_t i = 0; i < nelmts; i++) {
            uint32_t u;
            memcpy(&u, &src[i], 4);
            dst[i] = f32_to_f4e2m1(u);
        }
        return 0;
    }
    return H5Tconvert(src_id, dst_id, nelmts, buf, bkg, H5P_DEFAULT);
}

/* =========================================================================
 * Benchmark runner
 * ========================================================================= */

static void
fill_float(float *buf, size_t n)
{
    /* Use a range likely to produce normal values in FP8 E4M3 (max ~240) */
    for (size_t i = 0; i < n; i++)
        buf[i] = ((float)rand() / RAND_MAX) * 400.0f - 200.0f;
}

static void
fill_fp8(uint8_t *buf, size_t n)
{
    for (size_t i = 0; i < n; i++)
        buf[i] = (uint8_t)(rand() & 0xFF);
}

static void
fill_fp6(uint8_t *buf, size_t n)
{
    for (size_t i = 0; i < n; i++)
        buf[i] = (uint8_t)(rand() & 0x3F); /* 6 valid bits */
}

static void
fill_fp4(uint8_t *buf, size_t n)
{
    for (size_t i = 0; i < n; i++)
        buf[i] = (uint8_t)(rand() & 0xF); /* 4 valid bits */
}

typedef void (*fill_fn)(void *, size_t);

static void
run(const char *label, hid_t src_type, hid_t dst_type, fill_fn filler,
    H5T_conv_t fast_fn, const char *conv_name)
{
    size_t src_sz  = (size_t)H5Tget_size(src_type);
    size_t dst_sz  = (size_t)H5Tget_size(dst_type);
    size_t buf_sz  = N * (src_sz > dst_sz ? src_sz : dst_sz);
    void  *buf     = malloc(buf_sz);
    void  *scratch = malloc(N * src_sz);

    filler(buf, N);
    memcpy(scratch, buf, N * src_sz);

    /* General loop: stock converter (no registered converter expected for these types) */
    double t_soft = best_convert(src_type, dst_type, buf, scratch, N * src_sz);

    /* Register our fast converter via H5Tregister */
    if (H5Tregister(H5T_PERS_HARD, conv_name, src_type, dst_type, fast_fn) < 0) {
        fprintf(stderr, "H5Tregister failed for %s\n", label);
        exit(1);
    }

    /* Fast path: registered converter */
    double t_hard = best_convert(src_type, dst_type, buf, scratch, N * src_sz);

    /* Unregister */
    H5Tunregister(H5T_PERS_HARD, conv_name, src_type, dst_type, fast_fn);

    printf("%-30s  loop=%7.4fs  fast=%7.4fs  speedup=%6.1fx\n",
           label, t_soft, t_hard, t_soft / t_hard);

    free(buf);
    free(scratch);
}

/* =========================================================================
 * main
 * ========================================================================= */
int
main(void)
{
    srand(42);

    /* Build lookup tables for widening conversions */
    fpfmt_t f8e4m3 = {7, 3, 4, 7, 0, 3, 8, 1};  /* nan_all_exp_max=1: E4M3fn */
    fpfmt_t f8e5m2 = {7, 2, 5, 15, 0, 2, 8, 0}; /* IEEE-like Inf/NaN */
    fpfmt_t f6e3m2 = {5, 2, 3, 3, 0, 2, 6, 0};
    fpfmt_t f4e2m1 = {3, 1, 2, 1, 0, 1, 4, 0};

    build_widen_lut(g_lut_f8e4m3, 256, &f8e4m3);
    build_widen_lut(g_lut_f8e5m2, 256, &f8e5m2);
    build_widen_lut(g_lut_f6e3m2, 64, &f6e3m2);
    build_widen_lut(g_lut_f4e2m1, 16, &f4e2m1);

    printf("N = %d elements, best of %d runs\n\n", N, REPS);

    /* FP8 E4M3 */
    run("float  -> FP8 E4M3", H5T_NATIVE_FLOAT, H5T_FLOAT_F8E4M3,
        (fill_fn)fill_float, conv_float_f8e4m3, "flt_f8e4m3");
    run("FP8 E4M3  -> float", H5T_FLOAT_F8E4M3, H5T_NATIVE_FLOAT,
        (fill_fn)fill_fp8, conv_f8e4m3_float, "f8e4m3_flt");

    printf("\n");

    /* FP8 E5M2 */
    run("float  -> FP8 E5M2", H5T_NATIVE_FLOAT, H5T_FLOAT_F8E5M2,
        (fill_fn)fill_float, conv_float_f8e5m2, "flt_f8e5m2");
    run("FP8 E5M2  -> float", H5T_FLOAT_F8E5M2, H5T_NATIVE_FLOAT,
        (fill_fn)fill_fp8, conv_f8e5m2_float, "f8e5m2_flt");

    printf("\n");

    /* FP6 E3M2 */
    run("float  -> FP6 E3M2", H5T_NATIVE_FLOAT, H5T_FLOAT_F6E3M2,
        (fill_fn)fill_float, conv_float_f6e3m2, "flt_f6e3m2");
    run("FP6 E3M2  -> float", H5T_FLOAT_F6E3M2, H5T_NATIVE_FLOAT,
        (fill_fn)fill_fp6, conv_f6e3m2_float, "f6e3m2_flt");

    printf("\n");

    /* FP4 E2M1 */
    run("float  -> FP4 E2M1", H5T_NATIVE_FLOAT, H5T_FLOAT_F4E2M1,
        (fill_fn)fill_float, conv_float_f4e2m1, "flt_f4e2m1");
    run("FP4 E2M1  -> float", H5T_FLOAT_F4E2M1, H5T_NATIVE_FLOAT,
        (fill_fn)fill_fp4, conv_f4e2m1_float, "f4e2m1_flt");

    return 0;
}
