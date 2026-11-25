/**
  ******************************************************************************
  * @file    divoom_light_gfx.c
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
#include "divoom.h"
#include "divoom_light_math.h"

// Bresenham's algorithm - thx wikpedia
void divoom_light_gfx_drawLine(int8 x0, int8 y0, int8 x1, int8 y1, Pdivoom_light_color_info color, uint8 *disp_buf)
{
    int8 steep = divoom_math_abs(y1 - y0) > divoom_math_abs(x1 - x0);
    int8 ystep, err, dx, dy;

    if (steep)
    {
        DIVOOM_SWAP_INT8(x0, y0);
        DIVOOM_SWAP_INT8(x1, y1);
    }

    if (x0 > x1)
    {
        DIVOOM_SWAP_INT8(x0, x1);
        DIVOOM_SWAP_INT8(y0, y1);
    }

    dx = x1 - x0;
    dy = divoom_math_abs(y1 - y0);

    err = dx / 2;

    if (y0 < y1)
    {
        ystep = 1;
    }
    else
    {
        ystep = -1;
    }

    for (; x0<=x1; x0++)
    {
        if (steep)
        {
            divoom_disp_ctrl_set_data_xy(y0, x0, color, disp_buf);
        }
        else
        {
            divoom_disp_ctrl_set_data_xy(x0, y0, color,disp_buf);
        }
        err -= dy;
        if (err < 0)
        {
            y0 += ystep;
            err += dx;
        }
    }
}
//  u8 dx,dy,k,b;
//  dx = x1 - x0;
//  dy = y1 - y0;
//  k=dy/dx;
//  b=y0-(x0*k);
//
//  for(;x0<x1;x0++)
//  {
//    divoom_disp_ctrl_set_data_xy(x0,(x0*k)+b,color,disp_buf);
//  }


void divoom_light_gfx_drawFastVLine(int8 x, int8 y, int8 h, Pdivoom_light_color_info color, uint8 *disp_buf)
{
    // Update in subclasses if desired!
//    divoom_light_gfx_drawLine(x, y, x, y+h-1, color, disp_buf);
    int i;
    for(i = 0;i < h; i++)
    {
        divoom_disp_ctrl_set_data_xy(x, y+i, color, disp_buf);
    }
}

void divoom_light_gfx_drawFastHLine(int8 x, int8 y, int8 w, Pdivoom_light_color_info color, uint8 *disp_buf)
{
    // Update in subclasses if desired!
//    divoom_light_gfx_drawLine(x, y, x+w-1, y, color, disp_buf);
    int i;
    for(i = 0;i < w; i++)
    {
        divoom_disp_ctrl_set_data_xy(x+i, y, color, disp_buf);
    }
}

// Draw a rectangle
void divoom_light_gfx_drawRect(int8 x, int8 y, int8 w, int8 h, Pdivoom_light_color_info color, uint8 *disp_buf)
{
    divoom_light_gfx_drawFastHLine(x, y, w, color, disp_buf);
    divoom_light_gfx_drawFastHLine(x, y+h-1, w, color, disp_buf);
    divoom_light_gfx_drawFastVLine(x, y, h, color, disp_buf);
    divoom_light_gfx_drawFastVLine(x+w-1, y, h, color, disp_buf);
}

void divoom_light_gfx_fillRect(int8 x, int8 y, int8 w, int8 h,Pdivoom_light_color_info color, uint8 *disp_buf)
{
    // Update in subclasses if desired!
    int8 i;
    for (i=x; i<x+w; i++)
    {
        divoom_light_gfx_drawFastVLine(i, y, h, color, disp_buf);
    }
}
void divoom_light_gfx_drawRect_center(int8 x, int8 y, int8 w, int8 h, Pdivoom_light_color_info color, uint8 *disp_buf)
{
//从中间点画矩形，边长为奇数
//W.H为中点到边的距离
    divoom_light_gfx_drawFastHLine(x-w, y-h, (w<<1)+1, color, disp_buf);
    divoom_light_gfx_drawFastHLine(x-w, y+h, (w<<1)+1, color, disp_buf);
    divoom_light_gfx_drawFastVLine(x-w, y-h, (h<<1)+1, color, disp_buf);
    divoom_light_gfx_drawFastVLine(x+w, y-h, (h<<1)+1, color, disp_buf);
//下边W,H为矩形的边长
//    divoom_light_gfx_drawFastHLine(x-(w>>1), y-(h>>1), w, color, disp_buf);
//    divoom_light_gfx_drawFastHLine(x-(w>>1), y-(h>>1)+h-1, w, color, disp_buf);
//    divoom_light_gfx_drawFastVLine(x-(w>>1), y-(h>>1), h, color, disp_buf);
//    divoom_light_gfx_drawFastVLine(x-(w>>1)+w-1, y-(h>>1), h, color, disp_buf);
}

// Draw a circle outline
void divoom_light_gfx_drawCircle(int8 x0, int8 y0, int8 r, Pdivoom_light_color_info color, uint8 *disp_buf)
{
    int8 f = 1 - r;
    int8 ddF_x = 1;
    int8 ddF_y = -2 * r;
    int8 x = 0;
    int8 y = r;

    divoom_disp_ctrl_set_data_xy(x0  , y0+r, color, disp_buf);
    divoom_disp_ctrl_set_data_xy(x0  , y0-r, color, disp_buf);
    divoom_disp_ctrl_set_data_xy(x0+r, y0  , color, disp_buf);
    divoom_disp_ctrl_set_data_xy(x0-r, y0  , color, disp_buf);

    while (x<y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        divoom_disp_ctrl_set_data_xy(x0 + x, y0 + y, color, disp_buf);
        divoom_disp_ctrl_set_data_xy(x0 - x, y0 + y, color, disp_buf);
        divoom_disp_ctrl_set_data_xy(x0 + x, y0 - y, color, disp_buf);
        divoom_disp_ctrl_set_data_xy(x0 - x, y0 - y, color, disp_buf);
        divoom_disp_ctrl_set_data_xy(x0 + y, y0 + x, color, disp_buf);
        divoom_disp_ctrl_set_data_xy(x0 - y, y0 + x, color, disp_buf);
        divoom_disp_ctrl_set_data_xy(x0 + y, y0 - x, color, disp_buf);
        divoom_disp_ctrl_set_data_xy(x0 - y, y0 - x, color, disp_buf);
    }
}


//画圆弧:分4像限
void divoom_light_gfx_drawCircleHelper( int8 x0, int8 y0, int8 r, int8 cornername, Pdivoom_light_color_info color, uint8 *disp_buf)
{
    int8 f     = 1 - r;
    int8 ddF_x = 1;
    int8 ddF_y = -2 * r;
    int8 x     = 0;
    int8 y     = r;

    while (x<y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        if (cornername & 0x4)
        {
            divoom_disp_ctrl_set_data_xy(x0 + x, y0 + y, color, disp_buf);
            divoom_disp_ctrl_set_data_xy(x0 + y, y0 + x, color, disp_buf);
        }
        if (cornername & 0x2)
        {
            divoom_disp_ctrl_set_data_xy(x0 + x, y0 - y, color, disp_buf);
            divoom_disp_ctrl_set_data_xy(x0 + y, y0 - x, color, disp_buf);
        }
        if (cornername & 0x8)
        {
            divoom_disp_ctrl_set_data_xy(x0 - y, y0 + x, color, disp_buf);
            divoom_disp_ctrl_set_data_xy(x0 - x, y0 + y, color, disp_buf);
        }
        if (cornername & 0x1)
        {
            divoom_disp_ctrl_set_data_xy(x0 - y, y0 - x, color, disp_buf);
            divoom_disp_ctrl_set_data_xy(x0 - x, y0 - y, color, disp_buf);
        }
    }
}

// Used to do circles and roundrects
void divoom_light_gfx_fillCircleHelper(int8 x0, int8 y0, int8 r, int8 cornername, int8 delta, Pdivoom_light_color_info color, uint8 *disp_buf)
{

    int8 f     = 1 - r;
    int8 ddF_x = 1;
    int8 ddF_y = -2 * r;
    int8 x     = 0;
    int8 y     = r;

    while (x<y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;

        if (cornername & 0x1)
        {
            divoom_light_gfx_drawFastVLine(x0+x, y0-y, 2*y+1+delta, color, disp_buf);
            divoom_light_gfx_drawFastVLine(x0+y, y0-x, 2*x+1+delta, color, disp_buf);
        }
        if (cornername & 0x2)
        {
            divoom_light_gfx_drawFastVLine(x0-x, y0-y, 2*y+1+delta, color, disp_buf);
            divoom_light_gfx_drawFastVLine(x0-y, y0-x, 2*x+1+delta, color, disp_buf);
        }
    }
}

void divoom_light_gfx_fillCircle(int8 x0, int8 y0, int8 r, Pdivoom_light_color_info color, uint8 *disp_buf)
{
    divoom_light_gfx_drawFastVLine(x0, y0-r, 2*r+1, color, disp_buf);
    divoom_light_gfx_fillCircleHelper(x0, y0, r, 3, 0, color, disp_buf);
}

// Draw a triangle
void divoom_light_gfx_drawTriangle(int8 x0, int8 y0, int8 x1, int8 y1, int8 x2, int8 y2, Pdivoom_light_color_info color, uint8 *disp_buf)
{
    divoom_light_gfx_drawLine(x0, y0, x1, y1, color, disp_buf);
    divoom_light_gfx_drawLine(x1, y1, x2, y2, color, disp_buf);
    divoom_light_gfx_drawLine(x2, y2, x0, y0, color, disp_buf);
}

// Fill a triangle
void divoom_light_gfx_fillTriangle(int8 x0, int8 y0, int8 x1, int8 y1, int8 x2, int8 y2, Pdivoom_light_color_info color, uint8 *disp_buf)
{
    T_S16 a, b, y, last;
    T_S16
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
    T_S32
    sa   = 0,
    sb   = 0;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1)
    {
        DIVOOM_SWAP_INT8(y0, y1);
        DIVOOM_SWAP_INT8(x0, x1);
    }
    if (y1 > y2)
    {
        DIVOOM_SWAP_INT8(y2, y1);
        DIVOOM_SWAP_INT8(x2, x1);
    }
    if (y0 > y1)
    {
        DIVOOM_SWAP_INT8(y0, y1);
        DIVOOM_SWAP_INT8(x0, x1);
    }

    if(y0 == y2)   // Handle awkward all-on-same-line case as its own thing
    {
        a = b = x0;
        if(x1 < a)      a = x1;
        else if(x1 > b) b = x1;
        if(x2 < a)      a = x2;
        else if(x2 > b) b = x2;
        divoom_light_gfx_drawFastHLine(a, y0, b-a+1, color, disp_buf);
        return;
    }


    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if(y1 == y2) last = y1;   // Include y1 scanline
    else         last = y1-1; // Skip it

    for(y=y0; y<=last; y++)
    {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        /* longhand:
        a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b) DIVOOM_SWAP_INT8(a,b);
        divoom_light_gfx_drawFastHLine(a, y, b-a+1, color, disp_buf);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for(; y<=y2; y++)
    {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        /* longhand:
        a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b) DIVOOM_SWAP_INT8(a,b);
        divoom_light_gfx_drawFastHLine(a, y, b-a+1, color, disp_buf);
    }
}


// Draw a rounded rectangle
void divoom_light_gfx_drawRoundRect(int8 x, int8 y, int8 w, int8 h, int8 r, Pdivoom_light_color_info color, uint8 *disp_buf)
{
    // smarter version
    divoom_light_gfx_drawFastHLine(x+r  , y    , w-2*r, color, disp_buf); // Top
    divoom_light_gfx_drawFastHLine(x+r  , y+h-1, w-2*r, color, disp_buf); // Bottom
    divoom_light_gfx_drawFastVLine(x    , y+r  , h-2*r, color, disp_buf); // Left
    divoom_light_gfx_drawFastVLine(x+w-1, y+r  , h-2*r, color, disp_buf); // Right
    // draw four corners
    divoom_light_gfx_drawCircleHelper(x+r    , y+r    , r, 1, color, disp_buf);
    divoom_light_gfx_drawCircleHelper(x+w-r-1, y+r    , r, 2, color, disp_buf);
    divoom_light_gfx_drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color, disp_buf);
    divoom_light_gfx_drawCircleHelper(x+r    , y+h-r-1, r, 8, color, disp_buf);
}

// Fill a rounded rectangle
void divoom_light_gfx_fillRoundRect(int8 x, int8 y, int8 w, int8 h, int8 r, Pdivoom_light_color_info color, uint8 *disp_buf)
{
    // smarter version
    divoom_light_gfx_fillRect(x+r, y, w-2*r, h, color, disp_buf);

    // draw four corners
    divoom_light_gfx_fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color, disp_buf);
    divoom_light_gfx_fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color, disp_buf);
}


void divoom_light_gfx_fillScreen(Pdivoom_light_color_info color, uint8 *disp_buf)
{
    divoom_light_gfx_fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color, disp_buf);
}



void divoom_light_gfx_drawRhombus(int8 x, int8 y, int8 w, int8 h, Pdivoom_light_color_info color, uint8 *disp_buf)
{
//从中间点画正菱形，边长为奇数
//W.H为中点到端点的距离
    divoom_light_gfx_drawLine(x-w,y,x,y-h,color,disp_buf);
    divoom_light_gfx_drawLine(x-w,y,x,y+h,color,disp_buf);
    divoom_light_gfx_drawLine(x+w,y,x,y-h,color,disp_buf);
    divoom_light_gfx_drawLine(x+w,y,x,y+h,color,disp_buf);
}


