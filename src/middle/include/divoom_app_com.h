#ifndef _divoom_app_com_h_
#define _divoom_app_com_h_

#include "divoom_port_fs.h"

#define DIVOOM_BLOCK_SIZE (64*1024)
#define DIVOOM_PAGE_SIZE 256



/**
 * NAME               divoom_app_com_from_mode_get_file_name
 * *  @brief       	 通过mode得到文件名
 * *  @param[in] 	 	 mode    通过mode转换得到相应的文件
 * *  @return        返回 文件名
 * */

const char *divoom_app_com_from_mode_get_file_name(int mode);

/**
 * NAME               divoom_app_com_open_file
 * *  @brief       	 打开文件
 * *  @param[in] 	 	 mode    通过mode转换得到相应的文件
                     index     希望从文件的哪个位置开始操作
                     write_flag     是读这个文件,还是写这个文件
 * *  @return        返回 文件句柄
 * */

void *divoom_app_com_open_file(uint8 mode, uint32 index, uint8 write_flag);

/**
 * NAME               divoom_app_com_write_movie_file
 * *  @brief       	 按照前256字节写入文件总大小的格式保存成新文件
 * *  @param[in] 	 	 file_name   文件名
                     file_buf     保存到文件的数据
                     file_len			数据大小
                     tol_size     这个地方可能是总size+文字显示数据
                     head_len :tol_size数据长度
 * *  @return        none
 * */

void divoom_app_com_write_movie_file(char *file_name, char *file_buf,  int file_len, void *tol_size, int head_len);

/**
 * NAME               divoom_com_write_file
 * *  @brief       	 写文件
 * *  @param[in] 	file_name    文件名
                     file_len     文件长度
                     file_buf     文件缓冲区指针
 * *  @return        返回
 * */

void divoom_app_com_write_file(char *file_name, uint8 *file_buf, uint32 file_len);

char *divoom_app_com_read_file(char *file_name, int *file_len);
//返回文件大小
int divoom_app_com_get_file_size(char *file_name);

int divoom_app_com_close_file(void *fstream);
void divoom_app_com_del_file(char *file_name);
int divoom_app_com_del_dir(char  *dir_full_path);
int divoom_app_com_mk_dir(char  *dir_full_path);
//确认这个文件的所有目录已经创建
int divoom_app_com_check_and_mk_dir(char  *dir_full_path);
void divoom_app_com_mk_dir_for_file_path(const char *file_path);

int divoom_app_com_check_file_exist(char  *dir_full_path);
void divoom_exec_cmd(int8 *cmd, int8 *result, int buf_len);
int exec_command_system(const char *cmd);
/** 后台执行命令不等待（用于 arecord|aplay 等），避免 exec_command_system 的 2 秒超时杀进程 */
int exec_command_system_background(const char *cmd);
int divoom_app_com_check_check_sum(void *file_handle);
uint32 divoom_app_com_get_file_check_sum(void *file_handle, int *ret_size);
void divoom_app_com_rename_file(char *file_name, char *new_file_name);
void divoom_app_com_copy_file(char *src_file, char *dest_file);
char * divoom_app_com_copy_string(char *src_str, int len);
void divoom_app_com_strcpy(char *src_str, char *dest_str,int len);
int divoom_app_com_utf8_to_unicode(char* ppOutput,char* pInput, int max);
int divoom_app_com_unicode_to_utf8(uint16* pInput, int len,char* ppOutput, int max);
void divoom_com_print_buf(T_U8 *data, T_U16 len);
uint8 divoom_com_convert_vol(T_U8 vol_level);
int divoom_com_vol_ui_to_level(uint8 ui_vol);
uint8 divoom_com_vol_snap_ui(uint8 ui_vol);
uint8 divoom_com_vol_step_ui(uint8 ui_vol, int delta);
uint8 divoom_com_hfp_call_level_to_ui(uint8 level);
T_U32 divoom_app_com_get_check_sum(T_U8 *data, T_U16 len);
void divoom_app_com_finishcallback(int ret);

char * divoom_app_com_get_int_from_config(char *config, char *item_id, int *item_value);
char * divoom_app_com_get_float_from_config(char *config, char *item_id, float *item_value);
void divoom_app_com_float_to_str(char *buf, int buf_size, float val, int decimals);
void divoom_app_com_format_double(char *buf, int buf_size, double val, int decimals, int min_width, int force_plus, const char *suffix);
//得到文件的后缀名
void divoom_app_com_get_file_ex_name(char *file_name, char *ex_name);

char * divoom_app_com_get_str_from_config(char *config, char *item_id, char *item_value);
char * divoom_app_com_get_str_from_config_ex(char *config, char *item_id, char *item_value);
char * divoom_app_com_get_str_from_config_no_decode(char *config, char *item_id, char *item_value);

/*********************************************************************
  Function:        divoom_app_com_get_str_list_from_config
  Description:      从配置中获取指定字段类型的所有值
                    
  Para:            	config 配置
  					item_id 要获取的字段
                   	item_vaule  存储该字段的值
  Return:           个数
  Author:           wangft
**********************************************************************/

