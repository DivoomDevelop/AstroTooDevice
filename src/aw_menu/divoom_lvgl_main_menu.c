#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_photo_slide_effect.h"
#include "divoom_lvgl_dial_menu.h"
#include "divoom_aw_pic_decode.h"
#include "divoom_pixel_format.h"
#define DIVOOM_LVGL_MAIN_MENU_BTN_NUM 10

#define DIVOOM_MAIN_MENU_MID_IMG_START_Y 495
#define DIVOOM_MAIN_MENU_IMG_START_X 79
#define DIVOOM_MAIN_MENU_UP_IMG_START_Y      (215 - 52)
#define DIVOOM_MAIN_MENU_DOWN_IMG_START_Y       (879 -52)
#define DIVOOM_MAIN_MENU_MODE_STRING_START_X        101
#define DIVOOM_MAIN_MENU_MODE_ACTIVE_START_X 95
#define DIVOOM_MAIN_MENU_IMAGE_OPA_VALUE 128

#define DIVOOM_MAIN_MENU_MOVE_SPEED  300
#define DIVOOM_MAIN_MENU_ZOOM_VALUE	162
#define DIVOOM_MAIN_MENU_ZOOM_DIFFER_VALUE (256 - DIVOOM_MAIN_MENU_ZOOM_VALUE)
#define DIVOOM_ZOOM_FONT_SIZE 56

#define DIVOOM_TEXT_MID_X	38
#define DIVOOM_TEXT_MID_Y	97
#define DIVOOM_TEXT_UP_DOWN_X_OFFSET	92
#define DIVOOM_TEXT_UP_DOWN_Y_OFFSET	25
#define DIVOOM_TEXT_UP_DOWN_X	(DIVOOM_TEXT_MID_X + DIVOOM_TEXT_UP_DOWN_X_OFFSET)
#define DIVOOM_TEXT_UP_DOWN_Y	(DIVOOM_TEXT_MID_Y + DIVOOM_TEXT_UP_DOWN_Y_OFFSET)
#define DIVOOM_MAIN_MENU_IMAGE_BASE_PATH  "A:/usr/share/divoom_app/ui/"

typedef struct _divoom_lvgl_main_menu_img_dsc_t{
	int func_id;
	lv_img_dsc_t *img_cache;
}divoom_lvgl_main_menu_img_dsc_t,*Pdivoom_lvgl_main_menu_img_dsc_t;

typedef struct _divoom_lvgl_main_menu_ctrl
{
    int menu_id;
	void *main_obj;
	void *cout_obj;
	void *mid_obj;
	void *mid_label;
	void *up_obj;
	void *up_label;
	void *down_obj;
	void *down_label;
    int divoom_main_menu_index;
	lv_style_t backgroud_type;
	lv_anim_t up_anim;
	lv_anim_t down_anim;
	lv_anim_t mid_anim;
	int run_flag;
	void *sem;
	void *img_cache_queue;
	int func_id;
	int cur_func_id;
}divoom_lvgl_main_menu_ctrl, *Pdivoom_lvgl_main_menu_ctrl;

char *divoom_main_img_list[] = {
	"1/1_dial.png",
	"1/1_mato.png",
	"1/1_slide_effect.png", //幻灯片
	"1/1_alarm.png",
	"1/1_album_manage.png", //相簿管理
	"1/1_tool.png",
	"1/1_setting.png",
};

char *divoom_main_mode_str[] = {
	"Dial",
	"Pomodoro",
	"SlideEffect",
	"Alarm",
	"AlbumManage",
	"Tool",
	"Setting",
};

const int divoom_main_next_menu_id[] =
{
	DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW,
	DIVOOM_MENU_CONTROL_ID_POMODORO_MENU,
	DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT,
	DIVOOM_MENU_CONTROL_ID_ALARM_LIST_MENU,
	DIVOOM_MENU_CONTROL_ID_SELECT_ALBUM_PLAY,
	DIVOOM_MENU_CONTROL_ID_TOOL,
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU,
};

void divoom_lvgl_main_menu_set_img_src(void *handle, void *obj, int func_id);
int divoom_lvgl_main_menu_queue_compare(void *item1, void *item2);
int divoom_lvgl_main_menu_cache_png_image(int func_id, lv_img_dsc_t *img_cache);

char *divoom_lvgl_main_menu_get_mode_pic_info(int index)
{
    int num = sizeof(divoom_main_img_list)/sizeof(divoom_main_img_list[0]);

    index = index%num;
    return divoom_main_img_list[index];
}

