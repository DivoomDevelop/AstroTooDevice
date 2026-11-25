#ifndef _divoom_alarm_com_h
#define _divoom_alarm_com_h

#include <string.h>
#include <stdio.h>
#include "divoom_alarm_mode.h"

T_VOID divoom_com_init_in_alarm(Pdivoom_alarm_callback Pcallback);

T_VOID divoom_com_deinit(T_VOID);

//T_VOID divoom_com_print(T_CHR *format, ...);

T_U32 divoom_com_config_read(T_U8 mode, T_VOID *buf, T_U8 index, int size);

T_U32 divoom_com_config_write(T_U8 mode, T_VOID*buf, T_U8 index, int size);

T_VOID divoom_com_config_del_one(T_U8 mode, T_U8 index);

T_VOID divoom_com_config_del_all(T_U8 mode);

T_pVOID divoom_com_malloc_in_alarm(T_U32 size, T_CHR*file_name, int line);

T_VOID divoom_com_free_in_alarm(T_pVOID var);

T_U32 divoom_com_get_sec(T_VOID);

T_U32 divoom_com_time_to_sec(Pdivoom_alarm_system_date data);

T_VOID divoom_com_sec_to_time(Pdivoom_alarm_system_date data, T_U32 second);

T_VOID divoom_com_get_date(Pdivoom_alarm_system_date date);

//更改闹钟类型,通过参数看是设置还是获取,跟系统参数有关,参数DIVOOM_ALARM_GET_STATUS，返回1,是开机闹钟，0不是，DIVOOM_ALARM_SET_STATUS,设置为非开机闹钟，改为普通闹钟，可以不关心返回值
T_U8 divoom_com_power_on_status(DIVOOM_ALARM_STATUS flag, T_U8 alarm_power_type);

T_VOID divoom_com_unactive_alarm(T_U8 *alarm_id_list, T_U8 alarm_id_list_len);

T_VOID divoom_com_need_do(T_VOID);

T_VOID divoom_com_set_rtc_time(T_U8 flag, T_DIVOOM_ALARM_ACTIVE_DO_INFO *date, DIVOOM_ALARM_ACTIVE_CALLBACK active_on);

T_VOID divoom_com_active_do(T_DIVOOM_ALARM_ACTIVE_DO_INFO *active_do_info);

T_VOID divoom_com_lock_init(T_VOID);

T_VOID divoom_com_lock_deinit(T_VOID);

T_VOID divoom_com_lock_lock(T_VOID);

T_VOID divoom_com_lock_unlock(T_VOID);

#endif



