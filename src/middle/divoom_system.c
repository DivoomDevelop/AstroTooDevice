#include "divoom.h"
#include "divoom_port_os.h"
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>

void *divoom_system_create_sem(int max_count, int init_count)
{
	return divoom_port_sem_create(max_count, init_count);
}

void divoom_system_get_sem(void *handle)
{
	divoom_port_sem_wait(handle);
}

int divoom_system_get_sem_with_time(void *handle, int time_s)
{
	return divoom_port_sem_timedwait_s(handle, time_s);
}

void divoom_system_put_sem(void *handle)
{
	divoom_port_sem_post(handle);
}

void divoom_system_destroy_sem(void *handle)
{
	divoom_port_sem_destroy(handle);
}

void *divoom_system_create_lock(void)
{
	return divoom_port_mutex_create();
}

void divoom_system_get_lock_direct(void *handle)
{
	divoom_port_mutex_lock(handle);
}

void divoom_system_get_lock_inner(void *handle, const char *func_name, int line)
{
	int i, rc, cur_ms = divoom_system_get_tickms();

	for (i = 0; i < 6; i++) {
		rc = divoom_port_mutex_timedlock_s(handle, 20);
		if (rc == 0)
			break;
		divoom_print("try lock and check: %s:%d\n", func_name, line);
	}
	if (rc) {
		divoom_print("err: %d, %d, %d\n", rc, divoom_system_get_tickms(), cur_ms);
		divoom_check_system_lock_do_dead_lock(handle, func_name, line);
		divoom_port_mutex_lock(handle);
	}
	divoom_check_system_lock_add(handle, func_name, line);
}

int divoom_system_try_lock(void *handle)
{
	return divoom_port_mutex_trylock(handle);
}

void divoom_system_free_lock_direct(void *handle)
{
	divoom_port_mutex_unlock(handle);
}

void divoom_system_free_lock(void *handle)
{
	/* Retire this owner's record before another thread can acquire the lock. */
	divoom_check_system_lock_remove(handle);
	divoom_port_mutex_unlock(handle);
}

void divoom_system_reboot(void)
{
#if DIVOOM_BUILD_SIMULATOR
	/* 仿真重启通知由 UI 主循环消费，不能在 worker 退出宿主系统。 */
	extern void divoom_product_test_format_sim_reboot(void);
	divoom_product_test_format_sim_reboot();
#endif
#if DIVOOM_BUILD_RTOS_DEVICE
	extern int cmd_reboot(int argc, char **argv);

	cmd_reboot(1, NULL);
#endif
}

void divoom_system_destroy_lock(void *handle)
{
	divoom_port_mutex_destroy(handle);
	divoom_check_system_lock_remove(handle);
}

void *divoom_system_create_new_task(DIVOOM_TASK_CALLBACK func, void *para, const char *func_name, int line)
{
	(void)line;
	return divoom_system_create_task(func, func_name ? func_name : "task", para,
					 DIVOOM_SYSTEM_TASK_DEFAULT_STACK_SIZE);
}

void *divoom_system_create_task(DIVOOM_TASK_CALLBACK func, const char *task_name, void *para, int stack_size)
{
	return divoom_system_create_task_prio(func, task_name, para, stack_size,
					      DIVOOM_SYSTEM_TASK_DEFAULT_PRIO);
}

void *divoom_system_create_task_prio(DIVOOM_TASK_CALLBACK func, const char *task_name, void *para, int stack_size, int prio)
{
	pthread_t task_handle = (pthread_t)(uintptr_t)divoom_port_thread_create_ex(func, para, task_name, stack_size, prio);

	divoom_print("create thread: %lu name: %s stack: %d prio: %d\n",
		     (unsigned long)task_handle, task_name ? task_name : "", stack_size, prio);
	return (void *)(uintptr_t)task_handle;
}

void *divoom_system_create_joinable_task(DIVOOM_TASK_CALLBACK func,
	const char *task_name, void *para, int stack_size)
{
	pthread_t task_handle = (pthread_t)(uintptr_t)
		divoom_port_thread_create_joinable_ex(func, para, task_name, stack_size,
			DIVOOM_SYSTEM_TASK_DEFAULT_PRIO);

	divoom_print("create joinable thread: %lu name: %s stack: %d prio: %d\n",
		     (unsigned long)task_handle, task_name ? task_name : "", stack_size,
		     DIVOOM_SYSTEM_TASK_DEFAULT_PRIO);
	return (void *)(uintptr_t)task_handle;
}

int divoom_system_join_task(void *handle)
{
	return divoom_port_thread_join(handle);
}

void *divoom_system_thread_self(void)
{
	return divoom_port_thread_self();
}

int divoom_system_thread_equal(void *a, void *b)
{
	return divoom_port_thread_equal(a, b);
}

void divoom_system_destroy_task(void *handle)
{
	(void)handle;
	pthread_exit("Divoom exit thread!\n");
}

int divoom_system_start_time_s = 0;

uint32 divoom_system_get_tickcount(void)
{
	return divoom_system_get_tickms();
}

void divoom_system_init_system_time(void)
{
	struct timeval time_info;

	gettimeofday(&time_info, NULL);
	divoom_system_start_time_s = time_info.tv_sec;
	divoom_port_tick_reset_epoch();
}

uint32 divoom_system_get_tickms(void)
{
	return divoom_port_tick_ms();
}

uint32 divoom_system_get_ticks(void)
{
	return divoom_port_tick_s();
}

uint32 divoom_system_get_utc_time(void)
{
	return divoom_port_utc_time();
}

uint32 divoom_system_set_interva_ms(uint32 time)
{
	return time;
}

void divoom_system_delay_ms(uint32 time)
{
	divoom_port_delay_ms(time);
}

void divoom_system_yield(void)
{
	divoom_port_yield();
}

int divoom_system_task_get_current_prio(void)
{
	return divoom_port_task_get_current_prio();
}

void divoom_system_task_set_current_prio(int prio)
{
	divoom_port_task_set_current_prio(prio);
}

void divoom_system_task_set_prio(void *task, int prio)
{
	divoom_port_task_set_prio(task, prio);
}

long divoom_system_get_ram_free_size(void)
{
	return divoom_port_heap_free_kb();
}

long divoom_system_get_ram_total_size(void)
{
	return divoom_port_heap_total_kb();
}

long divoom_system_get_ram_largest_free_block_size(void)
{
	return divoom_port_heap_largest_free_block_kb();
}

void divoom_system_print_ram_info(const char *file_name, int file_line)
{
#if DIVOOM_BUILD_RTOS_DEVICE
	divoom_print("%s:%d free heap %ld kb, largest block %ld kb, total %ld kb\n",
		     file_name, file_line,
		     divoom_system_get_ram_free_size(),
		     divoom_system_get_ram_largest_free_block_size(),
		     divoom_system_get_ram_total_size());
#else
	(void)file_name;
	(void)file_line;
#endif
}