//这个动画是中间的变到上面
void divoom_lvgl_main_menu_mid_next_anim_cb(void * a, int32_t v) //中间移到上面
{
    lv_obj_set_y(a, DIVOOM_MAIN_MENU_MID_IMG_START_Y + (DIVOOM_MAIN_MENU_UP_IMG_START_Y - DIVOOM_MAIN_MENU_MID_IMG_START_Y)*v/100);
    lv_img_set_zoom(a, DIVOOM_MAIN_MENU_ZOOM_VALUE + DIVOOM_MAIN_MENU_ZOOM_DIFFER_VALUE *(100-v)/100);

	lv_obj_t * label = lv_obj_get_child(a, 0);  // 获取图片的第一个子对象，即文本框
	int zoom = lv_img_get_zoom(a);  // 获取当前图片的缩放比例
	int font_size = zoom*DIVOOM_ZOOM_FONT_SIZE/256;

	/* 根据缩放比例调整字体大小 */
	lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(font_size, DIVOOM_MENU_CONTROL_ID_MAIN), 0);
	lv_obj_set_pos(label, DIVOOM_TEXT_MID_X + DIVOOM_TEXT_UP_DOWN_X_OFFSET*v/100, DIVOOM_TEXT_MID_Y + DIVOOM_TEXT_UP_DOWN_Y_OFFSET*v/100);	
}

void divoom_lvgl_main_menu_up_next_anim_cb(void * a, int32_t v) //上面移到下面
{
//	lv_obj_set_y(a, 2*DIVOOM_MAIN_MENU_DOWN_IMG_START_Y - DIVOOM_MAIN_MENU_MID_IMG_START_Y - (DIVOOM_MAIN_MENU_DOWN_IMG_START_Y-DIVOOM_MAIN_MENU_MID_IMG_START_Y) * v/100);
    lv_obj_set_y(a, DIVOOM_MAIN_MENU_DOWN_IMG_START_Y);
    lv_img_set_zoom(a, DIVOOM_MAIN_MENU_ZOOM_VALUE *v/100);

	lv_obj_t * label = lv_obj_get_child(a, 0); 
	int zoom = lv_img_get_zoom(a);  
	int font_size = zoom*DIVOOM_ZOOM_FONT_SIZE/256;
	if (font_size == 0)
		font_size = 1;
	/* 根据缩放比例调整字体大小 */
	lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(font_size, DIVOOM_MENU_CONTROL_ID_MAIN), 0);
	lv_obj_set_pos(label, DIVOOM_TEXT_MID_X + DIVOOM_TEXT_UP_DOWN_X_OFFSET + (100 - v)*2, DIVOOM_TEXT_MID_Y + DIVOOM_TEXT_UP_DOWN_Y_OFFSET);	
}

void divoom_lvgl_main_menu_down_next_anim_cb(void * a, int32_t v) //下面移到中间
{
    lv_obj_set_y(a, DIVOOM_MAIN_MENU_DOWN_IMG_START_Y - (DIVOOM_MAIN_MENU_DOWN_IMG_START_Y-DIVOOM_MAIN_MENU_MID_IMG_START_Y)*v/100);
    lv_img_set_zoom(a, DIVOOM_MAIN_MENU_ZOOM_VALUE + DIVOOM_MAIN_MENU_ZOOM_DIFFER_VALUE*v/100);

	lv_obj_t * label = lv_obj_get_child(a, 0);	// 获取图片的第一个子对象，即文本框
	int zoom = lv_img_get_zoom(a);	// 获取当前图片的缩放比例
	int font_size = zoom*DIVOOM_ZOOM_FONT_SIZE/256;

	/* 根据缩放比例调整字体大小 */
	lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(font_size, DIVOOM_MENU_CONTROL_ID_MAIN), 0); // 小字体
	lv_obj_set_pos(label, DIVOOM_TEXT_MID_X + DIVOOM_TEXT_UP_DOWN_X_OFFSET - (DIVOOM_TEXT_UP_DOWN_X_OFFSET*v/100), DIVOOM_TEXT_MID_Y + DIVOOM_TEXT_UP_DOWN_Y_OFFSET -(DIVOOM_TEXT_UP_DOWN_Y_OFFSET*v/100));
}

