#ifndef DIVOOM_TWS_HOST_H
#define DIVOOM_TWS_HOST_H

#include "divoom_type.h"

/* 杰理 0x6B 推来的 5 字节快照。界面只读它，不自己计时。 */
typedef struct
{
    uint8 state;
    uint8 role;
    uint8 flags;
    uint8 remain_sec;
    uint8 fail_reason;
    uint8 valid;
} divoom_tws_snap_t;

void divoom_tws_host_on_snap(const uint8 *data, uint16 len);
void divoom_tws_host_get_snap(divoom_tws_snap_t *out);
int divoom_tws_host_send(uint8 func, uint8 arg);
void divoom_tws_host_on_page_enter(void);
void divoom_tws_host_on_page_leave(void);
void divoom_tws_host_page_poll(void);
int divoom_tws_host_is_right_linked(void);
/* 已连上对箱：1=左声道 2=右声道；未连上为 0 */
uint8 divoom_tws_host_linked_role(void);

#endif
