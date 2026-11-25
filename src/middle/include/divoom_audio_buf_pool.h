#ifndef _divoom_audio_buf_pool_h_
#define _divoom_audio_buf_pool_h_

#include "divoom_build_config.h"
#include "divoom_type.h"

typedef enum {
	DIVOOM_AUDIO_BUS_SLEEP = 0,
	DIVOOM_AUDIO_BUS_TONE,
#if DIVOOM_SHOW_TF_NAND
	DIVOOM_AUDIO_BUS_MUSIC,
#endif
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
	DIVOOM_AUDIO_BUS_NETWORK,
#endif
	DIVOOM_AUDIO_BUS_COUNT
} divoom_audio_bus_t;

#define DIVOOM_AUDIO_BUF_POOL_RING_BYTES  32768u
#ifdef CONFIG_DIVOOM_MPP_NET_AUDIO
/* 电台仅保留32KiB PCM输出队列：48kHz/双声道/16bit 下约171毫秒；必须为 2 的幂。 */
#define DIVOOM_AUDIO_BUF_POOL_NETWORK_RING_BYTES  32768u
#endif
#if DIVOOM_SHOW_TF_NAND
#define DIVOOM_AUDIO_BUF_POOL_MUSIC_RING_BYTES  131072u /* 展机 TF 切盘空窗，须 2 的幂 */
#endif

/* TONE 和展机 MUSIC 预分配；SLEEP、NETWORK 随租约申请与释放。 */
int divoom_audio_buf_pool_init(void);

int divoom_audio_buf_pool_acquire(divoom_audio_bus_t bus, const char *owner_tag);
void divoom_audio_buf_pool_release(divoom_audio_bus_t bus, const char *owner_tag);

int divoom_audio_buf_pool_is_leased(divoom_audio_bus_t bus);
int divoom_audio_buf_pool_is_owner(divoom_audio_bus_t bus, const char *owner_tag);
void *divoom_audio_buf_pool_ring(divoom_audio_bus_t bus);

unsigned int divoom_audio_buf_pool_lease_fail_count(void);

/* sink 拆通道后归还租约（fade 未完也可强制） */
void divoom_audio_buf_pool_force_release(divoom_audio_bus_t bus, const char *owner_tag);

#endif