void divoom_lvgl_main_menu_start_next_anim(Pdivoom_lvgl_main_menu_ctrl phandle)
{
    lv_obj_t *tmp_obj;
	int menu_index = phandle->func_id;

    lv_anim_init(&phandle->mid_anim);
    lv_anim_set_var(&phandle->mid_anim, phandle->mid_obj);
    lv_anim_path_linear(&phandle->mid_anim);
    lv_anim_set_path_cb(&phandle->mid_anim, lv_anim_path_linear);
    lv_anim_set_exec_cb(&phandle->mid_anim, (lv_anim_exec_xcb_t) divoom_lvgl_main_menu_mid_next_anim_cb);
    lv_anim_set_time(&phandle->mid_anim, DIVOOM_MAIN_MENU_MOVE_SPEED);
    lv_anim_set_values(&phandle->mid_anim, 0, 100);

    lv_anim_init(&phandle->down_anim);
    lv_anim_set_var(&phandle->down_anim, phandle->down_obj);
    lv_anim_path_linear(&phandle->down_anim);
    lv_anim_set_path_cb(&phandle->down_anim, lv_anim_path_linear);
    lv_anim_set_exec_cb(&phandle->down_anim, (lv_anim_exec_xcb_t) divoom_lvgl_main_menu_down_next_anim_cb);
    lv_anim_set_time(&phandle->down_anim, DIVOOM_MAIN_MENU_MOVE_SPEED);
    lv_anim_set_values(&phandle->down_anim, 0, 100);

    lv_anim_init(&phandle->up_anim);
    lv_anim_set_var(&phandle->up_anim, phandle->up_obj);
    lv_anim_path_linear(&phandle->up_anim);
    lv_anim_set_path_cb(&phandle->up_anim, lv_anim_path_linear);
    lv_anim_set_exec_cb(&phandle->up_anim, (lv_anim_exec_xcb_t) divoom_lvgl_main_menu_up_next_anim_cb);
    lv_anim_set_time(&phandle->up_anim, DIVOOM_MAIN_MENU_MOVE_SPEED);
    lv_anim_set_values(&phandle->up_anim, 0, 100);

	menu_index++;
	if(menu_index >= sizeof(divoom_main_img_list)/sizeof(divoom_main_img_list[0]))
		menu_index = 0;

	//改变图
//	divoom_menu_control_lv_img_set_src(phandle->up_obj, divoom_lvgl_main_menu_get_mode_pic_info(menu_index));
	divoom_lvgl_main_menu_set_img_src(phandle, phandle->up_obj, menu_index);

    lv_obj_t * label = lv_obj_get_child(phandle->up_obj, 0);  // 获取图片的第一个子对象，即文本框
    lv_label_set_text(label, divoom_menu_control_get_disp_str(divoom_main_mode_str[menu_index]));

	//下面将所有动画变换一下
    tmp_obj = phandle->up_obj;
    phandle->up_obj = phandle->mid_obj;
    phandle->mid_obj = phandle->down_obj;
    phandle->down_obj = tmp_obj;

    lv_anim_start(&phandle->mid_anim);
    lv_anim_start(&phandle->down_anim);
    lv_anim_start(&phandle->up_anim);
}

void divoom_lvgl_main_menu_mid_pre_anim_cb(void * a, int32_t v) //中间移到下面
{
    lv_obj_set_y(a, DIVOOM_MAIN_MENU_MID_IMG_START_Y + (DIVOOM_MAIN_MENU_DOWN_IMG_START_Y - DIVOOM_MAIN_MENU_MID_IMG_START_Y)*v/100);
    lv_img_set_zoom(a, DIVOOM_MAIN_MENU_ZOOM_VALUE + DIVOOM_MAIN_MENU_ZOOM_DIFFER_VALUE *(100-v)/100);
	lv_obj_t * label = lv_obj_get_child(a, 0);  // 获取图片的第一个子对象，即文本框
	int zoom = lv_img_get_zoom(a);  // 获取当前图片的缩放比例
	int font_size = zoom*DIVOOM_ZOOM_FONT_SIZE/256;

	/* 根据缩放比例调整字体大小 */
	lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(font_size, DIVOOM_MENU_CONTROL_ID_MAIN), 0);	// 小字体
	lv_obj_set_pos(label, DIVOOM_TEXT_MID_X + DIVOOM_TEXT_UP_DOWN_X_OFFSET*v/100, DIVOOM_TEXT_MID_Y + DIVOOM_TEXT_UP_DOWN_Y_OFFSET*v/100);	
}

void divoom_lvgl_main_menu_up_pre_anim_cb(void * a, int32_t v) //上面移到中间
{
    lv_obj_set_y(a, DIVOOM_MAIN_MENU_UP_IMG_START_Y + (DIVOOM_MAIN_MENU_MID_IMG_START_Y - DIVOOM_MAIN_MENU_UP_IMG_START_Y)*v/100);
    lv_img_set_zoom(a, DIVOOM_MAIN_MENU_ZOOM_VALUE + DIVOOM_MAIN_MENU_ZOOM_DIFFER_VALUE*v/100);

	lv_obj_t * label = lv_obj_get_child(a, 0);	// 获取图片的第一个子对象，即文本框
	int zoom = lv_img_get_zoom(a);  // 获取当前图片的缩放比例
	int font_size = zoom*DIVOOM_ZOOM_FONT_SIZE/256;

	/* 根据缩放比例调整字体大小 */
	lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(font_size, DIVOOM_MENU_CONTROL_ID_MAIN), 0);	// 小字体
	lv_obj_set_pos(label, DIVOOM_TEXT_MID_X + DIVOOM_TEXT_UP_DOWN_X_OFFSET - (DIVOOM_TEXT_UP_DOWN_X_OFFSET*v/100), DIVOOM_TEXT_MID_Y + DIVOOM_TEXT_UP_DOWN_Y_OFFSET -(DIVOOM_TEXT_UP_DOWN_Y_OFFSET*v/100));
}

