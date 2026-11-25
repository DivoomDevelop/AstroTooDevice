/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __SOUND_CORE_H
#define __SOUND_CORE_H

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <hal_sem.h>
#include <hal_mutex.h>
#include <hal_atomic.h>
#include <hal_interrupt.h>
#include <sunxi_hal_common.h>
#include "aw_list.h"
#include <sunxi_hal_sound.h>

//#include <sound_v2/sound_pcm_common.h>
#define SOUND_CARDS 8

typedef unsigned int hal_irq_state_t;

#define hal_local_irq_enable()  hal_interrupt_enable()
#define hal_local_irq_disable() hal_interrupt_disable()
#define hal_local_irq_save(flags) \
	do { \
		flags = hal_interrupt_disable_irqsave(); \
	} while (0)
#define hal_local_irq_restore(flags) \
	do { \
		hal_interrupt_enable_irqrestore(flags); \
	} while (0)


typedef hal_mutex_t sound_mutex_t;
sound_mutex_t sound_mutex_init(void);
int sound_mutex_lock_timeout(sound_mutex_t mutex, long ms);
int sound_mutex_lock(sound_mutex_t mutex);
void sound_mutex_unlock(sound_mutex_t mutex);
void sound_mutex_destroy(sound_mutex_t mutex);

typedef struct {
	hal_sem_t sem;
	int waiting;
} *sound_schd_t;

sound_schd_t sound_schd_init(void);
int sound_schd_timeout(sound_schd_t schd, long ms);
void sound_schd_wakeup(sound_schd_t schd);
void sound_schd_destroy(sound_schd_t schd);

#define sound_readb(reg)          (*(volatile uint8_t  *)(reg))
#define sound_readw(reg)          (*(volatile uint16_t *)(reg))
#define sound_readl(reg)          (*(volatile uint32_t *)(reg))
#define sound_writeb(value,reg)   (*(volatile uint8_t  *)(reg) = (value))
#define sound_writew(value,reg)   (*(volatile uint16_t *)(reg) = (value))
#define sound_writel(value,reg)   (*(volatile uint32_t *)(reg) = (value))

enum {
	SOUNDRV_PCM_STREAM_PLAYBACK = 0,
	SOUNDRV_PCM_STREAM_CAPTURE,
	SOUNDRV_PCM_STREAM_LAST = SOUNDRV_PCM_STREAM_CAPTURE,
};

enum sound_module_type {
	SOUND_MODULE_UNKNOWN = 0,
	SOUND_MODULE_CODEC,
	SOUND_MODULE_PLATFORM,
	SOUND_MODULE_USER,
};

/* number of supported soundcards */
#ifdef CONFIG_SOUND_DYNAMIC_MINORS
#define SOUNDDRV_CARDS CONFIG_SOUND_MAX_CARDS
#else
#define SOUNDDRV_CARDS 8
#endif

/* type of the object used in sound_device_*()
 * this also defines the calling order
 */
enum sound_device_type {
	SOUNDRV_DEV_LOWLEVEL,
	SOUNDRV_DEV_PCM,
	SOUNDRV_DEV_TIMER,
	SOUNDRV_DEV_JACK,
	SOUNDRV_DEV_CONTROL,	/* NOTE: this must be the last one */
};

enum sound_device_state {
	SOUNDRV_DEV_BUILD,
	SOUNDRV_DEV_REGISTERED,
};

struct sound_device;

struct sound_device_ops {
	int (*dev_unregiter)(struct sound_device *dev);
	int (*dev_register)(struct sound_device *dev);
};

struct sound_card;

/* For sound device manager, such as pcm, control */
struct sound_device {
	struct list_head list;		/* list of registered devices */
	struct sound_card *card;		/* card which holds this device */
	enum sound_device_state state;	/* state of the device */
	enum sound_device_type type;	/* device type */
	void *device_data;		/* device structure */
	const struct sound_device_ops *ops;	/* operations */
};

#define sound_device(n) list_entry(n, struct sound_device, list)

struct sound_sys_device;

struct sound_sys_device_ops {
	int (*open)(struct sound_sys_device *dev, unsigned int mode);
	int (*close)(struct sound_sys_device *dev);
	int (*control)(struct sound_sys_device *dev, unsigned int cmd, unsigned long arg);
	int (*write)(struct sound_sys_device *dev, const void *buffer, sound_pcm_uframes_t size);
	int (*read)(struct sound_sys_device *dev, void *buffer, sound_pcm_uframes_t size);
};

struct sound_card_device;

/* For sound device, such as pcm, control */
struct sound_sys_device {
	const char *name;
	int stream;
	struct sound_card_device *parent;
	const struct sound_sys_device_ops *sdev_ops;	/* sound dev operations */
	void *priv_data;
	struct list_head child_list;
};

/* For sound card device */
struct sound_card_device {
	const char *name;
	struct sound_card_device *sibling;
	void *priv_data;
	struct list_head child_head;    /* List of children of this device */
	struct list_head sibling_node;  /* Next device in list of all devices */
};

struct sound_card {
	const char *name; /* soundcard name */
	int num; /* number of soundcard */
	struct list_head devices; /* device list */
	struct sound_card_device dev; /* card device */
	struct sound_sys_device control_dev;
	sound_mutex_t ctrl_mutex;
	struct list_head controls;	/* all controls for this card */
	int controls_nums;		/* nums of all controls */
};

int sound_device_create(struct sound_card *card, enum sound_device_type type,
		   void *dev_data, const struct sound_device_ops *ops);

int sound_device_register(struct sound_card *card, void *device_data);
int sound_device_register_all(struct sound_card *card);
void sound_device_destory(struct sound_card *card, void *device_data);
struct sound_card *sound_card_find_by_name(const char *name);
const char *sound_card_find_by_num(int index);
int sound_card_get_number(void);
void sound_device_destory_all(struct sound_card *card);
int sound_card_create(const char* name, struct sound_card **card_ret);
int sound_card_destory(struct sound_card *card);

int sound_register_device(struct sound_card *card, const struct sound_sys_device_ops *sdev_ops,
			void *private_data, struct sound_sys_device *dev);
int sound_unregister_device(struct sound_card_device *card_dev, struct sound_sys_device *dev);


#endif /* __SOUND_CORE_H */
