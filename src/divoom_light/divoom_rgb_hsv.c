#include "divoom_disp_com.h"

/*****************************************************************************
设 (r, g, b) 分别是一个颜色的红、绿和蓝坐标，它们的值是在 0 到 1 之间的实数。
设 max 等价于 r, g 和 b 中的最大者。设 min 等于这些值中的最小者。
要找到在 HSV 空间中的 (h, s, v) 值，这里的 h ∈ [0, 360）是角度的色相角，
而 s, v ∈ [0,1] 是饱和度和亮度
*****************************************************************************/

/**
 * @function:    divoom_RgbToHsv
 * @description: 设 (r, g, b) 分别是一个颜色的红、绿和蓝坐标，它们的值是在 0 到 255 之间的整数。
                 设 max 等价于 r, g 和 b 中的最大者。设 min 等于这些值中的最小者。
                 要找到在 HSV 空间中的 (h, s, v) 值，这里的 h ∈ [0, 360）是角度的色相角，
                 而 s, v ∈ [0,1] 是饱和度和亮度
 * @Author:      coronaa
 * @date:        2017-07-03
 * @param        r              [0-255]
 * @param        g              [0-255]
 * @param        b              [0-255]
 * @param        hue            [0-1535]
 * @param        saturation     [0-255]
 * @param        value          [0-255]
 */
void divoom_RgbToHsv(uint8 r, uint8 g, uint8 b, int32 * hue, uint8 * saturation, uint8 * value)
{
// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
// if s == 0, then h = -1 (undefined)

    int32 min, max, delta,tmp;
    tmp = DIVOOM_MIN(r, g);
    min = DIVOOM_MIN(tmp, b);
    tmp = DIVOOM_MAX(r, g);
    max = DIVOOM_MAX(tmp, b);
    *value = max; // v
    delta = max - min;
    if(delta == 0)
    {
        *hue = 0;
        *saturation = 0;        
        return;
    }
    if( max != 0 )
        *saturation = (delta * 255) / max; // s
    else
    {
// r = g = b = 0 // s = 0, v is undefined
        *saturation = 0;
//        hue = UNDEFINEDCOLOR;
        *hue = 0;
        return;
    }
    if( r == max )
        *hue = ((int32)( g - b ) * 255) / delta; // between yellow & magenta
    else if( g == max )
        *hue = 2 * 255 + ((int32)( b - r ) * 255)/ delta; // between cyan & yellow
    else
        *hue = 4 * 255 + ((int32)( r - g ) * 255)/ delta; // between magenta & cyan

//    *hue *= 60; // degrees
    if( *hue < 0 )
        *hue += 1536;
}

/**
 * @function:    divoom_hsvToRGB
 * @description: HSV to RGB color conversion
 * @Author:      coronaa
 * @date:        2017-07-03
 * @param        hue            [hue in degrees (-1535 - 1535)]
 * @param        saturation     [0 - 255]
 * @param        value          [0 - 255]
 * @param        red            [0 - 255]
 * @param        green          [0 - 255]
 * @param        blue           [0 - 255]
 */
void divoom_hsvToRGB(int32 hue, uint8 saturation, uint8 value, uint8 * red, uint8 * green, uint8 * blue)
{
    uint8 r, g, b, lo;
    uint16 s1, v1;
    // Hue
    hue %= 1536;             // -1535 to +1535      0x600
    if(hue < 0) hue += 1536; //     0 to +1535
    lo = hue & 255;          // Low byte  = primary/secondary color mix     hue & 0xff
    switch(hue >> 8) {       // High byte = sextant of colorwheel
        case 0 :
            r = 255     ;
            g =  lo     ;
            b =   0     ;
            break; // R to Y
        case 1 :
            r = 255 - lo;
            g = 255     ;
            b =   0     ;
            break; // Y to G
        case 2 :
            r =   0     ;
            g = 255     ;
            b =  lo     ;
            break; // G to C
        case 3 :
            r =   0     ;
            g = 255 - lo;
            b = 255     ;
            break; // C to B
        case 4 :
            r =  lo     ;
            g =   0     ;
            b = 255     ;
            break; // B to M
        default:
            r = 255     ;
            g =   0     ;
            b = 255 - lo;
            break; // M to R
    }

    // Saturation: add 1 so range is 1 to 256, allowig a quick shift operation
    // on the result rather than a costly divide, while the type upgrade to int
    // avoids repeated type conversions in both directions.
    s1 = saturation + 1;
    r  = 255 - (((255 - r) * s1) >> 8);
    g  = 255 - (((255 - g) * s1) >> 8);
    b  = 255 - (((255 - b) * s1) >> 8);

    // Value (brightness) & 16-bit color reduction: similar to above, add 1
    // to allow shifts, and upgrade to int makes other conversions implicit.
    v1 = value+ 1;
    /*
    if(gflag) { // Gamma-corrected color?
        r = gamma[(r * v1) >> 8]; // Gamma correction table maps
        g = gamma[(g * v1) >> 8]; // 8-bit input to 4-bit output
        b = gamma[(b * v1) >> 8];
    } else { // linear (uncorrected) color
        r = (r * v1) >> 12; // 4-bit results
        g = (g * v1) >> 12;
        b = (b * v1) >> 12;
    }
    */
    r = (r * v1) >> 8; // 8-bit results
    g = (g * v1) >> 8;
    b = (b * v1) >> 8;

    *red = r;
    *green = g;
    *blue = b;
}
/**
 * @function:    divoom_CreateHSVColor
 * @description: HSV TO RGB     result for Pdivoom_screen_color_info
 * @Author:      coronaa
 * @date:        2017-07-03
 * @param        hue            [description]
 * @param        saturation     [description]
 * @param        value          [description]
 * @param        color          [description]
 */
