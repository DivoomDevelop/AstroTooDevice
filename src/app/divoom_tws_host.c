#include "divoom.h"
#include "divoom_btcore_uart_cmd.h"
#include "divoom_tws_host.h"
#include "divoom_lvgl_top_audio_tip_menu.h"

typedef struct
{
    divoom_tws_snap_t snap;
    uint8 sent_listen;
    void *lock;
} divoom_tws_host_ctrl_t;

static divoom_tws_host_ctrl_t s_tws = {0};

static void divoom_tws_host_ensure(void)
{
    if (s_tws.lock == NULL)
    {
        s_tws.lock = divoom_system_create_lock();
    }
}

static void divoom_tws_host_lock(void)
{
    divoom_tws_host_ensure();
    if (s_tws.lock)
    {
        divoom_system_get_lock(s_tws.lock);
    }
}

static void divoom_tws_host_unlock(void)
{
    if (s_tws.lock)
    {
        divoom_system_free_lock(s_tws.lock);
    }
}

void divoom_tws_host_on_snap(const uint8 *data, uint16 len)
{
    uint8 prev_connected;
    uint8 now_connected;
    uint8 role;

    if (data == NULL || len < DIVOOM_BTCORE_TWS_SNAP_LEN)
    {
        return;
    }
    divoom_tws_host_lock();
    prev_connected = (s_tws.snap.valid
        && s_tws.snap.state == DIVOOM_BTCORE_TWS_ST_CONNECTED) ? 1 : 0;
    s_tws.snap.state = data[0];
    s_tws.snap.role = data[1];
    s_tws.snap.flags = data[2];
    s_tws.snap.remain_sec = data[3];
    s_tws.snap.fail_reason = data[4];
    s_tws.snap.valid = 1;
    /* 以杰理快照为准。未配对允许再开等待；已经在等待则不要每圈重发。 */
    if (s_tws.snap.state == DIVOOM_BTCORE_TWS_ST_UNPAIRED)
    {
        s_tws.sent_listen = 0;
    }
    else if (s_tws.snap.state == DIVOOM_BTCORE_TWS_ST_LISTEN)
    {
        s_tws.sent_listen = 1;
    }
    now_connected = (s_tws.snap.state == DIVOOM_BTCORE_TWS_ST_CONNECTED) ? 1 : 0;
    role = s_tws.snap.role;
    divoom_tws_host_unlock();

    if (now_connected && !prev_connected)
    {
        if (role == DIVOOM_BTCORE_TWS_ROLE_LEFT)
            divoom_lvgl_top_audio_tip_menu_init(DIVOOM_AUDIO_TIP_TWS_CONNECTED_LEFT);
        else if (role == DIVOOM_BTCORE_TWS_ROLE_RIGHT)
            divoom_lvgl_top_audio_tip_menu_init(DIVOOM_AUDIO_TIP_TWS_CONNECTED_RIGHT);
    }
}

void divoom_tws_host_get_snap(divoom_tws_snap_t *out)
{
    if (out == NULL)
    {
        return;
    }
    divoom_tws_host_lock();
    *out = s_tws.snap;
    divoom_tws_host_unlock();
}

int divoom_tws_host_send(uint8 func, uint8 arg)
{
    uint8 payload[2];
    divoom_tws_snap_t before;
    int ret;

    divoom_tws_host_get_snap(&before);
    if ((before.flags & DIVOOM_BTCORE_TWS_F_CALL)
        && ((func == DIVOOM_BTCORE_TWS_FUNC_PAIR && arg == DIVOOM_BTCORE_TWS_ARG_ON)
            || func == DIVOOM_BTCORE_TWS_FUNC_UNPAIR))
    {
        return -2;
    }
    payload[0] = func;
    payload[1] = arg;
    /* 不等应答。0x69 / 0x6B 在串口线程写入快照，页面定时器再改字。 */
    ret = divoom_btcore_uart_cmd_send(DM_BTCORE_TWS_PAIR_CTRL, payload, 2);
    if (ret != 0)
    {
        return ret;
    }

    divoom_tws_host_lock();
    if (func == DIVOOM_BTCORE_TWS_FUNC_LISTEN && arg == DIVOOM_BTCORE_TWS_ARG_ON)
    {
        s_tws.sent_listen = 1;
    }
    if ((func == DIVOOM_BTCORE_TWS_FUNC_LISTEN && arg == DIVOOM_BTCORE_TWS_ARG_OFF)
        || func == DIVOOM_BTCORE_TWS_FUNC_UNPAIR)
    {
        s_tws.sent_listen = 0;
    }
    /* 解除后清记录，停在未配对。连着手机也不再自动进入等待。 */
    if (func == DIVOOM_BTCORE_TWS_FUNC_UNPAIR && s_tws.snap.valid)
    {
        s_tws.snap.state = DIVOOM_BTCORE_TWS_ST_UNPAIRED;
        s_tws.snap.role = DIVOOM_BTCORE_TWS_ROLE_NONE;
        s_tws.snap.flags &= (uint8)~DIVOOM_BTCORE_TWS_F_PAIRED;
        s_tws.snap.remain_sec = 0;
        s_tws.snap.fail_reason = DIVOOM_BTCORE_TWS_FAIL_NONE;
    }
    /* 0x69 应答会再写一遍。先改本地，页面不用等这一帧。 */
    if (func == DIVOOM_BTCORE_TWS_FUNC_POWER_SYNC && s_tws.snap.valid)
    {
        if (arg == DIVOOM_BTCORE_TWS_ARG_ON)
        {
            s_tws.snap.flags |= DIVOOM_BTCORE_TWS_F_POWER_SYNC;
        }
        else if (arg == DIVOOM_BTCORE_TWS_ARG_OFF)
        {
            s_tws.snap.flags &= (uint8)~DIVOOM_BTCORE_TWS_F_POWER_SYNC;
        }
    }
    divoom_tws_host_unlock();
    return 0;
}

