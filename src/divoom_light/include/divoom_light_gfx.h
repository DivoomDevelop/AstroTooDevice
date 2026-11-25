/**
  ******************************************************************************
  * @file    divoom_light_gfx.h
  * @author  coronaa
  * @version V1.0
  * @date    2017-07-03
  * @brief   
  ******************************************************************************
  *移植来源:
  *论坛:     http://bbs.mydigit.cn/read.php?tid=1721594
  *github:   https://github.com/TyMaker/STM32_Matrix
  *adafruit: https://learn.adafruit.com/32x16-32x32-rgb-led-matrix/library
  ******************************************************************************
**/
#ifndef __DIVOOM_LIGHT_GFX_H
#define	__DIVOOM_LIGHT_GFX_H

void divoom_light_gfx_drawLine(int8 x0, int8 y0, int8 x1, int8 y1, Pdivoom_light_color_info color, uint8 *disp_buf);

void divoom_light_gfx_drawFastVLine(int8 x, int8 y, int8 h, Pdivoom_light_color_info color, uint8 *disp_buf);

void divoom_light_gfx_drawFastHLine(int8 x, int8 y, int8 w, Pdivoom_light_color_info color, uint8 *disp_buf);

void divoom_light_gfx_drawRect(int8 x, int8 y, int8 w, int8 h, Pdivoom_light_color_info color, uint8 *disp_buf);

void divoom_light_gfx_fillRect(int8 x, int8 y, int8 w, int8 h,Pdivoom_light_color_info color, uint8 *disp_buf);

void divoom_light_gfx_drawRect_center(int8 x, int8 y, int8 w, int8 h, Pdivoom_light_color_info color, uint8 *disp_buf);//从中间点画矩形，边长为奇数

void divoom_light_gfx_drawCircle(int8 x0, int8 y0, int8 r, Pdivoom_light_color_info color, uint8 *disp_buf);

void divoom_light_gfx_drawCircleHelper( int8 x0, int8 y0, int8 r, int8 cornername, Pdivoom_light_color_info color, uint8 *disp_buf);

void divoom_light_gfx_fillCircleHelper(int8 x0, int8 y0, int8 r, int8 cornername, int8 delta, Pdivoom_light_color_info color, uint8 *disp_buf);

void divoom_light_gfx_fillCircle(int8 x0, int8 y0, int8 r, Pdivoom_light_color_info color, uint8 *disp_buf);

void divoom_light_gfx_drawTriangle(int8 x0, int8 y0, int8 x1, int8 y1, int8 x2, int8 y2, Pdivoom_light_color_info color, uint8 *disp_buf);

void divoom_light_gfx_fillTriangle(int8 x0, int8 y0, int8 x1, int8 y1, int8 x2, int8 y2, Pdivoom_light_color_info color, uint8 *disp_buf);

void divoom_light_gfx_drawRoundRect(int8 x, int8 y, int8 w, int8 h, int8 r, Pdivoom_light_color_info color, uint8 *disp_buf);

void divoom_light_gfx_fillRoundRect(int8 x, int8 y, int8 w, int8 h, int8 r, Pdivoom_light_color_info color, uint8 *disp_buf);

void divoom_light_gfx_fillScreen(Pdivoom_light_color_info color, uint8 *disp_buf);

/*
void drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap,	int16_t w, int16_t h, uint16_t color);
*/

void divoom_light_gfx_drawRhombus(int8 x, int8 y, int8 w, int8 h, Pdivoom_light_color_info color, uint8 *disp_buf);

#endif