int divoom_app_com_get_str_list_from_config(char *config, char *item_id, char item_vaule[][48]);

/*********************************************************************
  Function:        divoom_app_com_skip_char
  Description:      跳过若干次同一个字符，返回后面位置
                    
  Para:             net_data:获取数据失败，1：获取数据成功
                    ch
                    time
  Return:           指针
  Author:           albert
**********************************************************************/

char *divoom_app_com_skip_char(char *src_str, char ch, int time);

/*********************************************************************
  Function:        divoom_app_com_get_string_stop_ch
  Description:     拷贝字符串，到某个结束
                    
  Para:             src_str
                    dest_str
                    ch
  Return:           指针
  Author:           albert
**********************************************************************/


char * divoom_app_com_get_string_stop_ch(char *src_str, char *dest_str, char ch);


/*********************************************************************
  Function:        divoom_app_com_image_get_cache_data
  Description:     拷贝相关图像数据
                    
  Para:             pcache_info
                    item
                    pos
  Return:           
  Author:           albert
**********************************************************************/

void divoom_app_com_image_get_cache_data(void *pcache_info, void *item, int pos);

/*********************************************************************
  Function:        divoom_app_com_large_pic
  Description:     放大图像
                    
  Para:             rgb_data
                    raw_width
                    new_width
  Return:           
  Author:           albert
**********************************************************************/


void divoom_app_com_large_pic(uint8 *rgb_data, int raw_width, int new_width);

/*********************************************************************
  Function:        divoom_esp_jpeg_decode
  Description:     对JPEG进行解码
                    
  Para:             image_data
                    image_size
                    disp_pic_buf
  Return:           
  Author:           albert
**********************************************************************/

void divoom_esp_jpeg_decode(uint8 *image_data, int image_size, uint8 *disp_pic_buf);

/*********************************************************************
  Function:        divoom_app_com_get_file_sha1sum
  Description:     得到文件的SHA256的值
                    
  Para:             file_name
                    sha1sum_info
  Return:           
  Author:           albert
**********************************************************************/

void divoom_app_com_get_file_sha1sum(const char* file_name, char *sha1sum_info);

/*********************************************************************
  Function:        divoom_app_com_get_resouce_type
  Description:     得到资源文件的类型
                    
  Para:             file_name
  Return:           文件类型
**********************************************************************/

int divoom_app_com_get_resouce_type(char *file_name);
int divoom_app_com_get_local_resouce_type(char *file_name);

/*********************************************************************
  Function:        divoom_app_com_get_rand_index
  Description:     得到一个不同的随机数
                    
  Para:             cur_index
                    max_index
  Return:           随机数
**********************************************************************/

int divoom_app_com_get_rand_index(int cur_index, int max_index);

/*********************************************************************
  Function:        divoom_app_com_set_image_opa
  Description:     修改图像某个区域的OPA值
                    
  Para:             image_buf    rgba缓冲区
                    x
                    y
                    width
                    high
                    opa_value
                    screen_w
                    screen_h
  Return:           none
**********************************************************************/

void divoom_app_com_set_image_opa(uint8 *image_buf, int x, int y, int width, int high, int opa_value, int screen_w, int screen_h);

/*********************************************************
  * FUNCTION		: divoom_app_com_convert_rgb_bgra
  * 		        : 将RGB转化为BGRA方式
  * IUPUT			src_rgb
                    dest_rgb
                    src_w
                    src_h
  * OUTPUT			none
  *********************************************************/


void divoom_app_com_convert_rgb_bgra(uint8 *src_rgb, uint8 *dest_rgb, int src_w, int src_h);

/*********************************************************
  * FUNCTION		: divoom_app_com_abs_value
  * 		        取两个数差的绝对值
  * IUPUT			value1
                    value2
  * OUTPUT			abs value
  *********************************************************/

double divoom_app_com_abs_value(double value1, double value2);
/**
 * 过滤UTF-8字符串中的表情符号
 * @param in_info 输入的UTF-8字符串
 * @param out_info 输出的过滤后的UTF-8字符串
 */
void divoom_app_com_filter_emj(char *in_info, char *out_info);

/**
 * 版本2：创建新字符串（更安全的方式）
 * @param in_str 输入字符串
 * @param out_str 输出字符串
 * @param out_size 输出缓冲区大小
 * @return 处理后的字符串长度，如果缓冲区不足返回-1
 */
int divoom_app_com_add_escape_quote_safe(const char *in_str, char *out_str, size_t out_size);

/**
* NAME         divoom_app_com_rgb888_to_rgb565
* @BRIEF	将RGB888缓冲区转换为565
* @PARAM	dest_buf          
                    src_buf   
                    image_size
* @RETURN	 NONE
* @RETVAL
*/

void divoom_app_com_rgb888_to_rgb565(uint8 *dest_buf, uint8 *src_buf, int image_size);

#endif