void divoom_lvgl_main_menu_down_pre_anim_cb(void * a, int32_t v)//底下移到上面
{
//    lv_obj_set_y(a, 2*DIVOOM_MAIN_MENU_UP_IMG_START_Y - DIVOOM_MAIN_MENU_MID_IMG_START_Y + (DIVOOM_MAIN_MENU_MID_IMG_START_Y - DIVOOM_MAIN_MENU_UP_IMG_START_Y)*v/100);
    lv_obj_set_y(a, DIVOOM_MAIN_MENU_UP_IMG_START_Y);
	lv_img_set_zoom(a, DIVOOM_MAIN_MENU_ZOOM_VALUE *v/100);

	lv_obj_t * label = lv_obj_get_child(a, 0); 
	int zoom = lv_img_get_zoom(a);	
	int font_size = zoom*DIVOOM_ZOOM_FONT_SIZE/256;

	if (font_size == 0)
		font_size = 1;
	/* 根据缩放比例调整字体大小 */
	lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(font_size, DIVOOM_MENU_CONTROL_ID_MAIN), 0);
	lv_obj_set_pos(label, DIVOOM_TEXT_MID_X + DIVOOM_TEXT_UP_DOWN_X_OFFSET + (100 - v)*2, DIVOOM_TEXT_MID_Y + DIVOOM_TEXT_UP_DOWN_Y_OFFSET);	
}

void divoom_lvgl_main_menu_start_pre_anim(Pdivoom_lvgl_main_menu_ctrl phandle)
{
    lv_obj_t *tmp_obj;
	int menu_index = phandle->func_id;

    lv_anim_init(&phandle->mid_anim);
    lv_anim_set_var(&phandle->mid_anim, phandle->mid_obj);
    lv_anim_path_linear(&phandle->mid_anim);
    lv_anim_set_path_cb(&phandle->mid_anim, lv_anim_path_linear);
    lv_anim_set_exec_cb(&phandle->mid_anim, (lv_anim_exec_xcb_t) divoom_lvgl_main_menu_mid_pre_anim_cb);
    lv_anim_set_time(&phandle->mid_anim, DIVOOM_MAIN_MENU_MOVE_SPEED);
    lv_anim_set_values(&phandle->mid_anim, 0, 100);

    lv_anim_init(&phandle->down_anim);
    lv_anim_set_var(&phandle->down_anim, phandle->down_obj);
    lv_anim_path_linear(&phandle->down_anim);
    lv_anim_set_path_cb(&phandle->down_anim, lv_anim_path_linear);
    lv_anim_set_exec_cb(&phandle->down_anim, (lv_anim_exec_xcb_t) divoom_lvgl_main_menu_down_pre_anim_cb);
    lv_anim_set_time(&phandle->down_anim, DIVOOM_MAIN_MENU_MOVE_SPEED);
    lv_anim_set_values(&phandle->down_anim, 0, 100);

    lv_anim_init(&phandle->up_anim);
    lv_anim_set_var(&phandle->up_anim, phandle->up_obj);
    lv_anim_path_linear(&phandle->up_anim);
    lv_anim_set_path_cb(&phandle->up_anim, lv_anim_path_linear);
    lv_anim_set_exec_cb(&phandle->up_anim, (lv_anim_exec_xcb_t) divoom_lvgl_main_menu_up_pre_anim_cb);
    lv_anim_set_time(&phandle->up_anim, DIVOOM_MAIN_MENU_MOVE_SPEED);
    lv_anim_set_values(&phandle->up_anim, 0, 100);

	if (menu_index > 0)
		menu_index--;
	else
		menu_index = sizeof(divoom_main_img_list)/sizeof(divoom_main_img_list[0]) - 1;

//    divoom_menu_control_lv_img_set_src(phandle->down_obj, divoom_lvgl_main_menu_get_mode_pic_info(menu_index));
	divoom_lvgl_main_menu_set_img_src(phandle, phandle->down_obj, menu_index);
	lv_obj_t * label = lv_obj_get_child(phandle->down_obj, 0);  // 获取图片的第一个子对象，即文本框
    lv_label_set_text(label, divoom_menu_control_get_disp_str(divoom_main_mode_str[menu_index]));

	//下面将所有动画变换一下
	tmp_obj = phandle->down_obj;
	phandle->down_obj = phandle->mid_obj;
	phandle->mid_obj = phandle->up_obj;
	phandle->up_obj = tmp_obj;

    lv_anim_start(&phandle->mid_anim);
    lv_anim_start(&phandle->down_anim);
    lv_anim_start(&phandle->up_anim);
}

