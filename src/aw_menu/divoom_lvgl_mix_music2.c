#include "divoom.h"
#include "lvgl.h"
#include "divoom_light_app.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_mix_music2.h"
#include "middle/include/divoom_aw_pic_decode.h"
#include "divoom_pixel_format.h"

#define DIVOOM_LVGL_MIX_MUSIC2_UI_DIR               "133/"
#define DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT          17
#define DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT       8
#define DIVOOM_LVGL_MIX_MUSIC2_COMPOSITE_MAX_COUNT  8
#define DIVOOM_LVGL_MIX_MUSIC2_ANIM_TIME_MS         50
#define DIVOOM_LVGL_MIX_MUSIC2_STATE_TIME_MS        100
#define DIVOOM_LVGL_MIX_MUSIC2_ACTIVE_TIME_MS       3000
#define DIVOOM_LVGL_MIX_MUSIC2_FRAME_TIME_MS        120
#define DIVOOM_LVGL_MIX_MUSIC2_PIANO_SPEED          12
#define DIVOOM_LVGL_MIX_MUSIC2_PIANO_BOOST_STEP     4
#define DIVOOM_LVGL_MIX_MUSIC2_PIANO_BOOST_MAX      24
#define DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_SPEED       6
#define DIVOOM_LVGL_MIX_MUSIC2_IMAGE_PATH_MAX       96

typedef struct _divoom_lvgl_mix_music2_image_cache_
{
    lv_img_dsc_t dsc;
    divoom_aw_png_decode_buf_t decode;
    char path[DIVOOM_LVGL_MIX_MUSIC2_IMAGE_PATH_MAX];
}divoom_lvgl_mix_music2_image_cache;

typedef struct _divoom_lvgl_mix_music2_mask_rect_
{
    int16 x;
    int16 y;
    int16 width;
    int16 height;
}divoom_lvgl_mix_music2_mask_rect;

typedef struct _divoom_lvgl_mix_music2_handle_
{
    lv_obj_t *screen;
    lv_obj_t *key_btn;
    lv_obj_t *piano_segment[DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT];
    lv_obj_t *keyboard_key[DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT];
    lv_obj_t *keyboard_note[DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT];
    lv_obj_t *composite_base;
    lv_obj_t *rack_room_active;
    lv_obj_t *composite_inactive_mask;
    lv_obj_t *composite_active;
    lv_timer_t *anim_timer;
    lv_timer_t *state_timer;
    lv_style_t screen_style;
    divoom_light_app_mix_music2_state state;
    int16 piano_y[DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT];
    int16 keyboard_note_y[DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT];
    uint8 piano_rising[DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT];
    uint8 piano_done[DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT];
    uint8 keyboard_flying[DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT];
    uint8 piano_full;
    uint8 piano_boost;
    uint8 composite_mode;
    uint8 composite_item;
    uint8 composite_frame;
    uint8 composite_running;
    uint8 loaded_page;
    uint32 composite_start_tick;
    divoom_lvgl_mix_music2_image_cache piano_image;
    divoom_lvgl_mix_music2_image_cache keyboard_key_image[DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT];
    divoom_lvgl_mix_music2_image_cache keyboard_note_cache[DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT];
    divoom_lvgl_mix_music2_image_cache composite_base_image;
    divoom_lvgl_mix_music2_image_cache rack_room_image;
    divoom_lvgl_mix_music2_image_cache composite_active_image;
}divoom_lvgl_mix_music2_handle, *Pdivoom_lvgl_mix_music2_handle;

static const int16 divoom_lvgl_mix_music2_piano_x[DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT] =
{
    3, 31, 59, 87, 115, 143, 171, 199, 227, 255, 283, 311, 339, 367, 395, 423, 451
};

static const int16 divoom_lvgl_mix_music2_piano_rest_y[DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT] =
{
    451, 423, 395, 367, 339, 311, 283, 255, 227, 199, 171, 143, 115, 87, 59, 31, 3
};

static const int16 divoom_lvgl_mix_music2_keyboard_x[DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT] =
{
    12, 70, 128, 186, 244, 302, 360, 418
};

static const int16 divoom_lvgl_mix_music2_keyboard_note_x[DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT] =
{
    18, 76, 134, 192, 250, 308, 366, 424
};

static const uint16 divoom_lvgl_mix_music2_keyboard_height[DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT] =
{
    119, 109, 103, 98, 94, 88, 82, 76
};

static const char *divoom_lvgl_mix_music2_keyboard_idle[DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT] =
{
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_1_idle.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_2_idle.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_3_idle.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_4_idle.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_5_idle.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_6_idle.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_7_idle.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_8_idle.png"
};

static const char *divoom_lvgl_mix_music2_keyboard_active[DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT] =
{
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_1_active.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_2_active.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_3_active.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_4_active.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_5_active.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_6_active.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_7_active.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_8_active.png"
};

static const char *divoom_lvgl_mix_music2_keyboard_note_image[DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT] =
{
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_1_note.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_2_note.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_3_note.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_4_note.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_5_note.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_6_note.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_7_note.png",
    DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_keyboard_8_note.png"
};

