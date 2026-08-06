/*
 * bench_register.c
 *
 * Demonstrates registering a fast bfloat16 converter at runtime via
 * H5Tregister(), without any changes to the HDF5 library itself.
 *
 * The benchmark:
 *   1. Measures H5Tconvert with only the stock general loop available.
 *   2. Registers the fast bit-manipulation converter via H5Tregister().
 *   3. Measures H5Tconvert again -- now uses the registered fast path.
 *
 * Requires an HDF5 build that includes bfloat16 storage type support
 * (H5T_FLOAT_BFLOAT16LE / H5T_FLOAT_BFLOAT16BE).
 *
 * Build:
 *   gcc  -O3 -march=native -I<hdf5>/src -I<hdf5>/src/H5FDsubfiling \
 *        -I<build>/src bench_register.c -L<build>/bin -lhdf5 \
 *        -Wl,-rpath,<build>/bin -lm -o bench_register
 */

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

/* -------------------------------------------------------------------------
 * Fast-path converter functions (public H5T_conv_t signature)
 * These are identical in logic to the internal converters but use the
 * public API, so they can be registered by any application via H5Tregister.
 * ---------------------------------------------------------------------- */
static inline uint16_t
f32_to_bf16(uint32_t u)
{
    int      is_nan = ((u & 0x7f800000u) == 0x7f800000u) && ((u & 0x007fffffu) != 0u);
    uint32_t rounded = u + 0x8000u;
    return is_nan ? (uint16_t)((u >> 16) | 0x0040u) : (uint16_t)(rounded >> 16);
}

static herr_t
conv_double_bfloat16(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                     size_t nelmts, size_t buf_stride, size_t bkg_stride,
                     void *buf, void *bkg, hid_t dxpl)
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
        double   *src = (double *)buf;
        uint16_t *dst = (uint16_t *)buf;
        for (size_t i = 0; i < nelmts; i++) {
            float    f = (float)src[i];
            uint32_t u;
            memcpy(&u, &f, 4);
            dst[i] = f32_to_bf16(u);
        }
        return 0;
    }
    return H5Tconvert(src_id, dst_id, nelmts, buf, bkg, H5P_DEFAULT);
}

static herr_t
conv_float_bfloat16(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                    size_t nelmts, size_t buf_stride, size_t bkg_stride,
                    void *buf, void *bkg, hid_t dxpl)
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
        float    *src = (float *)buf;
        uint16_t *dst = (uint16_t *)buf;
        for (size_t i = 0; i < nelmts; i++) {
            uint32_t u;
            memcpy(&u, &src[i], 4);
            dst[i] = f32_to_bf16(u);
        }
        return 0;
    }
    return H5Tconvert(src_id, dst_id, nelmts, buf, bkg, H5P_DEFAULT);
}

static herr_t
conv_bfloat16_double(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                     size_t nelmts, size_t buf_stride, size_t bkg_stride,
                     void *buf, void *bkg, hid_t dxpl)
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
        const uint16_t *src = (const uint16_t *)buf + (nelmts - 1);
        double         *dst = (double *)buf + (nelmts - 1);
        for (size_t i = nelmts; i > 0; i--, src--, dst--) {
            uint32_t u = (uint32_t)*src << 16;
            float    f;
            memcpy(&f, &u, 4);
            *dst = (double)f;
        }
        return 0;
    }
    return H5Tconvert(src_id, dst_id, nelmts, buf, bkg, H5P_DEFAULT);
}

static herr_t
conv_bfloat16_float(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                    size_t nelmts, size_t buf_stride, size_t bkg_stride,
                    void *buf, void *bkg, hid_t dxpl)
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
        const uint16_t *src = (const uint16_t *)buf + (nelmts - 1);
        float          *dst = (float *)buf + (nelmts - 1);
        for (size_t i = nelmts; i > 0; i--, src--, dst--) {
            uint32_t u = (uint32_t)*src << 16;
            memcpy(dst, &u, 4);
        }
        return 0;
    }
    return H5Tconvert(src_id, dst_id, nelmts, buf, bkg, H5P_DEFAULT);
}

