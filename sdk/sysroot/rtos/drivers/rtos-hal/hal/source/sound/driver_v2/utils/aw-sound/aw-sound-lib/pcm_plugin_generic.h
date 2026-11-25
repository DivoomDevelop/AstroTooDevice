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

#ifndef __AW_ALSA_PCM_PLUGIN_GENERIC_H
#define __AW_ALSA_PCM_PLUGIN_GENERIC_H

#include "pcm_generic.h"

typedef sound_pcm_uframes_t (*sound_pcm_slave_xfer_areas_func_t)
	(sound_pcm_t *pcm,
	const sound_pcm_channel_area_t *areas, sound_pcm_uframes_t offset,
	sound_pcm_uframes_t size,
	const sound_pcm_channel_area_t *slave_areas,
	sound_pcm_uframes_t slave_offset,
	sound_pcm_uframes_t *slave_sizep);

typedef sound_pcm_sframes_t (*sound_pcm_slave_xfer_areas_undo_func_t)
	(sound_pcm_t *pcm,
	const sound_pcm_channel_area_t *res_areas,	/* result areas */
	sound_pcm_uframes_t res_offset,		/* offset of result areas */
	sound_pcm_uframes_t res_size,		/* size of result areas */
	sound_pcm_uframes_t slave_undo_size);

typedef struct {
	sound_pcm_generic_t gen;
	sound_pcm_slave_xfer_areas_func_t read;
	sound_pcm_slave_xfer_areas_func_t write;
	sound_pcm_slave_xfer_areas_undo_func_t undo_read;
	sound_pcm_slave_xfer_areas_undo_func_t undo_write;
	int (*init)(sound_pcm_t *pcm);
	sound_pcm_uframes_t appl_ptr, hw_ptr;
} sound_pcm_plugin_t;

void sound_pcm_plugin_init(sound_pcm_plugin_t *plugin);
int sound_pcm_plugin_may_wait_for_avail_min(sound_pcm_t *pcm, sound_pcm_uframes_t avail);
int sound_pcm_plugin_wait(sound_pcm_t *pcm, int timeout);

extern const sound_pcm_fast_ops_t sound_pcm_plugin_fast_ops;

sound_pcm_sframes_t sound_pcm_plugin_undo_read_generic
     (sound_pcm_t *pcm,
      const sound_pcm_channel_area_t *res_areas,	/* result areas */
      sound_pcm_uframes_t res_offset,		/* offset of result areas */
      sound_pcm_uframes_t res_size,		/* size of result areas */
      sound_pcm_uframes_t slave_undo_size);

sound_pcm_sframes_t sound_pcm_plugin_undo_write_generic
     (sound_pcm_t *pcm,
      const sound_pcm_channel_area_t *res_areas,	/* result areas */
      sound_pcm_uframes_t res_offset,		/* offset of result areas */
      sound_pcm_uframes_t res_size,		/* size of result areas */
      sound_pcm_uframes_t slave_undo_size);

int sound_pcm_linear_get_index(sound_pcm_format_t src_format, sound_pcm_format_t dst_format);
int sound_pcm_linear_put_index(sound_pcm_format_t src_format, sound_pcm_format_t dst_format);
int sound_pcm_linear_convert_index(sound_pcm_format_t src_format, sound_pcm_format_t dst_format);

#endif /* __AW_ALSA_PCM_PLUGIN_GENERIC_H */
