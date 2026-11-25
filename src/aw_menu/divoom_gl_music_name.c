#include "divoom.h"
#include <math.h>
#include "lvgl.h"
#include "divoom_menu_control.h"

#ifdef DIVOOM_USE_RK_IC

#include <lvgl/lv_drivers/sdl/gl/gl.h>

#define ALIGN(x, a) (((x) + (a - 1)) & ~(a - 1))

typedef struct
{
    lv_draw_label_dsc_t label_dsc;
    lv_img_dsc_t *img_dsc;
    lv_obj_t *canvas;
} divoom_gl_music_name_label_canvas;

#define DIVOOM_GL_MUSIC_NAME_MAX_WORD_NUM 5
typedef struct _divoom_gl_music_name_item_pos
{
    int x;
    int y;
    int z;
    int w;
    int h;
}divoom_gl_music_name_item_pos, *Pdivoom_gl_music_name_item_pos;

typedef struct  _divoom_gl_music_name_ctrl
{
    int  active_word_num;   //有几段激活的文字
    lv_anim_t image_anim;
    SDL_Rect view;
    SDL_Rect screen;
    lv_gl_obj_t *full_screen_obj;
    lv_gl_obj_t **obj_fold;
    uint8    **image_buf;
    Pdivoom_gl_music_name_item_pos pos_info;
}divoom_gl_music_name_ctrl, *Pdivoom_gl_music_name_ctrl;

static Pdivoom_gl_music_name_ctrl pdivoom_gl_music_name_handle = NULL;

static lv_gl_obj_t *divoom_gl_music_name_utf8_to_obj(divoom_gl_music_name_label_canvas *lc, char *text, int *width, int *high, uint8 **image_buf)
{
    lv_img_dsc_t img_dsc = {0};
    lv_gl_obj_t *obj;
    lv_gl_tex_t *tex;
    lv_gl_img_t img;
    lv_coord_t data_size;
    lv_point_t size;

    lv_txt_get_size(&size, text, lc->label_dsc.font,
        0, 0, LV_COORD_MAX, 0);

    data_size = lv_img_buf_get_img_size(ALIGN(size.x, 16),
            ALIGN(size.y, 16), LV_COLOR_FORMAT_RGB565A8);
    
DIVOOM_LV_IMG_HEADER_INIT(&    img_dsc.header);
    img_dsc.header.w = ALIGN(size.x, 16);
    img_dsc.header.h = ALIGN(size.y, 16);
    img_dsc.header.cf = LV_COLOR_FORMAT_RGB565A8;
    img_dsc.data = divoom_system_malloc(data_size);
    img_dsc.data_size = data_size;
    *image_buf = (uint8 *)img_dsc.data;
        
    lv_canvas_set_buffer(lc->canvas, (void *)img_dsc.data,
        ALIGN(size.x, 16), ALIGN(size.y, 16),
        LV_COLOR_FORMAT_RGB565A8);
    
    lv_canvas_draw_text(lc->canvas,
        ((ALIGN(size.x, 16) - size.x) / 2),
        ((ALIGN(size.y, 16) - size.y) / 2),
        ALIGN(size.x, 16), &lc->label_dsc, text);

    img.pixels = img_dsc.data;
    img.format = LV_GL_FMT_BGRA;
    img.w = ALIGN(size.x, 16);
    img.h = ALIGN(size.y, 16);
    divoom_print("image info: %d, %d\n", img.w, img.h);
    obj = lv_gl_obj_create(img.w, img.h);
    tex = lv_gl_tex_create(GL_TEX_TYPE_2D, 0, 0, &img);
    lv_gl_obj_bind_tex(obj, tex);
    *width = img.w;
    *high = img.h;

    return obj;
}

void divoom_gl_music_name_deinit_gl(Pdivoom_gl_music_name_ctrl phandle)
{
    int i;

    lv_gl_set_render_cb(NULL);
    if(phandle->full_screen_obj)
    {
        lv_gl_obj_del(phandle->full_screen_obj);
        phandle->full_screen_obj = NULL;
    }
    if(phandle->obj_fold)
    {
        for(i = 0; i < phandle->active_word_num; i++)
        {
            if(phandle->obj_fold[i] != NULL)
            {
                lv_gl_obj_del(phandle->obj_fold[i]);
                phandle->obj_fold[i] = NULL;
            }
        }
        divoom_system_free(phandle->obj_fold);
        phandle->obj_fold = NULL;
    }
    if(phandle->image_buf)
    {
        for(i = 0; i < phandle->active_word_num; i++)
        {
            if(phandle->image_buf[i] != NULL)
            {
                divoom_system_free(phandle->image_buf[i]);
                phandle->image_buf[i] = NULL;
            }
        }
        divoom_system_free(phandle->image_buf);
        phandle->image_buf = NULL;
    }
    if(phandle->pos_info)
    {
        divoom_system_free(phandle->pos_info);
        phandle->pos_info = NULL;
    }
}

