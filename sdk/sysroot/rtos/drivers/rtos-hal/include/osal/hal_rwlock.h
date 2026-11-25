#ifndef SUNXI_HAL_RWLOCK_H
#define SUNXI_HAL_RWLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <hal_mutex.h>
#include <hal_sem.h>
#include <hal_time.h>
#include <stdint.h>
#include <stddef.h>

typedef struct hal_rwlock {
	struct hal_sem r_lock;
	struct hal_sem w_lock;
	struct hal_mutex lock;
	volatile int waiting_readers;
	volatile int reader;
	volatile int waiting_writers;
	volatile int writer;
	char name[32];
	char *write_thread_name;
} hal_rwlock_t;

/**
 * Create a rwlock
 *
 * Return pointer to hal_rwlock_t on success, or NULL on failure.
 */
hal_rwlock_t *hal_rwlock_create(unsigned char *name);

/**
 * delete a rwlock
 *
 * @rwlock: a point that create by hal_rwlock_create
 *
 * Return HAL_OK on success
 */
int hal_rwlock_delete(hal_rwlock_t *rwlock);

/**
 * acquire a rwlock reader lock
 *
 * @rwlock: a rwlock pointer
 * @ticks: timeout ticks, can use MS_TO_OSTICK to convert ms to ticks
 *
 * Return HAL_OK on success, or HAL_TIMEOUT on timeout.
 */
int hal_rwlock_reader_lock_timedwait(hal_rwlock_t *rwlock, int ticks);

#define hal_rwlock_reader_lock(rwlock)		hal_rwlock_reader_lock_timedwait(rwlock, HAL_WAIT_FOREVER)
#define hal_rwlock_reader_trylock(rwlock)	hal_rwlock_reader_lock_timedwait(rwlock, 0)

/**
 * release a rwlock reader reader lock
 *
 * @rwlock: a rwlock pointer
 * @ticks: timeout ticks, can use MS_TO_OSTICK to convert ms to ticks
 *
 * Return HAL_OK on success, or HAL_TIMEOUT on timeout.
 */
int hal_rwlock_reader_unlock_timedwait(hal_rwlock_t *rwlock, int ticks);

#define hal_rwlock_reader_unlock(rwlock)	hal_rwlock_reader_unlock_timedwait(rwlock, HAL_WAIT_FOREVER)
#define hal_rwlock_reader_tryunlock(rwlock) hal_rwlock_reader_unlock_timedwait(rwlock, 0)

/**
 * acquire a rwlock writer lock
 *
 * @rwlock: a rwlock pointer
 * @ticks: timeout ticks, can use MS_TO_OSTICK to convert ms to ticks
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_rwlock_writer_lock_timedwait(hal_rwlock_t *rwlock, int ticks);

#define hal_rwlock_writer_lock(rwlock)		hal_rwlock_writer_lock_timedwait(rwlock, HAL_WAIT_FOREVER)
#define hal_rwlock_writer_trylock(rwlock)	hal_rwlock_writer_lock_timedwait(rwlock, 0)

/**
 * release a rwlock writer lock
 *
 * @rwlock: a rwlock pointer
 *
 * Return HAL_OK on success, or HAL_ERROR on failure
 */
int hal_rwlock_writer_unlock_timedwait(hal_rwlock_t *rwlock, int ticks);
#define hal_rwlock_writer_unlock(rwlock) hal_rwlock_writer_unlock_timedwait(rwlock, HAL_WAIT_FOREVER)

void show_rwlock(hal_rwlock_t *rwlock);
#ifdef __cplusplus
}
#endif

#endif
