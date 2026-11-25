#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_music_effect.h"
#include "divoom_pixel_format.h"

#define DIVOOM_TEST_FILE "/userdata/app_pic/20261/400000012-1769492854925-30.webp"
#define DIVOOM_TEST_IMAGE_W 400
#define DIVOOM_TEST_IMAGE_H 500
#define DIVOOM_TEST_IMAGE_DEST_W 320
#define DIVOOM_TEST_IMAGE_DEST_H 320

typedef struct _divoom_lvgl_test_3dgif_ctrl
{
    uint8 *src_buf;
    uint8 *dest_buf;
    void *image_handle;
    void *src_img_obj;
	lv_img_dsc_t src_image_buf;
	lv_img_dsc_t dest_image_buf;
    void *dest_img_obj;
    void *main_obj;
    void *pseudo3d_handle;

    /* 新增：文字飞入 + snapshot 后做伪3D */
    lv_obj_t *text_area;            /* 480x480 区域 */
    uint8_t fly_idx;                /* 已飞入数量 */
    uint8_t fly_total;              /* 总次数=5 */
    const char *fly_texts[5];
    lv_img_dsc_t snap_dsc;          /* snapshot 描述（指向 snap_buf） */
    uint8_t *snap_buf;              /* snapshot 像素缓存（RGB565A8） */
    uint32_t snap_buf_size;

    void *label_shadow_handle;       /* 带阴影的 label（字体 3D 测试用） */
    void *label_outline8_handle;     /* 8 方向描边 label（测试用） */
    void *label_outline8_handle_fill;     /* 8 方向描边 label（测试用） */
    void *fly_shadow_handles[5];     /* 五条飞入文字各自的带阴影 label 句柄 */
    void *scatter_fly_handle;       /* 每字旋转飞入一行（立体描边）测试用 */
}divoom_lvgl_test_3dgif_ctrl, *Pdivoom_lvgl_test_3dgif_ctrl;

/* 伪3D切条效果（在 divoom_lvgl_music_effect.c 中实现） */
void *pseudo3d_img_create_from_bgra(lv_obj_t *parent,
                                    const uint8_t *bgra,
                                    uint16_t w, uint16_t h,
                                    uint16_t slice_cnt);
void *pseudo3d_img_create_from_rgb565a8(lv_obj_t *parent,
                                        const uint8_t *rgb565a8,
                                        uint16_t w, uint16_t h,
                                        uint16_t slice_cnt);
void pseudo3d_img_set(void *handle,
                      lv_coord_t face_x, lv_coord_t face_y,
                      float yaw_deg,
                      lv_coord_t shear_px,
                      float rot_tilt_deg);
void pseudo3d_img_anim_yaw(void *handle,
                           lv_coord_t x, lv_coord_t y,
                           float yaw0_deg, float yaw1_deg,
                           lv_coord_t shear_px,
                           float rot_tilt_deg,
                           uint32_t dur_ms, uint32_t delay_ms);
void pseudo3d_img_delete(void *handle);
typedef struct {
    lv_coord_t top_len;
    lv_coord_t bottom_len;
    lv_coord_t left_len;
    lv_coord_t right_len;
} pseudo3d_edges_t;
void pseudo3d_img_anim_edges(void *handle,
                             lv_coord_t x, lv_coord_t y,
                             const pseudo3d_edges_t *to,
                             uint8_t shade_max,
                             uint32_t dur_ms, uint32_t delay_ms);
void pseudo3d_img_anim_iso_trap3(void *handle,
                                 lv_coord_t x, lv_coord_t y,
                                 lv_coord_t far_h, lv_coord_t near_h, lv_coord_t span_w,
                                 uint8_t shade_max,
                                 uint32_t dur_ms, uint32_t delay_ms);
static void divoom_start_next_fly(Pdivoom_lvgl_test_3dgif_ctrl phandle);
static void divoom_after_all_fly(Pdivoom_lvgl_test_3dgif_ctrl phandle);

typedef struct {
    Pdivoom_lvgl_test_3dgif_ctrl ctrl;
    lv_obj_t *label;
    lv_coord_t start_y;
    lv_coord_t end_y;
} divoom_fly_ctx_t;

static void divoom_fly_exec(void *var, int32_t v)
{
    /* v: 0..1000 */
    divoom_fly_ctx_t *c = (divoom_fly_ctx_t *)var;
    if (!c || !c->label) return;

    lv_coord_t y = c->start_y + (lv_coord_t)(((int32_t)(c->end_y - c->start_y) * v) / 1000);
    lv_obj_set_y(c->label, y);
    lv_obj_set_style_opa(c->label, (lv_opa_t)((255 * v) / 1000), 0);
}