static void divoom_lvgl_main_menu_event_cb(lv_event_t * e)
{
    Pdivoom_lvgl_main_menu_ctrl phandle = (Pdivoom_lvgl_main_menu_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_KEY)
    {
		uint32 para = *((uint32 *)lv_event_get_param(e));
        char c = (para&0xff);
        if(c == DIVOOM_LVGL_KEY_PRE) 
		{
			phandle->func_id ++;
            if(phandle->func_id >= sizeof(divoom_main_img_list)/sizeof(divoom_main_img_list[0]))
            {
                phandle->func_id = 0;
            }

            divoom_lvgl_main_menu_start_next_anim(phandle);
        }
        else if(c == DIVOOM_LVGL_KEY_NEXT) 
		{
			if(phandle->func_id == 0)
            {
                phandle->func_id = sizeof(divoom_main_img_list)/sizeof(divoom_main_img_list[0]) - 1;
            }
            else
            {
                phandle->func_id --;
            }
            divoom_lvgl_main_menu_start_pre_anim(phandle);
        }
        else if(c == DIVOOM_LVGL_KEY_ENTER)
        {
            divoom_print("aj test: %s:%d , enter next menu: %d\n", __func__, __LINE__,phandle->func_id);
            divoom_menu_control_load_next(divoom_main_next_menu_id[phandle->func_id], NULL, 0, 0);
        }
        else if(c == DIVOOM_LVGL_KEY_ESC)
        {
            divoom_print("aj test: %s:%d , enter exit menu: %d\n", __func__, __LINE__,phandle->func_id);
            //将退出所有菜单
            divoom_menu_control_return_pre();
//            divoom_lvgl_key_handle(para);
        }
        else 
		{
            return;
        }
    }
}

void divoom_lvgl_main_menu_set_img_src(void *handle, void *obj, int func_id)
{
	Pdivoom_lvgl_main_menu_ctrl phandle = (Pdivoom_lvgl_main_menu_ctrl)handle;

	//比较一下
	Pdivoom_lvgl_main_menu_img_dsc_t new_img_data = divoom_queue_get_cmp(phandle->img_cache_queue, &func_id, divoom_lvgl_main_menu_queue_compare);
	if (new_img_data == NULL)
	{
		//缓存起来
		new_img_data = divoom_system_malloc(sizeof(divoom_lvgl_main_menu_img_dsc_t));
		if (new_img_data != NULL)
		{
			new_img_data->func_id = func_id;
			new_img_data->img_cache = divoom_system_malloc(sizeof(lv_img_dsc_t));
			if (new_img_data->img_cache == NULL ||
				divoom_lvgl_main_menu_cache_png_image(func_id, new_img_data->img_cache) != 0)
			{
				if (new_img_data->img_cache != NULL)
				{
					divoom_system_free(new_img_data->img_cache);
				}
				divoom_system_free(new_img_data);
				new_img_data = NULL;
			}
			else
			{
				divoom_queue_push(phandle->img_cache_queue, new_img_data);
			}
		}
	}

	if (new_img_data != NULL)
	{
		lv_img_set_src(obj, new_img_data->img_cache);
	}
//	lv_obj_invalidate(obj);
}

