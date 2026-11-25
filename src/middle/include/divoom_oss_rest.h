#ifndef _divoom_oss_rest_h_
#define _divoom_oss_rest_h_

/*********************************************************************
  Function:        divoom_oss_rest_get_object_to_file
  Description:     通过 OSS REST API (GetObject) 下载对象到本地文件
  Input:           object_name  OSS 对象键
                   local_filename  本地保存路径
                   server_flag  0=国内  1=美国
  Return:          1 成功  0 失败
**********************************************************************/
int divoom_oss_rest_get_object_to_file(char *object_name, char *local_filename, int server_flag);

/*********************************************************************
  Function:        divoom_oss_rest_head_object_length
  Description:     通过 OSS REST API (HeadObject) 获取对象大小
  Input:           object_name  OSS 对象键
                   server_flag  0=国内  1=美国
  Return:          Content-Length，失败返回 0
**********************************************************************/
int divoom_oss_rest_head_object_length(char *object_name, int server_flag);

#endif