static void divoom_fly_ready(lv_anim_t *a)
{
    divoom_fly_ctx_t *c = (divoom_fly_ctx_t *)a->user_data;
    if (c && c->label) {
        lv_obj_set_y(c->label, c->end_y);
        lv_obj_set_style_opa(c->label, 255, 0);
    }

    if (c && c->ctrl) {
        c->ctrl->fly_idx++;
        if (c->ctrl->fly_idx < c->ctrl->fly_total) divoom_start_next_fly(c->ctrl);
        else divoom_after_all_fly(c->ctrl);
    }

    if (c) divoom_system_free(c);
}

static void divoom_after_all_fly(Pdivoom_lvgl_test_3dgif_ctrl phandle)
{
    if (!phandle || !phandle->text_area) return;

    /* 确保 snapshot 尺寸就是 480x480：去掉会增加 ext_draw_size 的样式（例如 border/shadow） */
    lv_obj_set_style_border_width(phandle->text_area, 0, 0);
    lv_obj_set_style_outline_width(phandle->text_area, 0, 0);
    lv_obj_set_style_shadow_width(phandle->text_area, 0, 0);

    /* 1) snapshot 480x480 区域到缓冲区 */
    uint32_t need = lv_snapshot_buf_size_needed(phandle->text_area, LV_COLOR_FORMAT_RGB565A8);
    if (need == 0) return;

    if (!phandle->snap_buf || phandle->snap_buf_size < need) {
        if (phandle->snap_buf) divoom_system_free(phandle->snap_buf);
        phandle->snap_buf = divoom_system_malloc(need);
        phandle->snap_buf_size = need;
    }
    if (!phandle->snap_buf) return;

    memset(&phandle->snap_dsc, 0, sizeof(phandle->snap_dsc));
    if (lv_snapshot_take_to_buf(phandle->text_area, LV_COLOR_FORMAT_RGB565A8,
                                &phandle->snap_dsc, phandle->snap_buf, phandle->snap_buf_size) != LV_RES_OK) {
        return;
    }

    /* 2) 隐藏文字区域，用 snapshot 图做伪3D */
    lv_obj_add_flag(phandle->text_area, LV_OBJ_FLAG_HIDDEN);

    /* 释放五条飞入的带阴影 label 句柄（控件已随 text_area 隐藏，只需释放句柄内存） */
    for (int i = 0; i < 5; i++) {
        if (phandle->fly_shadow_handles[i]) {
            lv_label_with_shadow_delete(phandle->fly_shadow_handles[i]);
            phandle->fly_shadow_handles[i] = NULL;
        }
    }

    if (phandle->pseudo3d_handle) {
        pseudo3d_img_delete(phandle->pseudo3d_handle);
        phandle->pseudo3d_handle = NULL;
    }

    lv_coord_t fx = 100;
    lv_coord_t fy = DIVOOM_TEST_IMAGE_H + 100;

    /* snapshot 结果的实际宽高（注意：如果有 ext_draw_size，会比 480 更大） */
    uint16_t snap_w = (uint16_t)phandle->snap_dsc.header.w;
    uint16_t snap_h = (uint16_t)phandle->snap_dsc.header.h;
    if (snap_w == 0 || snap_h == 0) return;

    phandle->pseudo3d_handle = pseudo3d_img_create_from_rgb565a8(phandle->main_obj,
                                                                 (const uint8_t *)phandle->snap_buf,
                                                                 snap_w,
                                                                 snap_h,
                                                                 128);
    if (phandle->pseudo3d_handle) {
        /* 最终变换：两个底边 240/360，高 320（对应 far_h/near_h/span_w） */
        pseudo3d_img_anim_iso_trap3(phandle->pseudo3d_handle, fx, fy,
                                    500, 400, 400,
                                    140, /* shade_max */
                                    900, 0);
    }
}