void divoom_lvgl_main_menu_create_icon_box(Pdivoom_lvgl_main_menu_ctrl phandle)
{
	int menu_index = phandle->func_id;
    lv_obj_t * cont = lv_btn_create(phandle->main_obj);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    phandle->cout_obj = cont;
    lv_obj_add_event_cb(cont, divoom_lvgl_main_menu_event_cb, LV_EVENT_KEY, phandle);

    phandle->mid_obj = lv_img_create(cont);
//    divoom_menu_control_lv_img_set_src(phandle->mid_obj, divoom_lvgl_main_menu_get_mode_pic_info(menu_index));
	divoom_lvgl_main_menu_set_img_src(phandle, phandle->mid_obj, menu_index);
    lv_obj_align(phandle->mid_obj, LV_FLEX_ALIGN_START, DIVOOM_MAIN_MENU_IMG_START_X, DIVOOM_MAIN_MENU_MID_IMG_START_Y);

	phandle->mid_label = lv_label_create(phandle->mid_obj);	   // 在图像上创建文本框
	lv_obj_set_pos(phandle->mid_label, DIVOOM_TEXT_MID_X, DIVOOM_TEXT_MID_Y);
	lv_obj_set_size(phandle->mid_label, 272 + 200, 90);
	lv_obj_set_style_text_align(phandle->mid_label, LV_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(phandle->mid_label, divoom_menu_control_get_font_info(DIVOOM_ZOOM_FONT_SIZE, DIVOOM_MENU_CONTROL_ID_MAIN), 0);  // 小字体
	lv_obj_set_style_text_color(phandle->mid_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(phandle->mid_label, divoom_menu_control_get_disp_str(divoom_main_mode_str[menu_index]));

	if (menu_index > 0)
		menu_index--;
	else
		menu_index = sizeof(divoom_main_img_list)/sizeof(divoom_main_img_list[0]) - 1;

    phandle->up_obj = lv_img_create(cont);
//    divoom_menu_control_lv_img_set_src(phandle->up_obj, divoom_lvgl_main_menu_get_mode_pic_info(menu_index));
	divoom_lvgl_main_menu_set_img_src(phandle, phandle->up_obj, menu_index);

    lv_obj_align(phandle->up_obj, LV_FLEX_ALIGN_START, DIVOOM_MAIN_MENU_IMG_START_X, DIVOOM_MAIN_MENU_UP_IMG_START_Y);
	int font_size = DIVOOM_MAIN_MENU_ZOOM_VALUE*DIVOOM_ZOOM_FONT_SIZE/256;

  	phandle->up_label = lv_label_create(phandle->up_obj);	   // 在图像上创建文本框
	lv_obj_set_pos(phandle->up_label, DIVOOM_TEXT_MID_X + DIVOOM_TEXT_UP_DOWN_X_OFFSET, DIVOOM_TEXT_MID_Y + DIVOOM_TEXT_UP_DOWN_Y_OFFSET);
	lv_obj_set_size(phandle->up_label, 272 + 200, 90);
	lv_obj_set_style_text_align(phandle->up_label, LV_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(phandle->up_label, divoom_menu_control_get_font_info(font_size, DIVOOM_MENU_CONTROL_ID_MAIN), 0);  // 小字体
	lv_obj_set_style_text_color(phandle->up_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(phandle->up_label, divoom_menu_control_get_disp_str(divoom_main_mode_str[menu_index]));
    lv_img_set_zoom(phandle->up_obj, DIVOOM_MAIN_MENU_ZOOM_VALUE);

	menu_index = phandle->func_id;
	menu_index++;
	if(menu_index >= sizeof(divoom_main_img_list)/sizeof(divoom_main_img_list[0]))
		menu_index = 0;

    phandle->down_obj = lv_img_create(cont);
//    divoom_menu_control_lv_img_set_src(phandle->down_obj, divoom_lvgl_main_menu_get_mode_pic_info(menu_index));
	divoom_lvgl_main_menu_set_img_src(phandle, phandle->down_obj, menu_index);
	lv_img_set_zoom(phandle->down_obj, DIVOOM_MAIN_MENU_ZOOM_VALUE);
    lv_obj_align(phandle->down_obj, LV_FLEX_ALIGN_START, DIVOOM_MAIN_MENU_IMG_START_X, DIVOOM_MAIN_MENU_DOWN_IMG_START_Y);
   	
	phandle->down_label = lv_label_create(phandle->down_obj);	   // 在图像上创建文本框
	lv_obj_set_pos(phandle->down_label, DIVOOM_TEXT_MID_X+DIVOOM_TEXT_UP_DOWN_X_OFFSET, DIVOOM_TEXT_MID_Y + DIVOOM_TEXT_UP_DOWN_Y_OFFSET);
	lv_obj_set_size(phandle->down_label, 272 + 200, 90);
	lv_obj_set_style_text_align(phandle->down_label, LV_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(phandle->down_label, divoom_menu_control_get_font_info(font_size, DIVOOM_MENU_CONTROL_ID_MAIN), 0);  // 小字体
	lv_obj_set_style_text_color(phandle->down_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(phandle->down_label, divoom_menu_control_get_disp_str(divoom_main_mode_str[menu_index]));
}

int divoom_lvgl_main_menu_get_next_func_from_id(int func_id) //从1开始
{
	int i = 0, num = 0;
	
	num = sizeof(divoom_main_mode_str)/sizeof(divoom_main_mode_str[0]);

	if (func_id < num -1)
		func_id++;
	else
		return -1;
	return func_id;
}

int divoom_lvgl_main_menu_get_prev_func_from_id(int func_id)
{
	if (func_id > 0)
		func_id--;
	else
		return -1;

	return func_id;	
}

int divoom_lvgl_main_menu_queue_compare(void *item1, void *item2)
{
	int *data1 = (int *)item1;
	Pdivoom_lvgl_main_menu_img_dsc_t data2 = (Pdivoom_lvgl_main_menu_img_dsc_t)item2;

	if (*data1 == data2->func_id)
		return 0;
	
	return 1;
}

void divoom_lvgl_main_menu_queue_free(void *item)
{
	Pdivoom_lvgl_main_menu_img_dsc_t img_data = (Pdivoom_lvgl_main_menu_img_dsc_t)item;

	if (img_data)
	{
		if (img_data->img_cache)
		{
			if (img_data->img_cache->data)
			{
				divoom_system_free((void *)img_data->img_cache->data);
			}
			divoom_system_free(img_data->img_cache);
		}
		divoom_system_free(img_data);
	}
}

char *divoom_lvgl_main_menu_get_img_path(int func_id)
{
	char *src = divoom_lvgl_main_menu_get_mode_pic_info(func_id);
	char *file_name = divoom_system_malloc(strlen(DIVOOM_MAIN_MENU_IMAGE_BASE_PATH) + strlen(src) + 10);

	if (file_name == NULL)
	{
		return NULL;
	}
	strcpy(file_name, DIVOOM_MAIN_MENU_IMAGE_BASE_PATH);
	strcat(file_name, src);
	divoom_print("file_name: %s\n", file_name);
	return file_name;
}

int divoom_lvgl_main_menu_cache_png_image(int func_id, lv_img_dsc_t *img_cache)
{
	uint8 *rgba_buf = NULL;
	uint8 *lv_buf = NULL;
	uint32 cache_size;
	uint32 px_count;
	int width = 0;
	int height = 0;
	char *file_name = NULL;
	int ret = -1;

	if (img_cache == NULL)
	{
		return -1;
	}
	memset(img_cache, 0, sizeof(*img_cache));
	file_name = divoom_lvgl_main_menu_get_img_path(func_id);
	if (file_name == NULL)
	{
		return -1;
	}

	/* 后台缓存线程不能进入 LVGL decoder；直接使用 Allwinner PNG 解码器。 */
	rgba_buf = divoom_aw_decode_png_file(file_name, &width, &height);
	if (rgba_buf == NULL || width <= 0 || height <= 0)
	{
		divoom_print("main menu png decode fail: %s\n", file_name);
		goto cleanup;
	}

	cache_size = divoom_pixel_format_size((uint32)width, (uint32)height,
		LV_COLOR_FORMAT_RGB565A8);
	if (cache_size == 0)
	{
		goto cleanup;
	}
	lv_buf = divoom_system_malloc(cache_size);
	if (lv_buf == NULL)
	{
		divoom_print("main menu png cache malloc fail: %s, size=%u\n",
			file_name, cache_size);
		goto cleanup;
	}

	px_count = (uint32)width * (uint32)height;
	divoom_rgba8888_to_rgb565a8_buf(rgba_buf, lv_buf, px_count);
	divoom_lv_img_dsc_init_fmt(img_cache, (uint32)width, (uint32)height,
		LV_COLOR_FORMAT_RGB565A8, lv_buf);
	lv_buf = NULL;
	ret = 0;

cleanup:
	if (lv_buf != NULL)
	{
		divoom_system_free(lv_buf);
	}
	if (rgba_buf != NULL)
	{
		divoom_system_free(rgba_buf);
	}
	divoom_system_free(file_name);
	return ret;
}

void *divoom_lvgl_dial_main_menu_img_cache_task_handle(void *para)
{
	int i = 0, j = 0, func_id = 0, index = 0, index2 = 0;
    Pdivoom_lvgl_main_menu_ctrl phandle = (Pdivoom_lvgl_main_menu_ctrl)para;
	Pdivoom_lvgl_main_menu_img_dsc_t img_data = NULL;
	Pdivoom_lvgl_main_menu_img_dsc_t new_img_data = NULL;

	//得到一个表盘列表
	int func_table[12] = {0};
	int del_func_table[12] = {0}; //删除列表

	while (phandle->run_flag)
	{
		index = 0;
//		if (phandle->cur_func_id != phandle->func_id)
		if (phandle->cur_func_id == -1) //全缓冲
		{
			memset(func_table, -1, sizeof(func_table));
			memset(del_func_table, -1, sizeof(del_func_table));
			index = 0;
			index2 = 0;
			func_id = 0;//phandle->func_id;
			phandle->cur_func_id = phandle->func_id;
			func_table[index++] = func_id;//phandle->func_id;
			for (i = 0; i < 7; i++) //全缓冲
			{
				func_id = divoom_lvgl_main_menu_get_next_func_from_id(func_id);
				if (func_id >= 0)
				{
					func_table[index++] = func_id;
				}
				else
					break;
			}
#if 0
			func_id = phandle->func_id;
			for (i = 0; i < 5; i++) //前3个
			{
				func_id = divoom_lvgl_main_menu_get_prev_func_from_id(func_id);
				if (func_id >= 0)
				{
					func_table[index++] = func_id;
				}
				else
					break;
			}

			int num = divoom_queue_items(phandle->img_cache_queue);
			for (i = 0; i < num; i++) //统计多余的表盘
			{
				new_img_data = divoom_queue_get_index_item(phandle->img_cache_queue, i);
				if (new_img_data)
				{
					for (j = 0; j < index; j++)
					{
						if (new_img_data->func_id == func_table[j])
							break;
					}
					if (j == index) //不存在
					{
						del_func_table[index2++] = new_img_data->func_id;
					}
				}
			}

			//从队列里面删除多余的
			for (i = 0; i < index2; i++)
			{
				img_data = divoom_queue_pop_cmp(phandle->img_cache_queue, &del_func_table[i], divoom_lvgl_main_menu_queue_compare);
				if (img_data)
				{
					divoom_lvgl_main_menu_queue_free(img_data);
				}
			}
#endif
			for (i = 0; i < index; i++)
			{
				//比较存在
				func_id = func_table[i];
				if (func_id >= 0)
				{
					//比较一下
					img_data = divoom_queue_get_cmp(phandle->img_cache_queue, &func_id, divoom_lvgl_main_menu_queue_compare);
					if (img_data == NULL)
					{
						//缓存起来
						Pdivoom_lvgl_main_menu_img_dsc_t new_img_data = divoom_system_malloc(sizeof(divoom_lvgl_main_menu_img_dsc_t));
						if (new_img_data != NULL)
						{
							new_img_data->func_id = func_id;
							new_img_data->img_cache = divoom_system_malloc(sizeof(lv_img_dsc_t));
							if (new_img_data->img_cache == NULL ||
								divoom_lvgl_main_menu_cache_png_image(func_id, new_img_data->img_cache) != 0)
							{
								if (new_img_data->img_cache != NULL)
								{
									divoom_system_free(new_img_data->img_cache);
								}
								divoom_system_free(new_img_data);
							}
							else
							{
								divoom_queue_push(phandle->img_cache_queue, new_img_data);
							}
						}
					}
				}
			}
		}
		divoom_system_delay_ms(100);
	}

	divoom_system_put_sem(phandle->sem);
	return NULL;
}

void *divoom_lvgl_main_menu_init(void *user_data, int menu_id)
{
	int i = 0;
    Pdivoom_lvgl_main_menu_ctrl phandle = divoom_system_malloc(sizeof(divoom_lvgl_main_menu_ctrl));

    phandle->menu_id = menu_id;
	phandle->main_obj = lv_obj_create(NULL);
	phandle->cur_func_id = -1;
	phandle->func_id = 0;
	lv_style_init(&phandle->backgroud_type);
	lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(0xFFFFFF));
	lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);

	lv_obj_t *img = lv_img_create(phandle->main_obj);
	lv_obj_set_pos(img, 0, 0);
	lv_obj_set_size(img, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);

	phandle->run_flag = 1;
	phandle->sem = divoom_system_create_sem(1, 0);
	phandle->img_cache_queue = divoom_queue_init(10, __func__, __LINE__);

    divoom_menu_control_lv_img_set_src(img, "1/1_main_bg.png");
	divoom_lvgl_main_menu_create_icon_box(phandle);
    divoom_system_create_task(divoom_lvgl_dial_main_menu_img_cache_task_handle, "lv_main_menu", phandle, DIVOOM_TASK_STACK_LVGL_MAIN_MENU);
    return phandle;
}

void divoom_lvgl_main_menu_disp(void *handle, void *active_obj)
{    
	Pdivoom_lvgl_main_menu_ctrl phandle = (Pdivoom_lvgl_main_menu_ctrl)handle;
    
    lv_group_focus_obj(phandle->cout_obj);
	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_main_menu_exit(void *handle)
{
	Pdivoom_lvgl_main_menu_ctrl phandle = (Pdivoom_lvgl_main_menu_ctrl)handle;

	phandle->run_flag = 0;	
	divoom_system_get_sem(phandle->sem);
	divoom_system_destroy_sem(phandle->sem);
	divoom_queue_destroy(phandle->img_cache_queue, divoom_lvgl_main_menu_queue_free);
	lv_style_reset(&phandle->backgroud_type);
	lv_obj_del(phandle->main_obj);
	divoom_system_free(phandle);
}