void *divoom_gl_music_name_init(void)
{
    Pdivoom_gl_music_name_ctrl phandle = divoom_system_malloc(sizeof(divoom_gl_music_name_ctrl));

    phandle->screen.x = 0;
    phandle->screen.y = 0;
    phandle->screen.w  = DIVOOM_SCREEN_WIDTH;
    phandle->screen.h  = DIVOOM_SCREEN_HIGH;

    phandle->view.x = 0;
    phandle->view.y = 0;
    phandle->view.w = DIVOOM_SCREEN_WIDTH;
    phandle->view.h = DIVOOM_SCREEN_HIGH;
    lv_gl_obj_set_viewport(NULL, &phandle->view);
    lv_anim_init(&phandle->image_anim);

    phandle->active_word_num = 0;
    pdivoom_gl_music_name_handle = phandle;
    return phandle;
}


static void divoom_gl_music_name_gl_finish_callback(struct _lv_anim_t* a)
{
    Pdivoom_gl_music_name_ctrl phandle = (Pdivoom_gl_music_name_ctrl)a->user_data;
    
}

//所有文字要动起来
void divoom_gl_music_name_anim_callback(void *var, int32_t v)
{
    int i;
    Pdivoom_gl_music_name_ctrl phandle = (Pdivoom_gl_music_name_ctrl)var;
    
    #if 1
    for(i = 0; i < phandle->active_word_num; i++)
    {
        phandle->obj_fold[i]->base.r.y = v *3;
        lv_gl_obj_move(phandle->obj_fold[i], phandle->full_screen_obj);
        phandle->obj_fold[i]->p[0].z = sin(M_PI * 2.0 * v /360);
        phandle->obj_fold[i]->p[1].z = sin(M_PI * 2.0 * v /360);
        phandle->obj_fold[i]->p[2].z = sin(-M_PI * 2.0 * v /360);
        phandle->obj_fold[i]->p[3].z = sin(-M_PI * 2.0 * v /360);
        lv_gl_obj_update_vao(phandle->obj_fold[i]);
    }
    lv_obj_invalidate(lv_layer_top());
    #endif
}

static divoom_gl_music_name_label_canvas *divoom_gl_music_name_create_canvas(lv_color_t color, lv_font_t *font)
{
    divoom_gl_music_name_label_canvas *lc;

    lc = lv_mem_alloc(sizeof(divoom_gl_music_name_label_canvas));
    if (!lc)
        return NULL;
    lv_memset_00(lc, sizeof(divoom_gl_music_name_label_canvas));

    lv_draw_label_dsc_init(&lc->label_dsc);
    lc->label_dsc.color = color;
    lc->label_dsc.font = font;
    lc->label_dsc.sel_bg_color = lv_color_hex(0x000000);  // 蓝色
    lc->label_dsc.opa = LV_OPA_COVER;  // 背景不透明
    lc->canvas = lv_canvas_create(NULL);

    return lc;
}

void divoom_gl_music_name_destory_canvas(divoom_gl_music_name_label_canvas *label_canvas)
{
    lv_obj_del(label_canvas->canvas);
    lv_img_buf_free(label_canvas->img_dsc);
    divoom_system_free(label_canvas);
}

