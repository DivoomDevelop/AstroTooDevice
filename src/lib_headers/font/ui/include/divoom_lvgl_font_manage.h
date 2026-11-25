#ifndef _divoom_lvgl_font_manage_h_
#define _divoom_lvgl_font_manage_h_

/* AI 正文独立的本地字体键，不占用服务器的正数 FontId。 */
#define DIVOOM_LVGL_FONT_MANAGE_AI_INTER_MEDIUM_ID (-1001)
#define DIVOOM_LVGL_FONT_MANAGE_AI_INTER_MEDIUM_PATH "/usr/share/divoom_app/system/ai_fonts/Inter-Medium.ttf"

void divoom_lvgl_font_manage_load_get_font_info(int font_id, int *font_type, char *file_name);

enum
{   
    DIVOOM_LVGL_FONT_MANAGE_TYPE_TTF_FONT = 1,
    DIVOOM_LVGL_FONT_MANAGE_TYPE_IMAGE_FONT = 2,
};
    
/*********************************************************************
  Function:        divoom_lvgl_font_manage_init
  Description:     初始化字体管理模块
  Input:           
                   
  Return:          NONE
**********************************************************************/

void divoom_lvgl_font_manage_init(void);

/*********************************************************************
  Function:        divoom_lvgl_font_manage_is_inited
  Description:     字体管理是否已完成初始化（lock 已就绪）
  Return:          1 已初始化，0 未初始化
**********************************************************************/

int divoom_lvgl_font_manage_is_inited(void);
/*********************************************************************
  Function:        divoom_lvgl_font_manage_load_menu_font
  Description:     加载全字体，这个字体加载就不会再释放
  Input:           font_size   字体大小
  Return:          返回lv_font_t
**********************************************************************/

void *divoom_lvgl_font_manage_load_all_font(int font_size);

/*********************************************************************
  Function:        divoom_lvgl_font_manage_load_menu_font
  Description:     加载菜单字体，这个字体加载就不会再释放
  Input:           font_size   字体大小
  Return:          返回lv_font_t
**********************************************************************/

void *divoom_lvgl_font_manage_load_menu_font(int font_size);



/*********************************************************************
  Function:        divoom_lvgl_font_manage_load_font
  Description:     加载某个字体
  Input:           font_id  字体ID
                   font_size   字体大小，如果为0，又是TTF字体，将根据width自动找适合的字体大小
                   disp_str     要显示的字串，只有font_size=0才被用到
                   width        显示宽度，只有font_size=0才被用到
                   min_size     最小字号，只有font_size=0才被用到
                   max_size     最大字号，只有font_size=0才被用到
                   font_type    字体类型
  Return:          如果是图像字体，将返回根目录路径，否则返回lv_font_t
**********************************************************************/

void *divoom_lvgl_font_manage_load_font(int font_id, int font_size,char *disp_str, int width, int min_size, int max_size, int *font_type, int menu_id, Pdivoom_clock_id_info pclock_info);

/*
 * 按 FreeType 样式加载固定字号字体。普通体和粗体分别缓存，调用方无需管理字体生命周期。
 */
void *divoom_lvgl_font_manage_load_font_style(int font_id, int font_size,
    uint16 font_style, int menu_id, Pdivoom_clock_id_info pclock_info);


/*********************************************************************
  Function:        divoom_lvgl_font_manage_clear_font
  Description:     释放所有的菜单字体
  Input:           flag  1:清除所有字体；0：只保留系统菜单字体
  Return:          NONE
**********************************************************************/

void divoom_lvgl_font_manage_clear_font(int flag);

/*********************************************************************
  Function:        divoom_lvgl_font_manage_clear_font
  Description:     释放所有的菜单字体
  Input:           flag  1:清除所有字体；0：只保留系统菜单字体
  Return:          NONE
**********************************************************************/

void divoom_lvgl_font_manage_clear_clock_font(int menu_id, Pdivoom_clock_id_info pclock_info);

#if defined(DIVOOM_SIMULATOR)
int divoom_lvgl_font_manage_whitebox_matches_font(int font_id, int font_size, const void *font);
#endif

#endif


