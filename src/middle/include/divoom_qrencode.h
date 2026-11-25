#ifndef _DIVOOM_QRENCODE_H_
#define _DIVOOM_QRENCODE_H_

typedef struct _divoom_qrencode_data
{
	uint8 pic_width;
	uint8 pic_high;
	uint8 *rgb_data;
}divoom_qrencode_data, *Pdivoom_qrencode_data;

/*
  Function:       divoom_qrencode_string
  Description:	  对字符串生成二维码
  Input:          string 字符串
  Return:         	返回一张rgb图片
  Author:         wangft
  Data:           2022-3-3
*/

Pdivoom_qrencode_data divoom_qrencode_string(char *string);

#endif

