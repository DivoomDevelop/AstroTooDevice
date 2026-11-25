#ifndef DIVOOM_CLOCK_ATO_RESOURCES_H
#define DIVOOM_CLOCK_ATO_RESOURCES_H

#include "divoom_type.h"

#define DIVOOM_CLOCK_ATO_ID 3499
#define DIVOOM_CLOCK_ATO_RESOURCE_ROOT "/userdata/ai_emoji/3499"
/* 资源包是497图片元素管理的外部文件；不编入固件或预装资源分区。 */

/* 显示worker非阻塞请求：准备中返回0，就绪返回1并取临时引用，用后必须release。 */
int divoom_clock_ato_resources_request(const char *package_path);
/* 同步安装入口：仅资源worker或安装验证调用，不能在显示/按键路径使用。 */
int divoom_clock_ato_resources_acquire(const char *package_path);
/* Buddy在LVGL锁内接收引用，retain/release不执行文件I/O。 */
int divoom_clock_ato_resources_retain(void);
void divoom_clock_ato_resources_release(void);
/* 删除表盘时请求清理；有引用时保留目录，引用归零后由已有worker的poll完成。 */
int divoom_clock_ato_resources_remove(void);
void divoom_clock_ato_resources_poll(void);
/* 仅映射ato/相对路径；lvgl_path用于Windows PNG解码器的A:驱动前缀。 */
int divoom_clock_ato_resources_path(const char *relative, char *out, uint32 capacity, int lvgl_path);

#endif
