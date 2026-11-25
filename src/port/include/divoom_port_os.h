#ifndef DIVOOM_PORT_OS_H
#define DIVOOM_PORT_OS_H

#include "divoom_type.h"

void *divoom_port_sem_create(int max_count, int init_count);
void divoom_port_sem_wait(void *handle);
int divoom_port_sem_timedwait_s(void *handle, int time_s);
void divoom_port_sem_post(void *handle);
void divoom_port_sem_destroy(void *handle);

void *divoom_port_mutex_create(void);
void divoom_port_mutex_lock(void *handle);
int divoom_port_mutex_timedlock_s(void *handle, int timeout_sec);
int divoom_port_mutex_trylock(void *handle);
void divoom_port_mutex_unlock(void *handle);
void divoom_port_mutex_destroy(void *handle);

/*
 * 阻塞消息队列。真机走 hal_queue；模拟器为空实现（create 返回 NULL）。
 * send/recv 返回 0 成功，非 0 失败；timeout_ms=0 表示不等待。
 * 字节流 UART 见 divoom_port_uart.h；模拟器替换的是 JL 设备行为，不是假串口。
 */
void divoom_port_critical_enter(void);
void divoom_port_critical_exit(void);

void *divoom_port_queue_create(const char *name, unsigned item_size, unsigned depth);
int divoom_port_queue_send(void *handle, void *item, int timeout_ms);
int divoom_port_queue_recv(void *handle, void *item, int timeout_ms);
int divoom_port_queue_len(void *handle);
void divoom_port_queue_destroy(void *handle);

void *divoom_port_thread_create(void *(*func)(void *), void *arg);
void *divoom_port_thread_create_ex(void *(*func)(void *), void *arg, const char *task_name, int stack_size, int prio);
void *divoom_port_thread_create_joinable_ex(void *(*func)(void *), void *arg,
					    const char *task_name, int stack_size, int prio);
int divoom_port_thread_join(void *handle);
void divoom_port_thread_detach(void *handle);
void *divoom_port_thread_self(void);
int divoom_port_thread_equal(void *a, void *b);
long divoom_port_thread_stack_free_bytes(void);

void divoom_port_tick_reset_epoch(void);
uint32 divoom_port_tick_ms(void);
uint32 divoom_port_tick_s(void);
uint32 divoom_port_utc_time(void);
void divoom_port_delay_ms(uint32 time_ms);
void divoom_port_yield(void);

int divoom_port_task_get_current_prio(void);
void divoom_port_task_set_current_prio(int prio);
void divoom_port_task_set_prio(void *thread, int prio);

long divoom_port_heap_free_kb(void);
long divoom_port_heap_total_kb(void);
long divoom_port_heap_largest_free_block_kb(void);

#endif /* DIVOOM_PORT_OS_H */