static void divoom_start_next_fly(Pdivoom_lvgl_test_3dgif_ctrl phandle)
{
    if (!phandle || !phandle->text_area) return;
    if (phandle->fly_idx >= phandle->fly_total) {
        divoom_after_all_fly(phandle);
        return;
    }

    const char *txt = phandle->fly_texts[phandle->fly_idx];
    const lv_font_t *font = (const lv_font_t *)divoom_menu_control_get_font_info(48, 0);
    lv_label_segment_t seg = { txt ? txt : "", font };

    /* 五条飞入文字全部使用带阴影的 label（多段接口，此处单段；0=直接显示） */
    lv_label_shadow_handle_t *h = (lv_label_shadow_handle_t *)lv_label_create_with_shadow(
        phandle->text_area,
        &seg, 1,
        0,                              /* max_width：0=不换行 */
        2, 2,
        lv_color_hex(0x404040), 200,
        lv_color_black(),
        0                               /* typing_duration_ms：0=直接显示 */
    );
    if (!h) return;

    phandle->fly_shadow_handles[phandle->fly_idx] = h;

    /* 容器宽度 480，与 text_area 一致；内层 label 裁剪、居中 */
    lv_obj_set_width(h->cont, DIVOOM_TEST_IMAGE_W);
    lv_obj_set_style_opa(h->cont, 0, 0);
    lv_obj_t *shadow_lab = lv_obj_get_child(h->cont, 0);
    if (shadow_lab) {
        lv_label_set_long_mode(shadow_lab, LV_LABEL_LONG_CLIP);
        lv_obj_set_width(shadow_lab, DIVOOM_TEST_IMAGE_W);
        lv_obj_set_style_text_align(shadow_lab, LV_TEXT_ALIGN_CENTER, 0);
    }
    lv_label_set_long_mode(h->main, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(h->main, DIVOOM_TEST_IMAGE_W);
    lv_obj_set_style_text_align(h->main, LV_TEXT_ALIGN_CENTER, 0);

    /* 目标位置：把 480 高度等分成 5 个槽位，每条居中到自己的槽位里 */
    lv_obj_update_layout(h->cont);
    lv_coord_t lab_h = lv_obj_get_height(h->cont);
    if (lab_h < 1) lab_h = 1;

    lv_coord_t slot_h = DIVOOM_TEST_IMAGE_H / phandle->fly_total;
    if (slot_h < 1) slot_h = 1;

    lv_coord_t end_y = (lv_coord_t)phandle->fly_idx * slot_h + (slot_h - lab_h) / 2;
    if (end_y < 0) end_y = 0;
    if (end_y > DIVOOM_TEST_IMAGE_H - lab_h) end_y = DIVOOM_TEST_IMAGE_H - lab_h;

    lv_coord_t start_y = DIVOOM_TEST_IMAGE_H + 30;
    lv_obj_set_pos(h->cont, 0, start_y);

    divoom_fly_ctx_t *ctx = (divoom_fly_ctx_t *)divoom_system_malloc(sizeof(*ctx));
    if (!ctx) return;
    ctx->ctrl = phandle;
    ctx->label = h->cont;   /* 动画移动的是带阴影 label 的容器 */
    ctx->start_y = start_y;
    ctx->end_y = end_y;

    lv_anim_t an;
    lv_anim_init(&an);
    lv_anim_set_var(&an, ctx);
    lv_anim_set_exec_cb(&an, divoom_fly_exec);
    lv_anim_set_values(&an, 0, 1000);
    lv_anim_set_time(&an, 520);
    lv_anim_set_delay(&an, 60);
    lv_anim_set_path_cb(&an, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&an, divoom_fly_ready);
    an.user_data = ctx;
    lv_anim_start(&an);
}

void *divoom_lvgl_test_3dgif_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_test_3dgif_ctrl phandle;
    uint8 *rgb_buf, *dest_buf;
    int image_w,image_h;

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_test_3dgif_ctrl));
    
    rgb_buf = divoom_rk_mpp_get_jpeg_info(DIVOOM_TEST_FILE, &image_w, &image_h);
	phandle->main_obj = lv_obj_create(NULL);
    #if 1
    dest_buf = divoom_system_malloc(divoom_pixel_format_size(DIVOOM_TEST_IMAGE_W, DIVOOM_TEST_IMAGE_H, LV_COLOR_FORMAT_RGB565A8));
    divoom_rk_mpp_stretch_image_lvfmt(rgb_buf, image_w, image_h,
        RK_FORMAT_RGB_565, dest_buf, DIVOOM_TEST_IMAGE_W, DIVOOM_TEST_IMAGE_H,
        LV_COLOR_FORMAT_RGB565A8);
    phandle->src_buf = dest_buf;
    phandle->src_img_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(phandle->src_img_obj, 100, 0);
    lv_obj_set_size(phandle->src_img_obj, DIVOOM_TEST_IMAGE_W, DIVOOM_TEST_IMAGE_H);
    
    divoom_lv_img_dsc_init_fmt(&phandle->src_image_buf,
        DIVOOM_TEST_IMAGE_W, DIVOOM_TEST_IMAGE_H, LV_COLOR_FORMAT_RGB565A8, phandle->src_buf);
    lv_img_set_src(phandle->src_img_obj, &phandle->src_image_buf);
    
    dest_buf = divoom_system_malloc(divoom_pixel_format_size(DIVOOM_TEST_IMAGE_DEST_W, DIVOOM_TEST_IMAGE_DEST_H, LV_COLOR_FORMAT_RGB565A8));
    phandle->dest_buf = dest_buf;


    phandle->dest_img_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_pos(phandle->dest_img_obj, 100, DIVOOM_TEST_IMAGE_H + 100);
    lv_obj_set_size(phandle->dest_img_obj, DIVOOM_TEST_IMAGE_DEST_W, DIVOOM_TEST_IMAGE_DEST_H);
    
    /* dest 图像描述必须和 dest_buf 尺寸一致，否则 LVGL 会越界读，出现破碎/花屏 */
    divoom_lv_img_dsc_init_fmt(&phandle->dest_image_buf,
        DIVOOM_TEST_IMAGE_DEST_W, DIVOOM_TEST_IMAGE_DEST_H, LV_COLOR_FORMAT_RGB565A8, phandle->dest_buf);
