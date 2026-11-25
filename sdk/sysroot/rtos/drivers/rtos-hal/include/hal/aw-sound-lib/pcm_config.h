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

#ifndef __AW_ALSA_PCM_CONFIG_H
#define __AW_ALSA_PCM_CONFIG_H

#include <stddef.h>
#include <aw-sound-lib/pcm.h>

typedef struct _sound_pcm_config {
	const char *name;
	const char *type;
	void *config;
} sound_pcm_config_t;

extern const sound_pcm_config_t * const _sound_pcm_global_configs;
extern const size_t _sound_pcm_global_configs_size;

#define SOUND_PCM_CONFIG(xname, xtype, xconfig) \
{ \
	.name = xname, \
	.type = xtype, \
	.config = (void *)xconfig, \
}

#define REGISTER_SOUND_PCM_GLOBAL_CONFIGS(configs_array) \
	const size_t _sound_pcm_global_configs_size = \
		sizeof(configs_array) / sizeof(configs_array[0]); \
	const sound_pcm_config_t * const _sound_pcm_global_configs = configs_array;

const sound_pcm_config_t *sound_pcm_config_get_config(const char *name);

typedef int (*sound_pcm_open_func_t)(sound_pcm_t **pcmp, const sound_pcm_config_t *pcm_config,
		sound_pcm_stream_t stream, int mode);

typedef struct {
	const char *type;
	sound_pcm_open_func_t func;
} _sound_pcm_open_func_t;

sound_pcm_open_func_t sound_pcm_config_get_open_func(const char *type);

#endif /* __AW_ALSA_PCM_CONFIG_H */
