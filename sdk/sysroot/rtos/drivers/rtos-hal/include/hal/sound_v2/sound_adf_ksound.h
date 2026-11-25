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
#ifndef __SOUND_ADF_KSOUND_H
#define __SOUND_ADF_KSOUND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sunxi_hal_sound.h>

typedef struct sound_pcm_dataflow sound_pcm_dataflow_t;
/* card */
int ksound_card_initialize(void);
int ksound_card_deinitialize(void);
int ksound_card_index(const char *name);
const char *ksound_card_name(int index);
int ksound_card_num(void);

/* pcm */
int ksound_pcm_control(void *pcm_dev_handle, unsigned int cmd, unsigned long karg);
int ksound_pcm_open(void **pcm_dev_handle, unsigned int card, unsigned int device,
                     sound_pcm_stream_t stream, int mode);
int ksound_pcm_release(void *pcm_dev_handle);
sound_pcm_sframes_t ksound_pcm_writei(void *pcm_dev_handle, const void *buffer, sound_pcm_uframes_t size);
sound_pcm_sframes_t ksound_pcm_readi(void *pcm_dev_handle, void *buffer, sound_pcm_uframes_t size);

/* ctl */
int ksound_ctl_num(const char *name);
int ksound_ctl_open(void **mixer_dev_handle, const char *card_name);
void ksound_ctl_close(void *mixer_dev_handler);
int ksound_ctl_get(const char *card_name, const char *ctl_name,
                         struct sound_control_info *info);
int ksound_ctl_get_by_name(void *mixer_dev, const char *name,
                                 struct sound_control_info *info);
int ksound_ctl_get_by_num(void *mixer_dev, const unsigned int id,
                                struct sound_control_info *info);
int ksound_ctl_add(void *mixer_dev, struct sound_control_info *info);
int ksound_ctl_remove(void *mixer_dev, unsigned int id);
int ksound_ctl_set(const char *card_name, const char *ctl_name,
                         unsigned int val);
int ksound_ctl_set_by_name(void *mixer_dev, const char *elem, unsigned int val);
int ksound_ctl_set_by_num(void *mixer_dev, const unsigned int elem_num, unsigned int val);

int ksound_ctl_set_multi_args(void *mixer_dev, const char *elem, int num, ...);

#ifdef __cplusplus
}
#endif

#endif /* __SOUND_ADF_KSOUND_H */
