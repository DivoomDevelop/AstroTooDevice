#ifndef _DIVOOM_SLS_LOG_H_
#define _DIVOOM_SLS_LOG_H_

#include "divoom_type.h"

#define DIVOOM_SLS_LOG_OK                  0
#define DIVOOM_SLS_LOG_ERR_PARAM          -1
#define DIVOOM_SLS_LOG_ERR_NOT_INIT       -2
#define DIVOOM_SLS_LOG_ERR_ALREADY_INIT   -3
#define DIVOOM_SLS_LOG_ERR_MALLOC         -4
#define DIVOOM_SLS_LOG_ERR_QUEUE_FULL     -5

#define DIVOOM_SLS_LOG_LAST_ERROR_LEN     160

/*
  Function:         divoom_sls_log_status
  Description:      异步上传模块运行状态，用于调试队列堆积、丢弃和最近一次上传结果。
*/
typedef struct _divoom_sls_log_status
{
    uint8 initialized;
    uint8 thread_running;
    uint32 queued_count;
    uint32 enqueue_count;
    uint32 sent_count;
    uint32 failed_count;
    uint32 dropped_count;
    int32 last_http_code;
    char last_error[DIVOOM_SLS_LOG_LAST_ERROR_LEN];
} divoom_sls_log_status;

/*
  Function:         divoom_sls_log_init
  Description:      初始化 SLS 异步日志模块并启动后台发送线程。
  Return:           DIVOOM_SLS_LOG_OK 成功，其它为错误码。
*/
int divoom_sls_log_init(void);

/*
  Function:         divoom_sls_log_upload
  Description:      主动提交一条日志到异步队列；函数不做网络 I/O，可在业务线程调用。
  Input:            module/message 两个日志字段；level 固定为 info。
  Return:           DIVOOM_SLS_LOG_OK 表示已入队，其它为错误码。
*/
int divoom_sls_log_upload(const char *module, const char *message);

/*
  Function:         divoom_sls_log_get_status
  Description:      获取当前队列与最近一次上传状态。
  Input:            status 输出状态结构体。
  Return:           DIVOOM_SLS_LOG_OK 成功，其它为错误码。
*/
int divoom_sls_log_get_status(divoom_sls_log_status *status);

/*
  Function:         divoom_sls_log_deinit
  Description:      停止后台发送线程并释放队列中未上传日志；通常只在模拟器退出或重新配置时调用。
*/
void divoom_sls_log_deinit(void);

#endif
