#ifndef DIVOOM_HA_HTTP_H
#define DIVOOM_HA_HTTP_H

#define DIVOOM_HA_HTTP_RESPONSE_MAX (16 * 1024)

/*********************************************************************
  Function:        divoom_ha_http_post_form
  Description:     使用独立局域网连接向 HA /auth/token 发送表单，响应正文上限 16 KB。
  Input:           url：http://主机[:端口]/auth/token；form：已编码表单；http_status：HTTP 状态输出。
  Return:          正文由调用方 divoom_system_free；地址、网络、协议或内存错误返回 NULL。
  调用约束:         仅 HA 后台任务同步调用，不共享公共 HTTP 状态，不跟随重定向、不支持 HTTPS。
**********************************************************************/
char *divoom_ha_http_post_form(const char *url, const char *form, int *http_status);

#endif