void divoom_tws_host_page_poll(void)
{
    divoom_tws_snap_t snap;
    uint8 sent;

    divoom_tws_host_get_snap(&snap);
    if (!snap.valid)
    {
        return;
    }
    /* 快照已是等待就停。只有杰理报未配对才补发，搜索、已连接、回连不发。 */
    if (snap.state == DIVOOM_BTCORE_TWS_ST_LISTEN)
    {
        return;
    }
    if (snap.state != DIVOOM_BTCORE_TWS_ST_UNPAIRED)
    {
        return;
    }
    if (snap.flags & (DIVOOM_BTCORE_TWS_F_PAIRED | DIVOOM_BTCORE_TWS_F_PHONE
        | DIVOOM_BTCORE_TWS_F_CALL))
    {
        return;
    }
    divoom_tws_host_lock();
    sent = s_tws.sent_listen;
    divoom_tws_host_unlock();
    if (sent)
    {
        return;
    }
    divoom_tws_host_send(DIVOOM_BTCORE_TWS_FUNC_LISTEN, DIVOOM_BTCORE_TWS_ARG_ON);
}

void divoom_tws_host_on_page_enter(void)
{
    divoom_tws_snap_t snap;

    divoom_tws_host_lock();
    s_tws.sent_listen = 0;
    divoom_tws_host_unlock();

    divoom_tws_host_get_snap(&snap);
    if (!snap.valid)
    {
        return;
    }
    if (snap.flags & DIVOOM_BTCORE_TWS_F_PAIRED)
    {
        return;
    }
    if (snap.state != DIVOOM_BTCORE_TWS_ST_UNPAIRED)
    {
        return;
    }
    if (snap.flags & DIVOOM_BTCORE_TWS_F_PHONE)
    {
        return;
    }
    divoom_tws_host_send(DIVOOM_BTCORE_TWS_FUNC_LISTEN, DIVOOM_BTCORE_TWS_ARG_ON);
}

void divoom_tws_host_on_page_leave(void)
{
    divoom_tws_snap_t snap;
    uint8 sent;

    divoom_tws_host_get_snap(&snap);
    divoom_tws_host_lock();
    sent = s_tws.sent_listen;
    s_tws.sent_listen = 0;
    divoom_tws_host_unlock();

    if (!sent && snap.state != DIVOOM_BTCORE_TWS_ST_SEARCH)
    {
        return;
    }
    if (snap.flags & DIVOOM_BTCORE_TWS_F_PAIRED)
    {
        return;
    }
    if (snap.state == DIVOOM_BTCORE_TWS_ST_CONNECTED)
    {
        return;
    }
    if (divoom_tws_host_send(DIVOOM_BTCORE_TWS_FUNC_LISTEN, DIVOOM_BTCORE_TWS_ARG_OFF) != 0)
    {
        divoom_tws_host_lock();
        s_tws.sent_listen = 1;
        divoom_tws_host_unlock();
    }
}

int divoom_tws_host_is_right_linked(void)
{
    return divoom_tws_host_linked_role() == DIVOOM_BTCORE_TWS_ROLE_RIGHT;
}

uint8 divoom_tws_host_linked_role(void)
{
    divoom_tws_snap_t snap;

    divoom_tws_host_get_snap(&snap);
    if (!snap.valid || snap.state != DIVOOM_BTCORE_TWS_ST_CONNECTED)
    {
        return DIVOOM_BTCORE_TWS_ROLE_NONE;
    }
    if (snap.role == DIVOOM_BTCORE_TWS_ROLE_LEFT
        || snap.role == DIVOOM_BTCORE_TWS_ROLE_RIGHT)
    {
        return snap.role;
    }
    return DIVOOM_BTCORE_TWS_ROLE_NONE;
}