/* MusicIndex 为零基索引：左下镲、左落地鼓、底鼓、右中鼓、右落地鼓、右上镲、左中鼓、左上镲。 */
static const uint8 divoom_lvgl_mix_music2_rack_asset[DIVOOM_LVGL_MIX_MUSIC2_COMPOSITE_MAX_COUNT] =
{
    6, 2, 1, 5, 3, 8, 4, 7
};

static const int16 divoom_lvgl_mix_music2_rack_x[DIVOOM_LVGL_MIX_MUSIC2_COMPOSITE_MAX_COUNT] =
{
    22, 66, 166, 250, 306, 354, 146, 30
};

static const int16 divoom_lvgl_mix_music2_rack_y[DIVOOM_LVGL_MIX_MUSIC2_COMPOSITE_MAX_COUNT] =
{
    206, 254, 246, 150, 238, 134, 150, 138
};

static const int16 divoom_lvgl_mix_music2_rack_width[DIVOOM_LVGL_MIX_MUSIC2_COMPOSITE_MAX_COUNT] =
{
    108, 104, 148, 84, 128, 116, 84, 100
};

static const int16 divoom_lvgl_mix_music2_rack_height[DIVOOM_LVGL_MIX_MUSIC2_COMPOSITE_MAX_COUNT] =
{
    40, 104, 144, 96, 148, 56, 96, 44
};

static const uint32 divoom_lvgl_mix_music2_rack_color[DIVOOM_LVGL_MIX_MUSIC2_COMPOSITE_MAX_COUNT] =
{
    0xd9ff3f, 0x8b5cff, 0xff3d98, 0x24d7ff,
    0x4d7cff, 0xff9f1c, 0x29ffb2, 0xffc83a
};

/* Bell/Violin/Maracas/Saxophone 的组件区域，顺序严格对应协议 MusicIndex。 */
static const divoom_lvgl_mix_music2_mask_rect
    divoom_lvgl_mix_music2_composite_mask[4][5] =
{
    {
        {156, 133, 168, 168}, {349, 50, 102, 105}, {44, 50, 93, 93},
        {30, 320, 126, 132}, {319, 286, 135, 168}
    },
    {
        {5, 102, 90, 273}, {100, 102, 90, 273}, {195, 102, 90, 273},
        {290, 102, 90, 273}, {385, 102, 90, 273}
    },
    {
        {44, 50, 132, 132}, {332, 82, 88, 84}, {177, 204, 126, 72},
        {64, 322, 100, 94}, {292, 324, 148, 88}
    },
    {
        {42, 37, 130, 168}, {234, 89, 204, 80}, {42, 321, 154, 100},
        {144, 229, 201, 45}, {300, 303, 118, 118}
    }
};

/* 资源编号仍按 MasterGo 图层保存，这里只做协议顺序到资源编号的映射。 */
static const uint8 divoom_lvgl_mix_music2_composite_asset[4][5] =
{
    {1, 5, 2, 4, 3},
    {1, 2, 3, 4, 5},
    {1, 2, 3, 4, 5},
    {1, 2, 4, 3, 5}
};

static void divoom_lvgl_mix_music2_detach_image(lv_obj_t *obj)
{
    if(obj == NULL || !lv_obj_is_valid(obj))
    {
        return;
    }
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_img_set_src(obj, NULL);
}

static void divoom_lvgl_mix_music2_release_image_cache(
    divoom_lvgl_mix_music2_image_cache *cache)
{
    if(cache == NULL)
    {
        return;
    }
    if(cache->dsc.data != NULL)
    {
        lv_img_cache_invalidate_src(&cache->dsc);
    }
    memset(&cache->dsc, 0, sizeof(cache->dsc));
    divoom_aw_png_decode_buf_release(&cache->decode);
    cache->path[0] = 0;
}

/*
 * PNG 不再交给 LVGL 文件解码器。每个可同时显示的图层只持有一块 RGB565A8
 * 缓冲，换图时直接覆盖复用，避免全局图片缓存随 Mixer/SetNew 切换持续增长。
 */
static uint8 divoom_lvgl_mix_music2_load_image(lv_obj_t *obj,
    divoom_lvgl_mix_music2_image_cache *cache, const char *relative_path)
{
    char full_path[160];
    uint8 *image_buf;
    uint8 was_hidden;
    int image_w = 0;
    int image_h = 0;

    if(obj == NULL || cache == NULL || relative_path == NULL)
    {
        return 0;
    }
    if(cache->dsc.data != NULL && strcmp(cache->path, relative_path) == 0)
    {
        lv_img_set_src(obj, &cache->dsc);
        return 1;
    }

    was_hidden = lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN) ? 1 : 0;
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_img_set_src(obj, NULL);
    if(cache->dsc.data != NULL)
    {
        lv_img_cache_invalidate_src(&cache->dsc);
    }
    memset(&cache->dsc, 0, sizeof(cache->dsc));
    cache->path[0] = 0;

#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
    divoom_menu_control_get_img_path(relative_path, full_path);
#else
    /* 本地文件读取接口不识别 LVGL 的 A: 盘符，真机必须使用实际文件系统路径。 */
    divoom_menu_control_get_img_path2(relative_path, full_path);