void divoom_gl_music_name_create_gl_handle(Pdivoom_gl_music_name_ctrl phandle, void *font_handle,char *music_name)
{
    uint32_t byte_id = 0;
    uint32_t last_byte_id = 0;
    uint32_t word;
    uint32_t words = 0;
    char buf[32];
    
    while (1)
    {
        word = _lv_txt_encoded_next(music_name, &byte_id);
        if (!word)
            break;
        words++;
    }
    
    phandle->obj_fold = divoom_system_malloc(words * sizeof(lv_gl_obj_t *));
    phandle->image_buf   = divoom_system_malloc(words * sizeof(void *));
    phandle->pos_info = divoom_system_malloc(words * sizeof(divoom_gl_music_name_item_pos));
    phandle->active_word_num = words;
    
    phandle->full_screen_obj = lv_gl_obj_create(DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    phandle->full_screen_obj->scale.x = 1;
    phandle->full_screen_obj->scale.y = 1;
    void *tex_fb = lv_gl_tex_create(GL_TEX_TYPE_2D, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, NULL);
    lv_gl_obj_bind_tex(phandle->full_screen_obj, tex_fb);
    divoom_gl_music_name_label_canvas *lc = divoom_gl_music_name_create_canvas(lv_palette_main(LV_PALETTE_ORANGE), font_handle);
    byte_id = 0;
    for (int j = 0; j < words; j++)
    {
        last_byte_id = byte_id;
        _lv_txt_encoded_next(music_name, &byte_id);
        memcpy(buf, music_name + last_byte_id,
            byte_id - last_byte_id);
        buf[byte_id - last_byte_id] = 0;
        phandle->obj_fold[j] = divoom_gl_music_name_utf8_to_obj(lc, buf, &phandle->pos_info[j].w, &phandle->pos_info[j].h, &phandle->image_buf[j]);
        lv_gl_obj_resize(phandle->obj_fold[j], phandle->full_screen_obj);
    }
    phandle->pos_info[0].x = 0;
    phandle->obj_fold[0]->tp.x = 0;
    for (int j = 1; j < words; j++)
    {
        phandle->pos_info[j].x = phandle->pos_info[j - 1].x + phandle->pos_info[j - 1].w;
        phandle->obj_fold[j]->base.r.x = phandle->pos_info[j].x;
        phandle->obj_fold[j]->base.r.y = 0;
        lv_gl_obj_move(phandle->obj_fold[j], phandle->full_screen_obj);
        lv_gl_obj_update_vao(phandle->obj_fold[j]);
        divoom_print("pos: %d, %d\n", j , phandle->obj_fold[j]->tp.x);
    }
    divoom_gl_music_name_destory_canvas(lc);
    
}

static void divoom_gl_music_name_gl_render(void)
{
    int i;
    /* render to screen */
    lv_gl_set_fb(pdivoom_gl_music_name_handle->full_screen_obj);
    lv_gl_tex_clear(pdivoom_gl_music_name_handle->full_screen_obj->base.tex, 0.0, 0.0, 0.0, 0);
    for(i = 0; i < pdivoom_gl_music_name_handle->active_word_num; i++)
    {
        lv_gl_obj_render(pdivoom_gl_music_name_handle->obj_fold[i]);
    }
    lv_gl_set_fb(NULL);
    lv_gl_obj_render(pdivoom_gl_music_name_handle->full_screen_obj);
}

void divoom_gl_music_name_update(void *handle, void * font_handle, char *music_name)
{
    int i;
    Pdivoom_gl_music_name_ctrl phandle = (Pdivoom_gl_music_name_ctrl)handle;

    if(handle == NULL)
    {

        divoom_print("aj test!\n");
        return;
    }

    if(strlen(music_name) == 0)
    {
        divoom_print("aj test!\n");
        return;
    }
    
    lv_gl_obj_set_viewport(NULL, &phandle->view);
    divoom_gl_music_name_deinit_gl(phandle);
    divoom_gl_music_name_create_gl_handle(phandle, font_handle, music_name);
    
    lv_anim_set_var(&phandle->image_anim, phandle);
    lv_anim_path_linear(&phandle->image_anim);
    lv_anim_set_path_cb(&phandle->image_anim, lv_anim_path_linear);
    lv_anim_set_exec_cb(&phandle->image_anim, (lv_anim_exec_xcb_t) divoom_gl_music_name_anim_callback);
    lv_anim_set_time(&phandle->image_anim, 2000);
    lv_anim_set_values(&phandle->image_anim, 0, 360);
    lv_anim_set_deleted_cb(&phandle->image_anim, divoom_gl_music_name_gl_finish_callback);

    lv_gl_set_render_cb(divoom_gl_music_name_gl_render);
    
    lv_anim_start(&phandle->image_anim);   

}

void divoom_gl_music_name_pause(void *handle)
{
    if(handle == NULL)
    {
        return;
    }
    lv_anim_del(handle, NULL);
    lv_gl_set_render_cb(NULL);
    lv_gl_obj_set_crop(NULL, NULL, 0);
    glFinish();
}


void divoom_gl_music_name_exit(void *handle)
{
    Pdivoom_gl_music_name_ctrl phandle = (Pdivoom_gl_music_name_ctrl)handle;

    if(handle == NULL)
    {
        return;
    }

    pdivoom_gl_music_name_handle = NULL;
    divoom_gl_music_name_pause(handle);
    divoom_gl_music_name_deinit_gl(phandle);
    divoom_system_free(phandle);
}
#endif