void divoom_CreateHSVColor(int32 hue, uint8 saturation, uint8 value, Pdivoom_screen_color_info color)
{
    uint8 r, g, b;
    divoom_hsvToRGB(hue, saturation, value, &r, &g, &b);

    color->r_c = r;
    color->g_c = g;
    color->b_c = b;     
}
/**
 * @function:    divoom_CreateHSV_Color
 * @description: divide into divisions part, then choose color  in setp:index 
 * @Author:      coronaa
 * @date:        2017-07-03
 * @param        divisions      [description]
 * @param        index          [description]
 * @param        saturation     [description]
 * @param        value          [description]
 * @param        color          [description]
 */
void divoom_CreateHSV_Color(uint16 divisions, uint16 index, uint8 saturation, uint8 value, Pdivoom_screen_color_info color)
{

    int32 hueAngle = (1536 * index) / divisions;
    divoom_CreateHSVColor(hueAngle, saturation, value, color);
}
/**
 * @function:    divoom_CreateAHSVColor
 * @description: 
 * @Author:      coronaa
 * @date:        2017-07-03
 * @param        divisions      [description]
 * @param        index          [description]
 * @param        saturation     [description]
 * @param        value          [description]
 * @param        color          [description]
 */
void divoom_CreateAHSVColor(uint16 divisions, uint16 index, uint8 saturation, uint8 value, Pdivoom_screen_color_info color)
{
    index %= divisions;
    divoom_CreateHSV_Color(divisions, index, saturation, value,color);
}
/**
 * @function:    divoom_CreateHSVColorWithDivisions
 * @description: 
 * @Author:      coronaa
 * @date:        2017-07-03
 * @param        divisions      [description]
 * @param        index          [description]
 * @param        color          [description]
 */
void divoom_CreateHSVColorWithDivisions(uint16 divisions, uint16 index, Pdivoom_screen_color_info color)
{
    divoom_CreateAHSVColor(divisions, index, 255, 255,color);
}
/**
 * @function:    divoom_CreateHSVColorWithArray
 * @description: 
 * @Author:      coronaa
 * @date:        2017-07-03
 * @param        array          [description]
 * @param        array_len      [description]
 * @param        index          [description]
 * @param        color          [description]
 */
void divoom_CreateHSVColorWithArray(uint16 *array, uint8 array_len, uint16 index, Pdivoom_screen_color_info color)
{
    index %= array_len;    
    divoom_CreateHSVColor(*(array + index), 255, 255,color);
}

void divoom_rgb_to_hsv(Pdivoom_screen_color_info pcolor, Pdivoom_disp_hsv_info phsv)
{
	divoom_RgbToHsv(pcolor->r_c, pcolor->g_c, pcolor->b_c, &phsv->h, &phsv->s, &phsv->v);
}

void divoom_hsv_to_rgb(Pdivoom_disp_hsv_info phsv, Pdivoom_screen_color_info pcolor)
{
	divoom_hsvToRGB(phsv->h, phsv->s, phsv->v, &pcolor->r_c, &pcolor->g_c, &pcolor->b_c);
}

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
void divoom_rgb_hsv_change_brightness(Pdivoom_screen_color_info fcolor, Pdivoom_screen_color_info tcolor, int index, T_FLOAT total_index)
{
	divoom_disp_hsv_info hsv = {0};
	T_FLOAT v_inter; //计算每个v变化的间隔值
	uint8 v_max = 0;
	uint8 v = 0;
	
	divoom_rgb_to_hsv(fcolor, &hsv);
	v_max = hsv.v;
	v_inter = (v_max/total_index);
	if (index <= total_index)
	{
		v = index * v_inter; //整数乘以一个浮点数，再取整
	}
	hsv.v = v;
	divoom_hsv_to_rgb(&hsv, tcolor);
}

/*********************************************************************
  Function:        divoom_rgb_hsv_change_rgb
  Description:    	改变色相
  Input:           	fcolor  原来的颜色rgb
  					tcolor  改变之后的颜色rgb
  					h	控制色相的改变系数,即改变颜色
  					s	改变颜色的深浅
  					v	改变颜色的亮度
  Return:           NONE
  Author:           wangft
  Data:            2019-6-19
**********************************************************************/
void divoom_rgb_hsv_change_rgb(Pdivoom_screen_color_info fcolor, Pdivoom_screen_color_info tcolor, int h, int s, int v)
{
	divoom_disp_hsv_info hsv = {0};
	
	divoom_rgb_to_hsv(fcolor, &hsv);

	if (h >= 0 )
		hsv.h = h;
	if (s >= 0)
		hsv.s = s;
	if (v >= 0)
		hsv.v = v;
	divoom_hsv_to_rgb(&hsv, tcolor);
}