#endif
    image_buf = divoom_aw_decode_png_file_to_lvfmt_reuse(full_path, &image_w,
        &image_h, &cache->decode, LV_COLOR_FORMAT_RGB565A8);
    if(image_buf == NULL || image_w <= 0 || image_h <= 0)
    {
        divoom_print("mix music2 png decode fail: %s\n", full_path);
        return 0;
    }

    divoom_lv_img_dsc_init_fmt(&cache->dsc, (uint32)image_w, (uint32)image_h,
        LV_COLOR_FORMAT_RGB565A8, image_buf);
    if(cache->dsc.data_size == 0 || cache->dsc.data_size > cache->decode.out_buf_size)
    {
        memset(&cache->dsc, 0, sizeof(cache->dsc));
        divoom_print("mix music2 png buffer invalid: %s\n", full_path);
        return 0;
    }
    lv_img_set_src(obj, &cache->dsc);
    strncpy(cache->path, relative_path, sizeof(cache->path) - 1);
    cache->path[sizeof(cache->path) - 1] = 0;
    if(!was_hidden)
    {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
    return 1;
}

static void divoom_lvgl_mix_music2_release_page_images(
    Pdivoom_lvgl_mix_music2_handle handle)
{
    uint8 index;

    if(handle == NULL)
    {
        return;
    }
    for(index = 0; index < DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT; index ++)
    {
        divoom_lvgl_mix_music2_detach_image(handle->piano_segment[index]);
    }
    divoom_lvgl_mix_music2_release_image_cache(&handle->piano_image);

    for(index = 0; index < DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT; index ++)
    {
        divoom_lvgl_mix_music2_detach_image(handle->keyboard_key[index]);
        divoom_lvgl_mix_music2_detach_image(handle->keyboard_note[index]);
        divoom_lvgl_mix_music2_release_image_cache(&handle->keyboard_key_image[index]);
        divoom_lvgl_mix_music2_release_image_cache(&handle->keyboard_note_cache[index]);
    }

    divoom_lvgl_mix_music2_detach_image(handle->composite_base);
    divoom_lvgl_mix_music2_detach_image(handle->rack_room_active);
    divoom_lvgl_mix_music2_detach_image(handle->composite_active);
    divoom_lvgl_mix_music2_release_image_cache(&handle->composite_base_image);
    divoom_lvgl_mix_music2_release_image_cache(&handle->rack_room_image);
    divoom_lvgl_mix_music2_release_image_cache(&handle->composite_active_image);
    handle->loaded_page = 0;
}

static uint8 divoom_lvgl_mix_music2_normalize_page(uint8 page)
{
    if(page == 0)
    {
        return DIVOOM_LVGL_MIX_MUSIC2_PAGE_PIANO;
    }
    if(page > DIVOOM_LVGL_MIX_MUSIC2_PAGE_MAX)
    {
        return DIVOOM_LVGL_MIX_MUSIC2_PAGE_MAX;
    }
    return page;
}

static int8 divoom_lvgl_mix_music2_composite_index(uint8 page)
{
    switch(page)
    {
        case DIVOOM_LVGL_MIX_MUSIC2_PAGE_RACK:
        {
            return 0;
        }
        case DIVOOM_LVGL_MIX_MUSIC2_PAGE_BELL:
        {
            return 1;
        }
        case DIVOOM_LVGL_MIX_MUSIC2_PAGE_VIOLIN:
        {
            return 2;
        }
        case DIVOOM_LVGL_MIX_MUSIC2_PAGE_MARACAS:
        {
            return 3;
        }
        case DIVOOM_LVGL_MIX_MUSIC2_PAGE_SAXOPHONE:
        {
            return 4;
        }
        default:
        {
            return -1;
        }
    }
}

static const char *divoom_lvgl_mix_music2_composite_name(uint8 composite_index)
{
    static const char *name[] = {"rack", "bell", "violin", "maracas", "saxophone"};

    if(composite_index >= sizeof(name) / sizeof(name[0]))
    {
        return "rack";
    }
    return name[composite_index];
}

static uint8 divoom_lvgl_mix_music2_composite_item_count(uint8 composite_index)
{
    if(composite_index == 0)
    {
        return 8;
    }
    return 5;
}

static uint8 divoom_lvgl_mix_music2_composite_frame_count(uint8 composite_index, uint8 item)
{
    if(composite_index == 0)
    {
        return 3;
    }
    if(composite_index == 1 && item < 5 &&
        divoom_lvgl_mix_music2_composite_asset[0][item] <= 3)
    {
        return 3;
    }
    return 1;
}

