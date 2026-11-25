#ifndef _divoom_check_system_lock_h_

#define _divoom_check_system_lock_h_

void divoom_check_system_lock_init(void);
void divoom_check_system_lock_add(void *lock_handle, char *func_name, int func_line);
void divoom_check_system_lock_remove(void *lock_handle);
void divoom_check_system_lock_do_dead_lock(void *lock_handle, char *func_name, int func_line);
#endif


