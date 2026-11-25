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
#ifndef __SOUND_ADF_COMPONENT_H
#define __SOUND_ADF_COMPONENT_H

//#include <sound_v2/sound_adf_core.h>

struct sound_adf_component;

struct sound_adf_component_driver {
	char *name;
	const struct sound_adf_control_new *controls;
	unsigned int num_controls;
	/* pcm creation and destruction */
	int (*pcm_construct)(struct sound_adf_component *component,
	struct sound_adf_pcm_running_param *rtp);
	void (*pcm_destruct)(struct sound_adf_component *component,
	struct sound_pcm *pcm);

	unsigned int (*read)(struct sound_adf_component *component,
			     unsigned int reg);
	int (*write)(struct sound_adf_component *component,
		     unsigned int reg, unsigned int val);

	int (*set_pll)(struct sound_adf_component *component, int pll_id,
		       int source, unsigned int freq_in, unsigned int freq_out);

	int (*probe)(struct sound_adf_component *component);
	void (*remove)(struct sound_adf_component *component);
	int (*suspend)(struct sound_adf_component *component);
	int (*resume)(struct sound_adf_component *component);

	int (*open)(struct sound_adf_component *component,
	struct sound_pcm_dataflow *dataflow);
	int (*close)(struct sound_adf_component *component,
	struct sound_pcm_dataflow *dataflow);
	int (*ioctl)(struct sound_adf_component *component,
	struct sound_pcm_dataflow *dataflow,
	unsigned int cmd, void *arg);
	int (*hw_params)(struct sound_adf_component *component,
	struct sound_pcm_dataflow *dataflow,
	struct sound_pcm_hw_params *params);
	int (*hw_free)(struct sound_adf_component *component,
	struct sound_pcm_dataflow *dataflow);
	int (*prepare)(struct sound_adf_component *component,
	struct sound_pcm_dataflow *dataflow);
	int (*trigger)(struct sound_adf_component *component,
	struct sound_pcm_dataflow *dataflow, int cmd);
	sound_pcm_uframes_t (*pointer)(struct sound_adf_component *component,
	struct sound_pcm_dataflow *dataflow);

	int (*dapm_control)(struct sound_adf_component *,
			struct sound_pcm_dataflow *, int onoff);
};

struct sound_adf_component {
	char *name;
	int id;
	void *private_data;
	void *addr_base;
	struct sound_adf_card *card;
	unsigned int suspended:1;
	struct list_head list;
	const struct sound_adf_component_driver *driver;
	struct list_head bind_list;
	int num_binds;
	int data_type;
};

int sound_adf_component_set_pll(struct sound_adf_component *component, int pll_id,
		       int source, unsigned int freq_in, unsigned int freq_out);

int sound_adf_component_open(struct sound_adf_component *component,
			   struct sound_pcm_dataflow *dataflow);
int sound_adf_component_close(struct sound_adf_component *component,
			   struct sound_pcm_dataflow *dataflow);

void sound_adf_component_suspend(struct sound_adf_component *component);

void sound_adf_component_resume(struct sound_adf_component *component);

sound_pcm_uframes_t sound_adf_pcm_component_pointer(struct sound_pcm_dataflow *dataflow);

int sound_adf_pcm_component_ioctl(struct sound_pcm_dataflow *dataflow,
				unsigned int cmd, void *arg);

int sound_adf_pcm_component_new(struct sound_adf_pcm_running_param *rtp);

void sound_adf_pcm_component_free(struct sound_adf_pcm_running_param *rtp);

int sound_adf_pcm_component_prepare(struct sound_pcm_dataflow *dataflow);

int sound_adf_pcm_component_hw_params(struct sound_pcm_dataflow *dataflow,
				    struct sound_pcm_hw_params *params);

void sound_adf_pcm_component_hw_free(struct sound_pcm_dataflow *dataflow);

int sound_adf_pcm_component_trigger(struct sound_pcm_dataflow *dataflow, int cmd);

int sound_adf_pcm_component_dapm_control(struct sound_pcm_dataflow *dataflow, int onoff);

int sound_adf_component_probe(struct sound_adf_component *component);
void sound_adf_component_remove(struct sound_adf_component *component);

unsigned int sound_adf_component_read_no_lock(struct sound_adf_component *component,
	unsigned int reg);

int sound_adf_component_write_no_lock(struct sound_adf_component *component,
		unsigned int reg, unsigned int val);

#endif /* __SOUND_ADF_COMPONENT_H */