static void divoom_lvgl_mix_music2_hide_all(Pdivoom_lvgl_mix_music2_handle handle)
{
    uint8 index;

    for(index = 0; index < DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT; index ++)
    {
        lv_obj_add_flag(handle->piano_segment[index], LV_OBJ_FLAG_HIDDEN);
    }
    for(index = 0; index < DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT; index ++)
    {
        lv_obj_add_flag(handle->keyboard_key[index], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(handle->keyboard_note[index], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_add_flag(handle->composite_base, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(handle->rack_room_active, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(handle->composite_inactive_mask, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(handle->composite_active, LV_OBJ_FLAG_HIDDEN);
}

static void divoom_lvgl_mix_music2_reset_piano(Pdivoom_lvgl_mix_music2_handle handle)
{
    uint8 index;

    handle->piano_full = 0;
    handle->piano_boost = 0;
    for(index = 0; index < DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT; index ++)
    {
        handle->piano_rising[index] = 0;
        handle->piano_done[index] = 0;
        handle->piano_y[index] = DIVOOM_SCREEN_HIGH;
        lv_obj_add_flag(handle->piano_segment[index], LV_OBJ_FLAG_HIDDEN);
    }
}

static void divoom_lvgl_mix_music2_show_piano_full(Pdivoom_lvgl_mix_music2_handle handle)
{
    uint8 index;

    handle->piano_full = 1;
    for(index = 0; index < DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT; index ++)
    {
        handle->piano_rising[index] = 0;
        lv_obj_set_pos(handle->piano_segment[index], divoom_lvgl_mix_music2_piano_x[index],
            divoom_lvgl_mix_music2_piano_rest_y[index]);
        lv_obj_clear_flag(handle->piano_segment[index], LV_OBJ_FLAG_HIDDEN);
    }
}

static void divoom_lvgl_mix_music2_start_piano(Pdivoom_lvgl_mix_music2_handle handle, uint8 item)
{
    if(item >= DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT)
    {
        item = DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT - 1;
    }
    if(handle->piano_full)
    {
        divoom_lvgl_mix_music2_reset_piano(handle);
    }
    if(handle->piano_rising[item])
    {
        if(handle->piano_boost < DIVOOM_LVGL_MIX_MUSIC2_PIANO_BOOST_MAX)
        {
            handle->piano_boost += DIVOOM_LVGL_MIX_MUSIC2_PIANO_BOOST_STEP;
        }
        return;
    }
    handle->piano_boost = 0;
    handle->piano_rising[item] = 1;
    handle->piano_y[item] = DIVOOM_SCREEN_HIGH;
    lv_obj_set_pos(handle->piano_segment[item], divoom_lvgl_mix_music2_piano_x[item],
        handle->piano_y[item]);
    lv_obj_clear_flag(handle->piano_segment[item], LV_OBJ_FLAG_HIDDEN);
}

static uint8 divoom_lvgl_mix_music2_set_keyboard_idle(Pdivoom_lvgl_mix_music2_handle handle,
    uint8 item)
{
    return divoom_lvgl_mix_music2_load_image(handle->keyboard_key[item],
        &handle->keyboard_key_image[item], divoom_lvgl_mix_music2_keyboard_idle[item]);
}

static void divoom_lvgl_mix_music2_start_keyboard(Pdivoom_lvgl_mix_music2_handle handle, uint8 item)
{
    if(item >= DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT)
    {
        item = DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT - 1;
    }
    if(!divoom_lvgl_mix_music2_load_image(handle->keyboard_key[item],
        &handle->keyboard_key_image[item], divoom_lvgl_mix_music2_keyboard_active[item]))
    {
        return;
    }
    if(!divoom_lvgl_mix_music2_load_image(handle->keyboard_note[item],
        &handle->keyboard_note_cache[item], divoom_lvgl_mix_music2_keyboard_note_image[item]))
    {
        divoom_lvgl_mix_music2_set_keyboard_idle(handle, item);
        return;
    }
    handle->keyboard_flying[item] = 1;
    handle->keyboard_note_y[item] = 142;
    lv_obj_set_pos(handle->keyboard_note[item], divoom_lvgl_mix_music2_keyboard_note_x[item],
        handle->keyboard_note_y[item]);
    lv_obj_clear_flag(handle->keyboard_note[item], LV_OBJ_FLAG_HIDDEN);
}

static void divoom_lvgl_mix_music2_show_composite_base(Pdivoom_lvgl_mix_music2_handle handle,
    uint8 composite_index)
{
    lv_obj_clear_flag(handle->composite_base, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(handle->rack_room_active, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(handle->composite_inactive_mask, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(handle->composite_active, LV_OBJ_FLAG_HIDDEN);
    handle->composite_running = 0;
    handle->composite_mode = composite_index;
}

static void divoom_lvgl_mix_music2_set_composite_inactive_hidden(
    Pdivoom_lvgl_mix_music2_handle handle, uint8 hidden)
{
    const divoom_lvgl_mix_music2_mask_rect *rect;

    if(!hidden || handle->composite_mode == 0 || handle->composite_mode > 4 ||
        handle->composite_item >= 5)
    {
        lv_obj_add_flag(handle->composite_inactive_mask, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    rect = &divoom_lvgl_mix_music2_composite_mask[handle->composite_mode - 1]
        [handle->composite_item];
    lv_obj_set_pos(handle->composite_inactive_mask, rect->x, rect->y);
    lv_obj_set_size(handle->composite_inactive_mask, rect->width, rect->height);
    lv_obj_clear_flag(handle->composite_inactive_mask, LV_OBJ_FLAG_HIDDEN);
}

static uint8 divoom_lvgl_mix_music2_apply_composite_frame(Pdivoom_lvgl_mix_music2_handle handle,
    uint8 frame)
{
    char active_path[64];

    if(handle->composite_mode == 0)
    {
        static const uint8 opacity[3] = {LV_OPA_COVER, 168, 90};
        uint8 item = handle->composite_item;
        uint8 asset = divoom_lvgl_mix_music2_rack_asset[item];

        snprintf(active_path, sizeof(active_path),
            DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_rack_%d.png", asset);
        if(!divoom_lvgl_mix_music2_load_image(handle->composite_active,
            &handle->composite_active_image, active_path))
        {
            return 0;
        }
        lv_obj_set_pos(handle->composite_active, divoom_lvgl_mix_music2_rack_x[item],
            divoom_lvgl_mix_music2_rack_y[item]);
        lv_obj_set_size(handle->composite_active, divoom_lvgl_mix_music2_rack_width[item],
            divoom_lvgl_mix_music2_rack_height[item]);
        /* 三帧共用同一张已带设计色的切图，只改变整体透明度。 */
        lv_obj_set_style_img_recolor_opa(handle->composite_active, LV_OPA_TRANSP, 0);
        lv_obj_set_style_img_opa(handle->composite_active, opacity[frame % 3], 0);
        lv_obj_set_style_img_recolor(handle->rack_room_active,
            lv_color_hex(divoom_lvgl_mix_music2_rack_color[item]), 0);
        lv_obj_set_style_img_recolor_opa(handle->rack_room_active, LV_OPA_COVER, 0);
        lv_obj_set_style_img_opa(handle->rack_room_active, opacity[frame % 3], 0);
        lv_obj_clear_flag(handle->rack_room_active, LV_OBJ_FLAG_HIDDEN);
        divoom_lvgl_mix_music2_set_composite_inactive_hidden(handle, 0);
    }
    else
    {
        const divoom_lvgl_mix_music2_mask_rect *rect;
        uint8 asset;

        lv_obj_add_flag(handle->rack_room_active, LV_OBJ_FLAG_HIDDEN);
        rect = &divoom_lvgl_mix_music2_composite_mask[handle->composite_mode - 1]
            [handle->composite_item];
        asset = divoom_lvgl_mix_music2_composite_asset[handle->composite_mode - 1]
            [handle->composite_item];
        snprintf(active_path, sizeof(active_path),
            DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_%s_%d_%d.png",
            divoom_lvgl_mix_music2_composite_name(handle->composite_mode),
            asset, frame + 1);
        if(!divoom_lvgl_mix_music2_load_image(handle->composite_active,
            &handle->composite_active_image, active_path))
        {
            return 0;
        }
        /* 裁剪激活层必须让对象跟随图片真实尺寸，否则会在旧的全屏区域内平铺。 */
        lv_img_set_size_mode(handle->composite_active, LV_IMG_SIZE_MODE_REAL);
        lv_obj_set_pos(handle->composite_active, rect->x, rect->y);
        lv_obj_set_size(handle->composite_active, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_update_layout(handle->composite_active);
        lv_obj_set_style_img_recolor_opa(handle->composite_active, LV_OPA_TRANSP, 0);
        lv_obj_set_style_img_opa(handle->composite_active, LV_OPA_COVER, 0);
        /* 激活切图解码到同一块 RGB565A8 缓冲，只保留当前组件尺寸的像素。 */
        divoom_lvgl_mix_music2_set_composite_inactive_hidden(handle, 1);
    }
    lv_obj_clear_flag(handle->composite_active, LV_OBJ_FLAG_HIDDEN);
    return 1;
}

static void divoom_lvgl_mix_music2_start_composite(Pdivoom_lvgl_mix_music2_handle handle,
    uint8 composite_index, uint8 item)
{
    uint8 item_count = divoom_lvgl_mix_music2_composite_item_count(composite_index);

    if(item >= item_count)
    {
        item = item_count - 1;
    }
    handle->composite_mode = composite_index;
    handle->composite_item = item;
    handle->composite_frame = 0xff;
    handle->composite_running = 1;
    handle->composite_start_tick = lv_tick_get();
    if(divoom_lvgl_mix_music2_apply_composite_frame(handle, 0))
    {
        handle->composite_frame = 0;
    }
}

static uint8 divoom_lvgl_mix_music2_prepare_page_images(
    Pdivoom_lvgl_mix_music2_handle handle, uint8 page)
{
    char image_path[64];
    int8 composite_index;
    uint8 index;

    if(page == DIVOOM_LVGL_MIX_MUSIC2_PAGE_PIANO)
    {
        if(!divoom_lvgl_mix_music2_load_image(handle->piano_segment[0],
            &handle->piano_image,
            DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_piano_segment.png"))
        {
            return 0;
        }
        for(index = 1; index < DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT; index ++)
        {
            lv_img_set_src(handle->piano_segment[index], &handle->piano_image.dsc);
        }
        return 1;
    }
    if(page == DIVOOM_LVGL_MIX_MUSIC2_PAGE_KEYBOARD)
    {
        for(index = 0; index < DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT; index ++)
        {
            if(!divoom_lvgl_mix_music2_set_keyboard_idle(handle, index))
            {
                return 0;
            }
        }
        return 1;
    }

    composite_index = divoom_lvgl_mix_music2_composite_index(page);
    if(composite_index < 0)
    {
        return 0;
    }
    snprintf(image_path, sizeof(image_path),
        DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_%s_base.png",
        divoom_lvgl_mix_music2_composite_name((uint8)composite_index));
    if(!divoom_lvgl_mix_music2_load_image(handle->composite_base,
        &handle->composite_base_image, image_path))
    {
        return 0;
    }
    if(composite_index == 0 &&
        !divoom_lvgl_mix_music2_load_image(handle->rack_room_active,
            &handle->rack_room_image,
            DIVOOM_LVGL_MIX_MUSIC2_UI_DIR "mix2_rack_room.png"))
    {
        return 0;
    }
    return 1;
}

static uint8 divoom_lvgl_mix_music2_show_page(Pdivoom_lvgl_mix_music2_handle handle,
    uint8 page, uint8 reset_page)
{
    int8 composite_index;
    uint8 index;

    divoom_lvgl_mix_music2_hide_all(handle);
    if(page != handle->loaded_page)
    {
        divoom_lvgl_mix_music2_release_page_images(handle);
        if(!divoom_lvgl_mix_music2_prepare_page_images(handle, page))
        {
            divoom_print("mix music2 page decode fail: %u\n", page);
            return 0;
        }
        handle->loaded_page = page;
    }
    if(page == DIVOOM_LVGL_MIX_MUSIC2_PAGE_PIANO)
    {
        if(reset_page)
        {
            divoom_lvgl_mix_music2_reset_piano(handle);
        }
        return 1;
    }
    if(page == DIVOOM_LVGL_MIX_MUSIC2_PAGE_KEYBOARD)
    {
        for(index = 0; index < DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT; index ++)
        {
            if(reset_page)
            {
                handle->keyboard_flying[index] = 0;
                divoom_lvgl_mix_music2_set_keyboard_idle(handle, index);
            }
            lv_obj_clear_flag(handle->keyboard_key[index], LV_OBJ_FLAG_HIDDEN);
            if(handle->keyboard_flying[index])
            {
                lv_obj_clear_flag(handle->keyboard_note[index], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return 1;
    }
    composite_index = divoom_lvgl_mix_music2_composite_index(page);
    if(composite_index >= 0)
    {
        divoom_lvgl_mix_music2_show_composite_base(handle, (uint8)composite_index);
        return 1;
    }
    return 0;
}

static void divoom_lvgl_mix_music2_sync_state(Pdivoom_lvgl_mix_music2_handle handle,
    const divoom_light_app_mix_music2_state *state)
{
    uint8 page = divoom_lvgl_mix_music2_normalize_page(state->effect);
    int8 composite_index;

    if(page != handle->loaded_page)
    {
        if(!divoom_lvgl_mix_music2_show_page(handle, page, 1))
        {
            return;
        }
    }
    handle->state = *state;
    if(state->icon == DIVOOM_LVGL_MIX_MUSIC2_ICON_IDLE)
    {
        (void)divoom_lvgl_mix_music2_show_page(handle, page, 1);
        return;
    }
    if(page == DIVOOM_LVGL_MIX_MUSIC2_PAGE_PIANO)
    {
        divoom_lvgl_mix_music2_start_piano(handle, state->icon);
        return;
    }
    if(page == DIVOOM_LVGL_MIX_MUSIC2_PAGE_KEYBOARD)
    {
        divoom_lvgl_mix_music2_start_keyboard(handle, state->icon);
        return;
    }
    composite_index = divoom_lvgl_mix_music2_composite_index(page);
    if(composite_index >= 0)
    {
        divoom_lvgl_mix_music2_start_composite(handle, (uint8)composite_index, state->icon);
    }
}

static void divoom_lvgl_mix_music2_state_timer_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_mix_music2_handle handle =
        (Pdivoom_lvgl_mix_music2_handle)lv_timer_get_user_data(timer);
    divoom_light_app_mix_music2_state state;

    if(handle == NULL)
    {
        return;
    }
    divoom_light_app_get_mix_music2_state(&state);
    if(state.generation != handle->state.generation || state.effect != handle->state.effect ||
        state.icon != handle->state.icon || state.activation_count != handle->state.activation_count)
    {
        divoom_lvgl_mix_music2_sync_state(handle, &state);
    }
}

static void divoom_lvgl_mix_music2_anim_timer_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_mix_music2_handle handle =
        (Pdivoom_lvgl_mix_music2_handle)lv_timer_get_user_data(timer);
    uint8 page;
    uint8 index;

    if(handle == NULL)
    {
        return;
    }
    page = divoom_lvgl_mix_music2_normalize_page(handle->state.effect);
    if(page == DIVOOM_LVGL_MIX_MUSIC2_PAGE_PIANO && !handle->piano_full)
    {
        uint8 done_count = 0;
        int16 speed = DIVOOM_LVGL_MIX_MUSIC2_PIANO_SPEED + handle->piano_boost;

        for(index = 0; index < DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT; index ++)
        {
            if(handle->piano_rising[index])
            {
                handle->piano_y[index] -= speed;
                lv_obj_set_y(handle->piano_segment[index], handle->piano_y[index]);
                if(handle->piano_y[index] + 26 <= 0)
                {
                    handle->piano_rising[index] = 0;
                    handle->piano_done[index] = 1;
                    lv_obj_add_flag(handle->piano_segment[index], LV_OBJ_FLAG_HIDDEN);
                }
            }
            if(handle->piano_done[index])
            {
                done_count ++;
            }
        }
        if(done_count == DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT)
        {
            divoom_lvgl_mix_music2_show_piano_full(handle);
        }
    }
    if(page == DIVOOM_LVGL_MIX_MUSIC2_PAGE_KEYBOARD)
    {
        for(index = 0; index < DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT; index ++)
        {
            if(handle->keyboard_flying[index])
            {
                handle->keyboard_note_y[index] -= DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_SPEED;
                lv_obj_set_y(handle->keyboard_note[index], handle->keyboard_note_y[index]);
                if(handle->keyboard_note_y[index] + 40 <= 0)
                {
                    handle->keyboard_flying[index] = 0;
                    lv_obj_add_flag(handle->keyboard_note[index], LV_OBJ_FLAG_HIDDEN);
                    divoom_lvgl_mix_music2_set_keyboard_idle(handle, index);
                }
            }
        }
    }
    if(handle->composite_running)
    {
        uint32 elapsed = lv_tick_elaps(handle->composite_start_tick);
        uint8 frame_count = divoom_lvgl_mix_music2_composite_frame_count(
            handle->composite_mode, handle->composite_item);
        uint8 frame = (uint8)((elapsed / DIVOOM_LVGL_MIX_MUSIC2_FRAME_TIME_MS) % frame_count);

        if(elapsed >= DIVOOM_LVGL_MIX_MUSIC2_ACTIVE_TIME_MS)
        {
            handle->composite_running = 0;
            lv_obj_add_flag(handle->rack_room_active, LV_OBJ_FLAG_HIDDEN);
            divoom_lvgl_mix_music2_set_composite_inactive_hidden(handle, 0);
            lv_obj_add_flag(handle->composite_active, LV_OBJ_FLAG_HIDDEN);
        }
        else if(frame != handle->composite_frame)
        {
            if(divoom_lvgl_mix_music2_apply_composite_frame(handle, frame))
            {
                handle->composite_frame = frame;
            }
        }
    }
}

static void divoom_lvgl_mix_music2_init_styles(Pdivoom_lvgl_mix_music2_handle handle)
{
    lv_style_init(&handle->screen_style);
    lv_style_set_bg_color(&handle->screen_style, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&handle->screen_style, LV_OPA_COVER);
    lv_style_set_border_width(&handle->screen_style, 0);
    lv_style_set_pad_all(&handle->screen_style, 0);
    lv_style_set_radius(&handle->screen_style, 0);
}

static void divoom_lvgl_mix_music2_key_event_cb(lv_event_t *event)
{
    char *key_info;

    if(event == NULL || lv_event_get_code(event) != LV_EVENT_KEY)
    {
        return;
    }
    key_info = (char *)lv_event_get_param(event);
    if(key_info != NULL && key_info[0] == DIVOOM_LVGL_KEY_ESC)
    {
        /* Mixer/SetNew 是全屏模式，ESC 统一退回进入该模式前的页面。 */
        divoom_menu_control_return_pre();
    }
}

void *divoom_lvgl_mix_music2_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_mix_music2_handle handle;
    uint8 index;

    (void)user_data;
    (void)menu_id;
    handle = (Pdivoom_lvgl_mix_music2_handle)divoom_system_malloc(sizeof(divoom_lvgl_mix_music2_handle));
    if(handle == NULL)
    {
        return NULL;
    }

    divoom_lvgl_mix_music2_init_styles(handle);
    handle->screen = lv_obj_create(NULL);
    lv_obj_add_style(handle->screen, &handle->screen_style, 0);
    lv_obj_set_size(handle->screen, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_clear_flag(handle->screen, LV_OBJ_FLAG_SCROLLABLE);

    /* 透明按钮只负责接收实体按键；必须使用 lv_btn 才能可靠收到 LV_EVENT_KEY。 */
    handle->key_btn = lv_btn_create(handle->screen);
    lv_obj_remove_style_all(handle->key_btn);
    lv_obj_set_pos(handle->key_btn, 0, 0);
    lv_obj_set_size(handle->key_btn, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_style_bg_opa(handle->key_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(handle->key_btn, 0, 0);
    lv_obj_set_style_outline_width(handle->key_btn, 0, 0);
    lv_obj_set_style_shadow_width(handle->key_btn, 0, 0);
    lv_obj_set_style_pad_all(handle->key_btn, 0, 0);
    lv_obj_clear_flag(handle->key_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(handle->key_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(handle->key_btn, divoom_lvgl_mix_music2_key_event_cb,
        LV_EVENT_KEY, handle);

    for(index = 0; index < DIVOOM_LVGL_MIX_MUSIC2_PIANO_COUNT; index ++)
    {
        handle->piano_segment[index] = lv_img_create(handle->screen);
        lv_obj_set_size(handle->piano_segment[index], 26, 26);
        lv_obj_add_flag(handle->piano_segment[index], LV_OBJ_FLAG_HIDDEN);
    }
    for(index = 0; index < DIVOOM_LVGL_MIX_MUSIC2_KEYBOARD_COUNT; index ++)
    {
        handle->keyboard_key[index] = lv_img_create(handle->screen);
        lv_obj_set_pos(handle->keyboard_key[index], divoom_lvgl_mix_music2_keyboard_x[index], 182);
        lv_obj_set_size(handle->keyboard_key[index], 52,
            divoom_lvgl_mix_music2_keyboard_height[index]);
        lv_obj_add_flag(handle->keyboard_key[index], LV_OBJ_FLAG_HIDDEN);

        handle->keyboard_note[index] = lv_img_create(handle->screen);
        lv_obj_set_size(handle->keyboard_note[index], 40, 40);
        lv_obj_add_flag(handle->keyboard_note[index], LV_OBJ_FLAG_HIDDEN);
        handle->keyboard_flying[index] = 0;
    }

    handle->composite_base = lv_img_create(handle->screen);
    lv_obj_set_pos(handle->composite_base, 0, 0);
    lv_obj_set_size(handle->composite_base, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_add_flag(handle->composite_base, LV_OBJ_FLAG_HIDDEN);
    handle->rack_room_active = lv_img_create(handle->screen);
    lv_obj_set_pos(handle->rack_room_active, 0, 0);
    lv_obj_set_size(handle->rack_room_active, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_add_flag(handle->rack_room_active, LV_OBJ_FLAG_HIDDEN);
    handle->composite_inactive_mask = lv_obj_create(handle->screen);
    lv_obj_remove_style_all(handle->composite_inactive_mask);
    lv_obj_set_style_bg_color(handle->composite_inactive_mask, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(handle->composite_inactive_mask, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(handle->composite_inactive_mask, 0, 0);
    lv_obj_set_style_radius(handle->composite_inactive_mask, 0, 0);
    lv_obj_set_style_pad_all(handle->composite_inactive_mask, 0, 0);
    lv_obj_clear_flag(handle->composite_inactive_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(handle->composite_inactive_mask, LV_OBJ_FLAG_HIDDEN);
    handle->composite_active = lv_img_create(handle->screen);
    lv_obj_set_pos(handle->composite_active, 0, 0);
    lv_obj_set_size(handle->composite_active, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_img_set_size_mode(handle->composite_active, LV_IMG_SIZE_MODE_REAL);
    lv_obj_add_flag(handle->composite_active, LV_OBJ_FLAG_HIDDEN);

    {
        divoom_light_app_mix_music2_state initial_state;

        divoom_light_app_get_mix_music2_state(&initial_state);
        handle->state.effect = 0;
        handle->state.icon = DIVOOM_LVGL_MIX_MUSIC2_ICON_IDLE;
        handle->state.activation_count = 0;
        handle->state.generation = initial_state.generation - 1;
        divoom_lvgl_mix_music2_reset_piano(handle);
        divoom_lvgl_mix_music2_sync_state(handle, &initial_state);
    }

    handle->anim_timer = lv_timer_create(divoom_lvgl_mix_music2_anim_timer_cb,
        DIVOOM_LVGL_MIX_MUSIC2_ANIM_TIME_MS, handle);
    handle->state_timer = lv_timer_create(divoom_lvgl_mix_music2_state_timer_cb,
        DIVOOM_LVGL_MIX_MUSIC2_STATE_TIME_MS, handle);
    return handle;
}

void divoom_lvgl_mix_music2_exit(void *handle_data)
{
    Pdivoom_lvgl_mix_music2_handle handle = (Pdivoom_lvgl_mix_music2_handle)handle_data;

    if(handle == NULL)
    {
        return;
    }
    if(handle->anim_timer != NULL)
    {
        lv_timer_del(handle->anim_timer);
        handle->anim_timer = NULL;
    }
    if(handle->state_timer != NULL)
    {
        lv_timer_del(handle->state_timer);
        handle->state_timer = NULL;
    }
    if(handle->screen != NULL)
    {
        divoom_lvgl_mix_music2_release_page_images(handle);
        lv_obj_del(handle->screen);
        handle->screen = NULL;
    }
    lv_style_reset(&handle->screen_style);
    divoom_system_free(handle);
}

void divoom_lvgl_mix_music2_disp(void *handle_data, void *active_obj)
{
    Pdivoom_lvgl_mix_music2_handle handle = (Pdivoom_lvgl_mix_music2_handle)handle_data;
    lv_group_t *group;

    (void)active_obj;
    if(handle == NULL || handle->screen == NULL)
    {
        return;
    }
    lv_scr_load(handle->screen);
    group = lv_group_get_default();
    if(group != NULL && handle->key_btn != NULL)
    {
        lv_group_remove_all_objs(group);
        lv_group_add_obj(group, handle->key_btn);
        lv_group_focus_obj(handle->key_btn);
    }
    if(handle->anim_timer != NULL)
    {
        lv_timer_resume(handle->anim_timer);
    }
    if(handle->state_timer != NULL)
    {
        lv_timer_resume(handle->state_timer);
    }
}

void divoom_lvgl_mix_music2_pause(void *handle_data)
{
    Pdivoom_lvgl_mix_music2_handle handle = (Pdivoom_lvgl_mix_music2_handle)handle_data;

    if(handle == NULL)
    {
        return;
    }
    if(handle->anim_timer != NULL)
    {
        lv_timer_pause(handle->anim_timer);
    }
    if(handle->state_timer != NULL)
    {
        lv_timer_pause(handle->state_timer);
    }
}