/* -------------------------------------------------------------------------
 * Benchmark one conversion direction
 * ---------------------------------------------------------------------- */
typedef enum { TY_DOUBLE, TY_FLOAT, TY_BFLOAT16 } elem_ty;

static size_t
ty_size(elem_ty t)
{
    return t == TY_DOUBLE ? sizeof(double) : t == TY_FLOAT ? sizeof(float) : sizeof(uint16_t);
}

static void
fill(void *buf, elem_ty t)
{
    if (t == TY_DOUBLE) {
        double *d = (double *)buf;
        for (size_t i = 0; i < N; i++)
            d[i] = ((double)rand() / RAND_MAX) * 2000.0 - 1000.0;
    }
    else if (t == TY_FLOAT) {
        float *f = (float *)buf;
        for (size_t i = 0; i < N; i++)
            f[i] = ((float)rand() / RAND_MAX) * 2000.0f - 1000.0f;
    }
    else {
        uint16_t *b = (uint16_t *)buf;
        for (size_t i = 0; i < N; i++) {
            float    f = ((float)rand() / RAND_MAX) * 2000.0f - 1000.0f;
            uint32_t u;
            memcpy(&u, &f, 4);
            b[i] = (uint16_t)(u >> 16);
        }
    }
}

static void
run(const char *label, hid_t src_type, hid_t dst_type, elem_ty src_ty,
    H5T_conv_t fast_fn, const char *conv_name, const char *builtin_name)
{
    size_t src_sz  = ty_size(src_ty);
    size_t dst_sz  = (size_t)H5Tget_size(dst_type);
    size_t buf_sz  = N * (src_sz > dst_sz ? src_sz : dst_sz);
    void  *buf     = malloc(buf_sz);
    void  *scratch = malloc(N * src_sz);

    fill(buf, src_ty);
    memcpy(scratch, buf, N * src_sz);

    /* Remove any baked-in hard converter so we measure the true general loop */
    H5Tunregister(H5T_PERS_HARD, builtin_name, src_type, dst_type, NULL);

    /* General loop: stock converter */
    double t_soft = best_convert(src_type, dst_type, buf, scratch, N * src_sz);

    /* Register our fast converter via the public API */
    if (H5Tregister(H5T_PERS_HARD, conv_name, src_type, dst_type, fast_fn) < 0) {
        fprintf(stderr, "H5Tregister failed for %s\n", label);
        exit(1);
    }

    /* Fast path: registered converter */
    double t_hard = best_convert(src_type, dst_type, buf, scratch, N * src_sz);

    /* Unregister */
    H5Tunregister(H5T_PERS_HARD, conv_name, src_type, dst_type, fast_fn);

    printf("%-26s  loop=%7.4fs  fast=%7.4fs  speedup=%5.1fx\n",
           label, t_soft, t_hard, t_soft / t_hard);

    free(buf);
    free(scratch);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int
main(void)
{
    srand(42);
    printf("N = %d elements, best of %d runs\n\n", N, REPS);

    run("double -> bfloat16le", H5T_NATIVE_DOUBLE,    H5T_FLOAT_BFLOAT16LE,
        TY_DOUBLE,   conv_double_bfloat16, "reg_dbl_bf16", "dbl_bf16le");
    run("float  -> bfloat16le", H5T_NATIVE_FLOAT,     H5T_FLOAT_BFLOAT16LE,
        TY_FLOAT,    conv_float_bfloat16,  "reg_flt_bf16", "flt_bf16le");
    run("bfloat16le -> double", H5T_FLOAT_BFLOAT16LE, H5T_NATIVE_DOUBLE,
        TY_BFLOAT16, conv_bfloat16_double, "reg_bf16_dbl", "bf16le_dbl");
    run("bfloat16le -> float",  H5T_FLOAT_BFLOAT16LE, H5T_NATIVE_FLOAT,
        TY_BFLOAT16, conv_bfloat16_float,  "reg_bf16_flt", "bf16le_flt");

    return 0;
}
