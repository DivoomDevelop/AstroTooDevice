#ifndef _DIVOOM_Color_HSV_H
#define _DIVOOM_Color_HSV_H
/*
H: hue in degrees (-1535 - 1535)   H是色彩
S: saturation (0 - 255)       S是深浅， S = 0时，只有灰度
V: value (0 - 255)            V是明暗，表示色彩的明亮程度，但与光强无直接联系
*/

#define H_MAX 1535
#define S_MAX 255
#define V_MAX 255

typedef struct __divoom_screen_hsv_info
{
	int32 hue;		//色彩
    uint8 saturation;
    uint8 value;
}divoom_screen_hsv_info, *Pdivoom_screen_hsv_info;


void divoom_hsvToRGB(int32 hue, uint8 saturation, uint8 value, uint8 * red, uint8 * green, uint8 * blue);
void divoom_CreateHSVColor(int32 hue, uint8 saturation, uint8 value, Pdivoom_screen_color_info color);
void divoom_CreateHSV_Color(uint16 divisions, uint16 index, uint8 saturation, uint8 value, Pdivoom_screen_color_info color);
void divoom_CreateAHSVColor(uint16 divisions, uint16 index, uint8 saturation, uint8 value, Pdivoom_screen_color_info color);
void divoom_CreateHSVColorWithDivisions(uint16 divisions, uint16 index, Pdivoom_screen_color_info color);
void divoom_CreateHSVColorWithArray(uint16 *array, uint8 array_len, uint16 index, Pdivoom_screen_color_info color);

void divoom_RgbToHsv(uint8 r, uint8 g, uint8 b, int32 * hue, uint8 * saturation, uint8 * value);

void divoom_rgb_to_hsv(Pdivoom_screen_color_info pcolor, Pdivoom_disp_hsv_info phsv);
void divoom_hsv_to_rgb(Pdivoom_disp_hsv_info phsv, Pdivoom_screen_color_info pcolor);

/*********************************************************************
  Function:        divoom_rgb_hsv_change_brightness
  Description:    	改变颜色亮度
  Input:           	fcolor  原来的颜色rgb
  					tcolor  改变之后的颜色rgb
  					index	控制亮度的改变系数
  					total_index   把原有颜色亮度值分成多少份
  Return:           NONE
  Author:           wangft
  Data:            2019-6-19
**********************************************************************/
void divoom_rgb_hsv_change_brightness(Pdivoom_screen_color_info fcolor, Pdivoom_screen_color_info tcolor, int index, T_FLOAT total_index);

/*********************************************************************
  Function:        divoom_rgb_hsv_change_rgb
  Description:    	改变色相
  Input:           	fcolor  原来的颜色rgb
  					tcolor  改变之后的颜色rgb
  					h	控制色相的改变系数
  					s	改变颜色的深浅
  					v	改变颜色的亮度
  Return:           NONE
  Author:           wangft
  Data:            2019-6-19
**********************************************************************/
void divoom_rgb_hsv_change_rgb(Pdivoom_screen_color_info fcolor, Pdivoom_screen_color_info tcolor, int h, int s, int v);

#if 0
原型
/* Define to prevent recursive inclusion -------------------------------------*/
//#define NUM_OF_COLOR_VALUES 256
//#define MIN_COLOR_VALUE     0
#define MAX_COLOR_VALUE     255
//#define PALETTE_SIZE        256
//#define NUM_OF_PALETTES      12
//#define GRAYSCALE_PALETTE     0
//#define SPECTRUM_PALETTE      1
//#define SIN1_PALETTE          2
//#define SIN2_PALETTE          3
//#define SIN3_PALETTE          4
//#define SIN4_PALETTE          5
//#define SIN5_PALETTE          6
//#define RANDOM_PALETTE        7
//#define FIRE_PALETTE          8
//#define SEA_PALETTE           9
//#define BLUERED_PALETTE       10
//#define COLORWHEEL_PALETTE    11

H: hue in degrees (0 - 360.0)   H是色彩
S: saturation (0.0 - 1.0)       S是深浅， S = 0时，只有灰度
V: value (0.0 - 1.0)            V是明暗，表示色彩的明亮程度，但与光强无直接联系
#define rgb24 T_U32
void divoom_hsvToRGB(T_FLOAT hue, T_FLOAT saturation, T_FLOAT value, T_FLOAT * red, T_FLOAT * green, T_FLOAT * blue);
#endif

#endif 


