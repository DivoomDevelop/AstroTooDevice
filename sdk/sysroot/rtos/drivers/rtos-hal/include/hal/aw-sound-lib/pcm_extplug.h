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

#ifndef __AW_ALSA_PCM_EXTPLUG_H
#define __AW_ALSA_PCM_EXTPLUG_H

#include <aw-sound-lib/pcm.h>

/** hw constraints for extplug */
enum {
	SOUND_PCM_EXTPLUG_HW_FORMAT,	/**< format */
	SOUND_PCM_EXTPLUG_HW_CHANNELS,	/**< channels */
	SOUND_PCM_EXTPLUG_HW_PARAMS	/**< max number of hw constraints */
};

/** Handle of external filter plugin */
typedef struct sound_pcm_extplug sound_pcm_extplug_t;
/** Callback table of extplug */
typedef struct sound_pcm_extplug_callback sound_pcm_extplug_callback_t;

/** Handle of extplug */
struct sound_pcm_extplug {
	/**
	 * name of this plugin; must be filled before calling #sound_pcm_extplug_create()
	 */
	const char *name;
	/**
	 * callbacks of this plugin; must be filled before calling #sound_pcm_extplug_create()
	 */
	const sound_pcm_extplug_callback_t *callback;
	/**
	 * private data, which can be used freely in the driver callbacks
	 */
	void *private_data;
	/**
	 * PCM handle filled by #sound_pcm_extplug_create()
	 */
	sound_pcm_t *pcm;
	/**
	 * stream direction; read-only status
	 */
	sound_pcm_stream_t stream;
	/**
	 * format hw parameter; filled after hw_params is caled
	 */
	sound_pcm_format_t format;
	/**
	 * subformat hw parameter; filled after hw_params is caled
	 */
	//sound_pcm_subformat_t subformat;
	/**
	 * channels hw parameter; filled after hw_params is caled
	 */
	unsigned int channels;
	/**
	 * rate hw parameter; filled after hw_params is caled
	 */
	unsigned int rate;
	/**
	 * slave_format hw parameter; filled after hw_params is caled
	 */
	sound_pcm_format_t slave_format;
	/**
	 * slave_channels hw parameter; filled after hw_params is caled
	 */
	unsigned int slave_channels;
};

/** Callback table of extplug */
struct sound_pcm_extplug_callback {
	/**
	 * transfer between source and destination; this is a required callback
	 */
	sound_pcm_sframes_t (*transfer)(sound_pcm_extplug_t *ext,
				      const sound_pcm_channel_area_t *dst_areas,
				      sound_pcm_uframes_t dst_offset,
				      const sound_pcm_channel_area_t *src_areas,
				      sound_pcm_uframes_t src_offset,
				      sound_pcm_uframes_t size);
	/**
	 * close the PCM; optional
	 */
	int (*close)(sound_pcm_extplug_t *ext);
	/**
	 * hw_params; optional
	 */
	int (*hw_params)(sound_pcm_extplug_t *ext, sound_pcm_hw_params_t *params);
	/**
	 * hw_free; optional
	 */
	int (*hw_free)(sound_pcm_extplug_t *ext);
	/**
	 * dump; optional
	 */
	void (*dump)(sound_pcm_extplug_t *ext);
	/**
	 * init; optional initialization called at prepare or reset
	 */
	int (*init)(sound_pcm_extplug_t *ext);
	/**
	 * query the channel maps; optional; since v1.0.2
	 */
	//sound_pcm_chmap_query_t **(*query_chmaps)(sound_pcm_extplug_t *ext);
	/**
	 * get the channel map; optional; since v1.0.2
	 */
	//sound_pcm_chmap_t *(*get_chmap)(sound_pcm_extplug_t *ext);
	/**
	 * set the channel map; optional; since v1.0.2
	 */
	//int (*set_chmap)(sound_pcm_extplug_t *ext, const sound_pcm_chmap_t *map);
};

int sound_pcm_extplug_create(sound_pcm_extplug_t *extplug, const char *name,
			   const char *spcm_name, sound_pcm_stream_t stream, int mode);

/* clear hw_parameter setting */
void sound_pcm_extplug_params_reset(sound_pcm_extplug_t *ext);

/* hw_parameter setting */
int sound_pcm_extplug_set_param_list(sound_pcm_extplug_t *extplug, int type, unsigned int num_list, const unsigned int *list);
int sound_pcm_extplug_set_param_minmax(sound_pcm_extplug_t *extplug, int type, unsigned int min, unsigned int max);
int sound_pcm_extplug_set_slave_param_list(sound_pcm_extplug_t *extplug, int type, unsigned int num_list, const unsigned int *list);
int sound_pcm_extplug_set_slave_param_minmax(sound_pcm_extplug_t *extplug, int type, unsigned int min, unsigned int max);

/**
 * set the parameter constraint with a single value
 */
static __inline__ int sound_pcm_extplug_set_param(sound_pcm_extplug_t *extplug, int type, unsigned int val)
{
	return sound_pcm_extplug_set_param_list(extplug, type, 1, &val);
}

/**
 * set the parameter constraint for slave PCM with a single value
 */
static __inline__ int sound_pcm_extplug_set_slave_param(sound_pcm_extplug_t *extplug, int type, unsigned int val)
{
	return sound_pcm_extplug_set_slave_param_list(extplug, type, 1, &val);
}

#endif /* __AW_ALSA_PCM_EXTPLUG_H */
