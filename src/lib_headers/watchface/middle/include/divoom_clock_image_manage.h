#ifndef DIVOOM_CLOCK_IMAGE_MANAGE_H
#define DIVOOM_CLOCK_IMAGE_MANAGE_H

/*********************************************************************
  Function:        divoom_clock_image_manage_init
  Description:     管理从外部url下载的图片
  Input:           void
  Author:          wangft
  Return:          NONE
**********************************************************************/
void divoom_clock_image_manage_init(void);

// 更新图片地址
int divoom_clock_image_manage_update_addr(int clock_id, int index, char *new_addr);

// 获取图片地址
char *divoom_clock_image_manage_get_addr(int clock_id, int index);

void divoom_clock_image_manage_get_addr_by_id(int image_id, char *url_addr);

// 释放资源
void divoom_clock_image_manage_deinit(void);

int divoom_clock_image_manage_get_image_id(int clock_id, int index);

#endif // DIVOOM_CLOCK_IMAGE_manage_H 
