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
#ifndef __SOUND_ADF_DAI_H
#define __SOUND_ADF_DAI_H

#include "aw_list.h"
//#include <sound_v2/sound_adf_core.h>

struct sound_adf_dai;
struct sound_adf_dai_driver;

struct sound_adf_dai_ops {
	/*
	 * DAI clocking configuration, all optional.
	 * Called by soc_card drivers, normally in their hw_params.
	 */
	int (*set_sysclk)(struct sound_adf_dai *dai,
		int clk_id, unsigned int freq, int dir);
	int (*set_pll)(struct sound_adf_dai *dai, int pll_id, int source,
		unsigned int freq_in, unsigned int freq_out);
	int (*set_clkdiv)(struct sound_adf_dai *dai, int div_id, int div);
	int (*set_bclk_ratio)(struct sound_adf_dai *dai, unsigned int ratio);

	/*
	 * DAI format configuration
	 * Called by soc_card drivers, normally in their hw_params.
	 */
	int (*set_fmt)(struct sound_adf_dai *dai, unsigned int fmt);
	int (*set_tdm_slot)(struct sound_adf_dai *dai,
		unsigned int tx_mask, unsigned int rx_mask,
		int slots, int slot_width);

	/*
	 * PCM audio operations - all optional.
	 * Called by adf during audio PCM operations.
	 */
	int (*startup)(struct sound_pcm_dataflow *,
		struct sound_adf_dai *);
	void (*shutdown)(struct sound_pcm_dataflow *,
		struct sound_adf_dai *);
	int (*hw_params)(struct sound_pcm_dataflow *,
		struct sound_pcm_hw_params *, struct sound_adf_dai *);
	int (*hw_free)(struct sound_pcm_dataflow *,
		struct sound_adf_dai *);
	int (*prepare)(struct sound_pcm_dataflow *,
		struct sound_adf_dai *);

	int (*trigger)(struct sound_pcm_dataflow *, int,
		struct sound_adf_dai *);

};


struct sound_adf_dai_driver {
	const char *name;
	int id;
	/* DAI driver callbacks */
	int (*probe)(struct sound_adf_dai *dai);
	int (*remove)(struct sound_adf_dai *dai);
	const struct sound_adf_dai_ops *ops;
	/* DAI capabilities */
	struct sound_adf_pcm_stream capture;
	struct sound_adf_pcm_stream playback;
	int probe_rank;
	int remove_rank;
};

struct sound_adf_dai {
	const char *name;
	int id;
	/* driver ops */
	struct sound_adf_dai_driver *driver;

	/* DAI DMA data */
	void *playback_dma_data;
	void *capture_dma_data;

	/* parent platform/codec */
	struct sound_adf_component *component;

	struct list_head list;
	/* bit field */
	unsigned int probed:1;

};

/*
 * Master Clock Directions
 */
#define SOUND_ADF_CLOCK_IN		0
#define SOUND_ADF_CLOCK_OUT		1

int sound_adf_dai_set_sysclk(struct sound_adf_dai *dai, int clk_id,
			   unsigned int freq, int dir);

int sound_adf_dai_set_clkdiv(struct sound_adf_dai *dai,
			   int div_id, int div);

int sound_adf_dai_set_pll(struct sound_adf_dai *dai, int pll_id, int source,
			unsigned int freq_in, unsigned int freq_out);

int sound_adf_dai_set_bclk_ratio(struct sound_adf_dai *dai, unsigned int ratio);

int sound_adf_set_tdm_slot(struct sound_adf_dai *dai,
			     unsigned int tx_mask, unsigned int rx_mask,
			     int slots, int slot_width);

int sound_adf_dai_set_fmt(struct sound_adf_dai *dai, unsigned int fmt);

int sound_adf_dai_hw_params(struct sound_adf_dai *dai,
			  struct sound_pcm_dataflow *dataflow,
			  struct sound_pcm_hw_params *params);

int sound_adf_dai_hw_free(struct sound_adf_dai *dai,
			  struct sound_pcm_dataflow *dataflow);

int sound_adf_dai_startup(struct sound_adf_dai *dai,
			struct sound_pcm_dataflow *dataflow);


void sound_adf_dai_shutdown(struct sound_adf_dai *dai,
			struct sound_pcm_dataflow *dataflow);

bool sound_adf_dai_stream_valid(struct sound_adf_dai *dai, int dir);


int sound_adf_pcm_dai_probe(struct sound_adf_pcm_running_param *rtp);
int sound_adf_pcm_dai_remove(struct sound_adf_pcm_running_param *rtp);

int sound_adf_pcm_dai_prepare(struct sound_pcm_dataflow *dataflow);

int sound_adf_pcm_dai_trigger(struct sound_pcm_dataflow *dataflow, int cmd);

static inline struct sound_adf_pcm_stream *
sound_adf_dai_get_pcm_stream(const struct sound_adf_dai *dai, int stream)
{
	return (stream == SOUNDRV_PCM_STREAM_PLAYBACK) ?
		&dai->driver->playback : &dai->driver->capture;
}

static inline void *sound_adf_dai_get_dma_data(const struct sound_adf_dai *dai,
					     const struct sound_pcm_dataflow *ss)
{
	return (ss->stream == SOUNDRV_PCM_STREAM_PLAYBACK) ?
		dai->playback_dma_data : dai->capture_dma_data;
}

static inline void sound_adf_dai_set_dma_data(struct sound_adf_dai *dai,
					    const struct sound_pcm_dataflow *ss,
					    void *data)
{
	if (ss->stream == SOUNDRV_PCM_STREAM_PLAYBACK)
		dai->playback_dma_data = data;
	else
		dai->capture_dma_data = data;
}

static inline void sound_adf_dai_init_dma_data(struct sound_adf_dai *dai,
					     void *playback, void *capture)
{
	dai->playback_dma_data = playback;
	dai->capture_dma_data = capture;
}

#endif /* __SOUND_ADF_CORE_H */
