#ifndef __SUNXI_SOUND_LITE_CORE_H
#define __SUNXI_SOUND_LITE_CORE_H

#include<stdint.h>
#include <errno.h>
#include <hal_mutex.h>
#include <hal_osal.h>
#include <interrupt.h>
#include <hal_sem.h>
#include <limits.h>
#include "aw_list.h"

/******* dma *******/
#define SOUND_CMA_BLOCK_BYTES		1024
#define SOUND_CMA_DEFAULT_KBYTES	32
#define SOUND_DEFAULT_DTS_MAXBURST	4
#define SOUND_DEFAULT_SRC_MAXBURST	4
#define sunxi_slave_id(d, s) (((d)<<16) | (s))
typedef void* __iomem;
typedef __iomem dma_addr_t;

/******* memory alloc func *******/
#define sound_malloc(size)	calloc(1, size)
#define sound_calloc(n, size)	calloc(n, size)
#define sound_free(ptr)		free((void *)ptr)

/******* driver log func *******/
#define SOUNDRV_LOG_COLOR_RED		"\e[31m"
#define SOUNDRV_LOG_COLOR_GREEN		"\e[32m"
#define SOUNDRV_LOG_COLOR_NONE		"\e[0m"

#ifdef CONFIG_SUNXI_SOUND_DEBUG_ON
#define SOUND_LOG_DEBUG(fmt, args...) \
	printf(SOUNDRV_LOG_COLOR_GREEN "[SND_DEBUG][%s:%d]" fmt \
		SOUNDRV_LOG_COLOR_NONE, __FUNCTION__, __LINE__, ##args)
#else
#define SOUND_LOG_DEBUG(fmt, args...)
#endif
#define SOUND_LOG_ERR(fmt, args...) \
	printf(SOUNDRV_LOG_COLOR_RED "[SND_ERR][%s:%d]" fmt \
		SOUNDRV_LOG_COLOR_NONE, __FUNCTION__, __LINE__, ##args)
#define SOUND_LOG_INFO(fmt, args...) \
	printf(SOUNDRV_LOG_COLOR_GREEN "[SND_INFO][%s:%d]" fmt \
		SOUNDRV_LOG_COLOR_NONE, __FUNCTION__, __LINE__, ##args)

struct sound_sunxi_lite_card;

typedef enum {
	SUNXI_LITE_TRIGGER_START,
	SUNXI_LITE_TRIGGER_STOP,
} sunxi_lite_trigger_cmd_t;

typedef enum {
	SUNXI_LITE_STREAM_PLAY,
	SUNXI_LITE_STREAM_CAP,
} sunxi_lite_stream_t;

typedef enum {
	SUNXI_LITE_FORMAT_UNKNOWN = -1,
	SUNXI_LITE_FORMAT_S16_LE = 0,
	SUNXI_LITE_FORMAT_S24_LE,
	SUNXI_LITE_FORMAT_S32_LE,
} sunxi_lite_format_t;

typedef enum {
	SUNXI_LITE_STATE_RUNNING,
	SUNXI_LITE_STATE_XRUN,
	SUNXI_LITE_STATE_OPEN,
	SUNXI_LITE_STATE_DRAINING,
	SUNXI_LITE_STATE_SETUP,
} sunxi_lite_state_t;

struct sound_lite_controls {
	char *name;
	int val;
};

struct sunxi_lite_pcm_config {
	unsigned int rate;
	short channels;
	sunxi_lite_format_t format;
	unsigned long period_size;
	unsigned long buffer_size;
};

struct sunxi_lite_ptr_manager {
	sunxi_lite_state_t state;
	unsigned long hw_ptr;
	unsigned long appl_ptr;
	unsigned long hw_ptr_base;
	unsigned long min_align;
	unsigned long boundary;
};

typedef struct {
	hal_sem_t sem;
	int waiting;
} *sound_schd_t;

struct sound_sunxi_lite_runtime {
	dma_addr_t dma_addr;
	size_t dma_bytes;
	struct sunxi_dma_chan *chan;
	struct sunxi_lite_ptr_manager ptr_manager;
	sunxi_lite_stream_t stream;

	unsigned int frame_bits;
	unsigned long buffer_size;
	unsigned long period_size;
	unsigned int rate;
	short channels;
	sunxi_lite_format_t format;

	unsigned int twake;
	struct sound_sunxi_lite_card *card;
	hal_mutex_t mutex;
	hal_spinlock_t lock;
	sound_schd_t dsleep;
	sound_schd_t tsleep;
	bool running;
};

struct sound_lite_pcm {
	struct sound_sunxi_lite_card *card;
	struct sound_sunxi_lite_runtime *runtime;
	sunxi_lite_stream_t stream;
};

struct sound_sunxi_lite_card {
	const char *name;
	void *private_data;
	bool playback_only;
	bool capture_only;
	int (*set)(void *card_priv, short index, int val);
	int (*get)(short index);
	int (*open)(struct sound_sunxi_lite_runtime *runtime,
		    void *card_priv, sunxi_lite_stream_t stream);
	int (*trigger)(struct sound_sunxi_lite_runtime *runtime,
		       void *card_priv, sunxi_lite_trigger_cmd_t cmd,
		       sunxi_lite_stream_t stream);
	int (*prepare)(struct sound_sunxi_lite_runtime *runtime, void *card_priv);
	void (*close)(struct sound_sunxi_lite_runtime *runtime, void *card_priv);

	struct sound_sunxi_lite_runtime *play_runtime;
	struct sound_sunxi_lite_runtime *cap_runtime;
	struct list_head list;
};

/******* reg label for suspend/resume *******/
#define REG_LABEL(constant)	{#constant, constant, 0}
#define REG_LABEL_END		{NULL, 0, 0}

struct audio_reg_label {
	const char *name;
	const unsigned int address;
	unsigned int value;
};

/* EX:
 * static struct audio_reg_label reg_labels[] = {
 * 	REG_LABEL(SUNXI_REG_0),
 * 	REG_LABEL(SUNXI_REG_1),
 * 	REG_LABEL(SUNXI_REG_n),
 * 	REG_LABEL_END,
 * };
 */

/******* component load func *******/
#ifdef CONFIG_KERNEL_FREERTOS
struct sound_module {
	const char *name;
	int (*dev_probe)(void);
	void (*dev_remove)(void);
#if defined(CONFIG_COMPONENTS_PM) || defined(CONFIG_STANDBY)
	int (*suspend)(void);
	int (*resume)(void);
#endif
};

#define SECTION_USED(x)  __attribute__((used,section(x)))

#define REGISTER_MODULE(module) \
    const struct sound_module* sunxi_sound_##module SECTION_USED("sunxi_sound_module") = &(module)

void sound_lite_load_card(void);
void sound_lite_remove_card(void);
#if defined(CONFIG_COMPONENTS_PM) || defined(CONFIG_STANDBY)
void sound_lite_suspend_card(void);
void sound_lite_resume_card(void);
#endif
#endif

/******* REG *******/
#define sound_readl(reg)          (*(volatile uint32_t *)(reg))
#define sound_writel(value,reg)   (*(volatile uint32_t *)(reg) = (value))

int sound_lite_update_bits(void *addr_base,
			   unsigned int reg, unsigned int mask, unsigned int value);
int sound_lite_write(void *addr_base,
		     unsigned int reg, unsigned int val);
unsigned int sound_lite_read(void *addr_base, unsigned int reg);

/******* CORE *******/
static inline ssize_t frames_to_bytes(struct sound_sunxi_lite_runtime *runtime,
					   unsigned int size)
{
	return size * runtime->frame_bits / 8;
}
static inline long bytes_to_frames(struct sound_sunxi_lite_runtime *runtime,
					   ssize_t size)
{
	return size * 8 / runtime->frame_bits;
}
static inline unsigned long sound_pcm_capture_avail(struct sound_sunxi_lite_runtime *runtime)
{
	long avail = runtime->ptr_manager.hw_ptr - runtime->ptr_manager.appl_ptr;
	if (avail < 0)
		avail += runtime->ptr_manager.boundary;
	return avail;
}

static inline unsigned long sound_pcm_playback_avail(struct sound_sunxi_lite_runtime *runtime)
{
	long avail = runtime->ptr_manager.hw_ptr +
				runtime->buffer_size - runtime->ptr_manager.appl_ptr;

	if (avail < 0)
		avail += runtime->ptr_manager.boundary;
	else if ((unsigned long) avail >= runtime->ptr_manager.boundary)
		avail -= runtime->ptr_manager.boundary;

	return avail;
}

static inline unsigned long sound_pcm_playback_hw_avail(struct sound_sunxi_lite_runtime *runtime)
{
	return runtime->buffer_size - sound_pcm_playback_avail(runtime);
}

/* Get the free space for capture */
static inline unsigned long sound_pcm_capture_hw_avail(struct sound_sunxi_lite_runtime *runtime)
{
	return runtime->buffer_size - sound_pcm_capture_avail(runtime);
}

long sound_lite_xfer_data(struct sound_lite_pcm *pcm,
			 void *data, unsigned long size);
int sound_lite_dmaengine_start(struct sunxi_dma_chan *chan,
        struct sound_sunxi_lite_runtime *runtime);
void sound_lite_dmaengine_stop(struct sunxi_dma_chan *chan);

int sound_lite_register_card(struct sound_sunxi_lite_card *card);
void sound_lite_unregister_card(struct sound_sunxi_lite_card *card);
int sound_lite_card_open(struct sound_lite_pcm **pcmp, char *card_name,
			 sunxi_lite_stream_t stream, struct sunxi_lite_pcm_config *pcm_config);
int sound_lite_card_prepare(struct sound_lite_pcm *pcm);
void sound_lite_card_close(struct sound_lite_pcm *pcm);
int sound_lite_control_get(char *card_name, short ctrl);
int sound_lite_control_set(char *card_name, short ctrl, int val);
static inline long sound_lite_pcm_write(struct sound_lite_pcm *pcm,
			 void *data, unsigned long size)
{
	return sound_lite_xfer_data(pcm, data, size);
}
static inline long sound_lite_pcm_read(struct sound_lite_pcm *pcm,
			void *data, unsigned long size)
{
	return sound_lite_xfer_data(pcm, data, size);
}

typedef int (*sound_pcm_transfer_f)(struct sound_sunxi_lite_runtime *runtime,
				    int channel, unsigned long hwoff,
				    void *buf, unsigned long bytes);

int sound_lite_save_reg(struct audio_reg_label *reg_labels, void *addr_base);
int sound_lite_echo_reg(struct audio_reg_label *reg_labels, void *addr_base);

#endif /* __SUNXI_SOUND_LITE_CORE_H */
