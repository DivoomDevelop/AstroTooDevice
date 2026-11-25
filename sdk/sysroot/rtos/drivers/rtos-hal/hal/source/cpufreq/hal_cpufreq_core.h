/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS'S SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
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

#ifndef __HAL_CPUFREQ_CORE_H__
#define __HAL_CPUFREQ_CORE_H__

#include <sunxi_hal_common.h>
#include <hal_atomic.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CPUFREQ_FLAG_DVFS_SUPPORT    (1 << 0)
#define CPUFREQ_FLAG_THERMAL_LIMIT   (1 << 1)
#define CPUFREQ_FLAG_BOOST_MODE      (1 << 2)

struct cpufreq_freq_point {
	unsigned long freq;
	unsigned int voltage;
	unsigned int div_m0;
	unsigned int div_m1;
	unsigned int div_n;
	unsigned int div_p;
	unsigned int latency;
};

struct cpufreq_platform_config {
	const char *platform_name;
	unsigned long reg_ctrl_base;
	unsigned long reg_ssc_base;
	struct cpufreq_freq_point *freq_point;
	unsigned int flags;
};

struct cpufreq_device;

struct cpufreq_driver_ops {
	int (*init)(struct cpufreq_device *dev);
	void (*exit)(struct cpufreq_device *dev);
	int (*set_freq)(struct cpufreq_device *dev, unsigned int freq);
	int (*get_freq)(struct cpufreq_device *dev);

	int (*get_voltage)(struct cpufreq_device *dev);
	int (*set_voltage)(struct cpufreq_device *dev, unsigned int voltage);
};

struct cpufreq_device {
	struct cpufreq_platform_config *config;
	struct cpufreq_driver_ops *ops;
	void *priv_data;
	unsigned int current_freq;
	hal_spinlock_t lock;
};

int cpufreq_core_init(void);
struct cpufreq_device *cpufreq_get_device(void);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_CPUFREQ_CORE_H__ */
