#ifndef _divoom_malloc_h_
#define _divoom_malloc_h_

#include "divoom_type.h"

void *divoom_malloc(uint32 size, uint32 mode_id, uint32 model_line, const char *file_name);
//void *divoom_malloc(uint32 size, uint32 mode_id, const char *file_name, uint32 model_line)
void divoom_free_line(void *ptr, const char *file_name, int line);
/**
 * *  @brief       	divoom_remalloc
 				重新分配内存
 * *  @date       	2014-10-28
 * *  @param[in]    ptr
 * *  @return        void *
 * */

void *divoom_remalloc(void *ptr,uint32 size, uint32 model_line, const char *file_name);

#define divoom_free(ptr) divoom_free_line(ptr, __FILE__, __LINE__)
#define divoom_system_free(ptr) divoom_free_line(ptr, __FILE__, __LINE__)
#define divoom_system_remalloc(ptr, size) divoom_remalloc(ptr, size, __LINE__, __FILE__)

void divoom_malloc_check_ptr(void *ptr, int size, char *file_name, int line);


/*********************************************************************
  Function:       divoom_malloc_check_all_ram_in
  Description:	   进入模块之前检查内存分配情况
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2019-11-21
**********************************************************************/
void divoom_malloc_check_all_ram_in(void);
/*********************************************************************
  Function:       divoom_malloc_check_all_ram_out
  Description:	   退出模块之后检查内存分配情况
  Input:          NONE
  Return:         1 前后一样 0 前后不一样
  Author:         wangft
  Data:           2019-11-21
**********************************************************************/
uint8 divoom_malloc_check_all_ram_out(const char *func_name);
void divoom_malloc_debug_start_ctrl(uint8 flag);

void *divoom_malloc_raw(uint32 size, uint32 mode_id, uint32 model_line, const char *file_name, int zero_flag);

#endif