#endif
    /* 新功能：480x480 文字区域（5次飞入） -> snapshot -> 伪3D 变换 */
    phandle->fly_total = 5;
    phandle->fly_idx = 0;
    phandle->fly_texts[0] = "Line 1: Hello from below";
    phandle->fly_texts[1] = "Line 2: LVGL fly-in text";
    phandle->fly_texts[2] = "Line 3: Snapshot to image";
    phandle->fly_texts[3] = "Line 4: Pseudo 3D transform";
    phandle->fly_texts[4] = "Line 5: Done";
    for (int i = 0; i < 5; i++) phandle->fly_shadow_handles[i] = NULL;

    phandle->text_area = lv_obj_create(phandle->main_obj);
    lv_obj_set_pos(phandle->text_area, 100, DIVOOM_TEST_IMAGE_H + 100);
    lv_obj_set_size(phandle->text_area, DIVOOM_TEST_IMAGE_W, DIVOOM_TEST_IMAGE_H);
    lv_obj_clear_flag(phandle->text_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(phandle->text_area, 0, 0);
    lv_obj_set_style_border_width(phandle->text_area, 2, 0);
    lv_obj_set_style_border_color(phandle->text_area, lv_color_hex(0x808080), 0);
    lv_obj_set_style_bg_opa(phandle->text_area, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(phandle->text_area, lv_color_white(), 0);

    /* 开始第一条飞入 */
    divoom_start_next_fly(phandle);

    phandle->label_shadow_handle = NULL;
    phandle->label_outline8_handle = NULL;

    
    divoom_system_free(rgb_buf);
    
    return phandle;
}

void divoom_lvgl_test_3dgif_disp(void *handle, void *active)
{
    Pdivoom_lvgl_test_3dgif_ctrl phandle = (Pdivoom_lvgl_test_3dgif_ctrl)handle;

    lv_scr_load(phandle->main_obj);

    /* 8 方向描边 label 测试：多段接口，此处单段 */
    if (!phandle->label_outline8_handle) {
        const lv_font_t *font48 = (const lv_font_t *)divoom_menu_control_get_font_info(48, 0);
        const lv_font_t *font32 = (const lv_font_t *)divoom_menu_control_get_font_info(32, 0);
        lv_label_segment_t seg_trans[3] = {{ "Hello ", font48 },{ "8 Dir", font32 },{ " trans", font48 }};
        phandle->label_outline8_handle = lv_label_create_with_outline8(
            phandle->main_obj,
            seg_trans, 3,
            0,                              /* max_width：0=不换行 */
            2,
            lv_color_hex(0x1a237e), 255,
            LV_LABEL_OUTLINE8_MODE_TRANSPARENT_CENTER,
            lv_color_white(),
            1200,                              /* typing_duration_ms：约1.2秒打完 */
            NULL);                            /* cursor_img：打字机时“笔”位置对象，NULL 不显示 */
        if (phandle->label_outline8_handle) {
            lv_obj_set_pos(((lv_label_outline8_handle_t *)phandle->label_outline8_handle)->cont,
                         100, 200);
        }
        lv_label_segment_t seg_fill = { "Hello 8 Dir Fill", font48 };
        phandle->label_outline8_handle_fill = lv_label_create_with_outline8(
            phandle->main_obj,
            &seg_fill, 1,
            0,                              /* max_width：0=不换行 */
            2,
            lv_color_hex(0xFF00), 255,
            LV_LABEL_OUTLINE8_MODE_OPAQUE_FILL,
            lv_color_hex(0xFF),
            1200,                              /* typing_duration_ms：约1.2秒打完 */
            NULL);                             /* cursor_img：打字机时“笔”位置对象，NULL 不显示 */
        if (phandle->label_outline8_handle_fill) {
            lv_obj_set_pos(((lv_label_outline8_handle_t *)phandle->label_outline8_handle_fill)->cont,
                         100, 100);
        }
        /* 每字独立旋转飞入、最终排成一行（立体描边），文本/字体用 lv_label_segment_t 数组，可不同字号 */
        if (!phandle->scatter_fly_handle) {
            const lv_font_t *font32 = (const lv_font_t *)divoom_menu_control_get_font_info(32, 0);
            const lv_font_t *font48 = (const lv_font_t *)divoom_menu_control_get_font_info(48, 0);
            if (font32) {
                /* 段数组：与 lv_label_create_with_outline8 一致，每段 .text + .font，可不同字号 */
                lv_label_segment_t segs[] = {
                    { "陪你到", font32 },
                    { "想去的地方", font48 ? font48 : font32 }
                };
                uint32_t segment_count = sizeof(segs) / sizeof(segs[0]);
                lv_obj_t *parent = phandle->main_obj;
                lv_coord_t offset_px = 2;
                lv_color_t outline_color = lv_color_hex(0x1a237e);
                lv_opa_t outline_opa = 255;
                uint8_t mode = LV_LABEL_OUTLINE8_MODE_OPAQUE_FILL;
                lv_color_t fill_color = lv_color_hex(0xFFFDE7);
                lv_coord_t row_x = 40;
                lv_coord_t row_y = 280;
                uint32_t duration_ms = 550;
                uint32_t delay_per_char_ms = 70;
                const lv_scatter_char_start_t *starts = NULL;
                int32_t arc_rotate_angle_01 = 900;
                lv_coord_t arc_radius = 90;
                int16_t arc_center_deg = 90;
                uint16_t arc_spread_deg = 120;

                phandle->scatter_fly_handle = lv_label_outline8_scatter_fly_in_create(
                    parent, segs, segment_count,
                    0,                             /* max_width：0=单行不换行 */
                    offset_px, outline_color, outline_opa, mode, fill_color,
                    row_x, row_y,
                    duration_ms, delay_per_char_ms,
                    starts,
                    arc_rotate_angle_01,
                    arc_radius, arc_center_deg, arc_spread_deg);
            }
        }
    }
}

void divoom_lvgl_test_3dgif_eixt(void *handle)
{
    Pdivoom_lvgl_test_3dgif_ctrl phandle = (Pdivoom_lvgl_test_3dgif_ctrl)handle;
    if (phandle->label_shadow_handle) {
        lv_label_with_shadow_delete(phandle->label_shadow_handle);
        phandle->label_shadow_handle = NULL;
    }
    if (phandle->label_outline8_handle) {
        lv_label_with_outline8_delete(phandle->label_outline8_handle);
        phandle->label_outline8_handle = NULL;
    }
    if (phandle->label_outline8_handle_fill) {
        lv_label_with_outline8_delete(phandle->label_outline8_handle_fill);
        phandle->label_outline8_handle_fill = NULL;
    }
    if (phandle->scatter_fly_handle) {
        lv_label_outline8_scatter_fly_in_delete(phandle->scatter_fly_handle);
        phandle->scatter_fly_handle = NULL;
    }
    for (int i = 0; i < 5; i++) {
        if (phandle->fly_shadow_handles[i]) {
            lv_label_with_shadow_delete(phandle->fly_shadow_handles[i]);
            phandle->fly_shadow_handles[i] = NULL;
        }
    }
	lv_obj_del(phandle->main_obj);
    if (phandle->pseudo3d_handle) pseudo3d_img_delete(phandle->pseudo3d_handle);
    if (phandle->snap_buf) divoom_system_free(phandle->snap_buf);
	divoom_system_free(phandle->src_buf);
	divoom_system_free(phandle->dest_buf);
	divoom_system_free(phandle);
}
