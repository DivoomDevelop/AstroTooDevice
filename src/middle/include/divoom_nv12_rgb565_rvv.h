#ifndef DIVOOM_NV12_RGB565_RVV_H
#define DIVOOM_NV12_RGB565_RVV_H
#include <stdint.h>
#include <stddef.h>
#include <riscv_vector.h>

/* Match the JPEG full-range scalar calculation, including rounding. */
static inline vint32m4_t divoom_rvv_expand(vuint8m1_t value, size_t vl)
{
    return __riscv_vreinterpret_v_u32m4_i32m4(__riscv_vzext_vf2_u32m4(
        __riscv_vzext_vf2_u16m2(value, vl), vl));
}

static inline vuint32m4_t divoom_rvv_clip(vint32m4_t value, size_t vl)
{
    value = __riscv_vsra_vx_i32m4(value, 8, vl);
    value = __riscv_vmax_vx_i32m4(value, 0, vl);
    value = __riscv_vmin_vx_i32m4(value, 255, vl);
    return __riscv_vreinterpret_v_i32m4_u32m4(value);
}

static inline vuint16m2_t divoom_rvv_rgb565(vuint8m1_t y, vuint8m1_t u,
    vuint8m1_t v, size_t vl)
{
    vint32m4_t c = divoom_rvv_expand(y, vl);
    vint32m4_t d = __riscv_vsub_vx_i32m4(divoom_rvv_expand(u, vl), 128, vl);
    vint32m4_t e = __riscv_vsub_vx_i32m4(divoom_rvv_expand(v, vl), 128, vl);
    c = __riscv_vadd_vx_i32m4(__riscv_vmul_vx_i32m4(c, 256, vl), 128, vl);
    vuint32m4_t rgb = divoom_rvv_clip(__riscv_vadd_vv_i32m4(c,
        __riscv_vmul_vx_i32m4(e, 359, vl), vl), vl);
    rgb = __riscv_vsll_vx_u32m4(__riscv_vsrl_vx_u32m4(rgb, 3, vl), 11, vl);
    vuint32m4_t channel = divoom_rvv_clip(__riscv_vsub_vv_i32m4(
        __riscv_vsub_vv_i32m4(c, __riscv_vmul_vx_i32m4(d, 88, vl), vl),
        __riscv_vmul_vx_i32m4(e, 183, vl), vl), vl);
    rgb = __riscv_vor_vv_u32m4(rgb, __riscv_vsll_vx_u32m4(
        __riscv_vsrl_vx_u32m4(channel, 2, vl), 5, vl), vl);
    channel = divoom_rvv_clip(__riscv_vadd_vv_i32m4(c,
        __riscv_vmul_vx_i32m4(d, 454, vl), vl), vl);
    rgb = __riscv_vor_vv_u32m4(rgb, __riscv_vsrl_vx_u32m4(channel, 3, vl), vl);
    return __riscv_vnsrl_wx_u16m2(rgb, 0, vl);
}

/* Input validation and the no-chroma case are handled by the scalar wrapper. */
static void divoom_nv12_rgb565_rvv(const uint8_t *y, const uint8_t *uv,
    uint16_t *out, int width, int height, int y_stride, int uv_stride, int uv_rows)
{
    int pairs = width / 2;
    for(int row = 0; row < height; ++row) {
        int chroma_row = row / 2 < uv_rows ? row / 2 : uv_rows - 1;
        const uint8_t *ys = y + (size_t)row * y_stride;
        const uint8_t *cs = uv + (size_t)chroma_row * uv_stride;
        uint16_t *dst = out + (size_t)row * width;
        for(int pair = 0; pair < pairs;) {
            size_t vl = __riscv_vsetvl_e8m1(pairs - pair);
            vuint8m1_t u = __riscv_vlse8_v_u8m1(cs + pair * 2, 2, vl);
            vuint8m1_t v = __riscv_vlse8_v_u8m1(cs + pair * 2 + 1, 2, vl);
            vuint16m2_t even = divoom_rvv_rgb565(
                __riscv_vlse8_v_u8m1(ys + pair * 2, 2, vl), u, v, vl);
            __riscv_vsse16_v_u16m2(dst + pair * 2, 4, even, vl);
            vuint16m2_t odd = divoom_rvv_rgb565(
                __riscv_vlse8_v_u8m1(ys + pair * 2 + 1, 2, vl), u, v, vl);
            __riscv_vsse16_v_u16m2(dst + pair * 2 + 1, 4, odd, vl);
            pair += (int)vl;
        }
        if(width & 1) {
            int c = 256 * (int)ys[width - 1] + 128;
            int d = (int)cs[(pairs - 1) * 2] - 128;
            int e = (int)cs[(pairs - 1) * 2 + 1] - 128;
            int r = (c + 359 * e) >> 8;
            int g = (c - 88 * d - 183 * e) >> 8;
            int b = (c + 454 * d) >> 8;
            r = r < 0 ? 0 : r > 255 ? 255 : r;
            g = g < 0 ? 0 : g > 255 ? 255 : g;
            b = b < 0 ? 0 : b > 255 ? 255 : b;
            dst[width - 1] = (uint16_t)(((r & 248) << 8) | ((g & 252) << 3) | (b >> 3));
        }
    }
}
#endif
