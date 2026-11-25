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

#ifndef __AW_ALSA_PCM_GENERIC_H
#define __AW_ALSA_PCM_GENERIC_H

#include <aw-sound-lib/pcm.h>

typedef struct {
	sound_pcm_t *slave;
	int close_slave;
} sound_pcm_generic_t;

int sound_pcm_generic_close(sound_pcm_t *pcm);
int sound_pcm_generic_hw_free(sound_pcm_t *pcm);
int sound_pcm_generic_sw_params(sound_pcm_t *pcm, sound_pcm_sw_params_t *params);
int sound_pcm_generic_hw_refine(sound_pcm_t *pcm, sound_pcm_hw_params_t *params);
int sound_pcm_generic_hw_params(sound_pcm_t *pcm, sound_pcm_hw_params_t *params);
int sound_pcm_generic_prepare(sound_pcm_t *pcm);
int sound_pcm_generic_channel_info(sound_pcm_t *pcm, sound_pcm_channel_info_t *info);
int sound_pcm_generic_mmap(sound_pcm_t *pcm);
int sound_pcm_generic_munmap(sound_pcm_t *pcm);
sound_pcm_chmap_query_t **sound_pcm_generic_query_chmaps(sound_pcm_t *pcm);
sound_pcm_chmap_t *sound_pcm_generic_get_chmap(sound_pcm_t *pcm);
int sound_pcm_generic_set_chmap(sound_pcm_t *pcm, const sound_pcm_chmap_t *map);
sound_pcm_state_t sound_pcm_generic_state(sound_pcm_t *pcm);
int sound_pcm_generic_hwsync(sound_pcm_t *pcm);
int sound_pcm_generic_reset(sound_pcm_t *pcm);
int sound_pcm_generic_start(sound_pcm_t *pcm);
int sound_pcm_generic_drop(sound_pcm_t *pcm);
int sound_pcm_generic_drain(sound_pcm_t *pcm);
int sound_pcm_generic_pause(sound_pcm_t *pcm, int enable);
int sound_pcm_generic_resume(sound_pcm_t *pcm);
int sound_pcm_generic_delay(sound_pcm_t *pcm, sound_pcm_sframes_t *delayp);
sound_pcm_sframes_t sound_pcm_generic_writei(sound_pcm_t *pcm, const void *buffer, sound_pcm_uframes_t size);
sound_pcm_sframes_t sound_pcm_generic_readi(sound_pcm_t *pcm, void *buffer, sound_pcm_uframes_t size);
sound_pcm_sframes_t sound_pcm_generic_avail_update(sound_pcm_t *pcm);
sound_pcm_sframes_t sound_pcm_generic_mmap_commit(sound_pcm_t *pcm,
					      sound_pcm_uframes_t offset,
					      sound_pcm_uframes_t size);
int sound_pcm_generic_may_wait_for_avail_min(sound_pcm_t *pcm, sound_pcm_uframes_t avail);

#endif /* __AW_ALSA_PCM_GENERIC_H */
