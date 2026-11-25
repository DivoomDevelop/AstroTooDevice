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
#ifndef __SOUND_PCM_COMMON_H
#define __SOUND_PCM_COMMON_H

#include <stdint.h>
#include <errno.h>
#include <sunxi_hal_sound.h>

#ifndef EBADFD
#define EBADFD		77
#endif
#ifndef ESTRPIPE
#define ESTRPIPE	86
#endif

#ifndef LONG_MAX
#define LONG_MAX	((long)(~0UL>>1))
#endif
#ifndef ULONG_MAX
#define ULONG_MAX	(~0UL)
#endif
#ifndef UINT_MAX
#define UINT_MAX	(~0U)
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX	(~0ULL)
#endif


#define SOUNDRV_PCM_INFO_MMAP		0x00000001	/* hardware supports mmap */
#define SOUNDRV_PCM_INFO_MMAP_VALID	0x00000002	/* period data are valid during transfer */
#define SOUNDRV_PCM_INFO_DOUBLE		0x00000004	/* Double buffering needed for PCM start/stop */
#define SOUNDRV_PCM_INFO_BATCH		0x00000010	/* double buffering */
#define SOUNDRV_PCM_INFO_INTERLEAVED	0x00000100	/* channels are interleaved */
#define SOUNDRV_PCM_INFO_NONINTERLEAVED	0x00000200	/* channels are not interleaved */
#define SOUNDRV_PCM_INFO_COMPLEX		0x00000400	/* complex frame organization (mmap only) */
#define SOUNDRV_PCM_INFO_BLOCK_TRANSFER	0x00010000	/* hardware transfer block of samples */
#define SOUNDRV_PCM_INFO_OVERRANGE	0x00020000	/* hardware supports ADC (capture) overrange detection */
#define SOUNDRV_PCM_INFO_RESUME		0x00040000	/* hardware supports stream resume after suspend */
#define SOUNDRV_PCM_INFO_PAUSE		0x00080000	/* pause ioctl is supported */
#define SOUNDRV_PCM_INFO_HALF_DUPLEX	0x00100000	/* only half duplex */
#define SOUNDRV_PCM_INFO_JOINT_DUPLEX	0x00200000	/* playback and capture stream are somewhat correlated */
#define SOUNDRV_PCM_INFO_SYNC_START	0x00400000	/* pcm support some kind of sync go */
#define SOUNDRV_PCM_INFO_NO_PERIOD_WAKEUP	0x00800000	/* period wakeup can be disabled */
#define SOUNDRV_PCM_INFO_HAS_WALL_CLOCK   0x01000000      /* (Deprecated)has audio wall clock for audio/system time sync */
#define SOUNDRV_PCM_INFO_HAS_LINK_ATIME              0x01000000  /* report hardware link audio time, reset on startup */
#define SOUNDRV_PCM_INFO_HAS_LINK_ABSOLUTE_ATIME     0x02000000  /* report absolute hardware link audio time, not reset on startup */
#define SOUNDRV_PCM_INFO_HAS_LINK_ESTIMATED_ATIME    0x04000000  /* report estimated link audio time */
#define SOUNDRV_PCM_INFO_HAS_LINK_SYNCHRONIZED_ATIME 0x08000000  /* report synchronized audio/system time */
#define SOUNDRV_PCM_INFO_DRAIN_TRIGGER	0x40000000		/* internal kernel flag - trigger in drain */
#define SOUNDRV_PCM_INFO_FIFO_IN_FRAMES	0x80000000	/* internal kernel flag - FIFO size is in frames */


static inline int hw_is_mask(int var)
{
	return var >= SOUND_PCM_HW_PARAM_FIRST_MASK &&
		var <= SOUND_PCM_HW_PARAM_LAST_MASK;
}

static inline int hw_is_range(int var)
{
	return var >= SOUND_PCM_HW_PARAM_FIRST_RANGE &&
		var <= SOUND_PCM_HW_PARAM_LAST_RANGE;
}

static inline unsigned int div32(unsigned int a, unsigned int b,
				 unsigned int *r)
{
	if (b == 0) {
		*r = 0;
		return UINT_MAX;
	}
	*r = a % b;
	return a / b;
}

static inline uint64_t div_u64_rem(uint64_t dividend, uint32_t divisor, uint32_t *remainder)
{
	*remainder = dividend % divisor;
	return dividend / divisor;
}

static inline unsigned int div_down(unsigned int a, unsigned int b)
{
	if (b == 0)
		return UINT_MAX;
	return a / b;
}

static inline unsigned int div_up(unsigned int a, unsigned int b)
{
	unsigned int r;
	unsigned int q;
	if (b == 0)
		return UINT_MAX;
	q = div32(a, b, &r);
	if (r)
		++q;
	return q;
}

static inline unsigned int mul(unsigned int a, unsigned int b)
{
	if (a == 0)
		return 0;
	if (div_down(UINT_MAX, a) < b)
		return UINT_MAX;
	return a * b;
}

static inline unsigned int add(unsigned int a, unsigned int b)
{
	if (a >= UINT_MAX - b)
		return UINT_MAX;
	return a + b;
}

static inline unsigned int sub(unsigned int a, unsigned int b)
{
	if (a > b)
		return a - b;
	return 0;
}

static inline unsigned int muldiv32(unsigned int a, unsigned int b,
				    unsigned int c, unsigned int *r)
{
	uint64_t n = (uint64_t)a * (uint64_t)b;
	uint32_t rem;
	if (c == 0) {
		*r = 0;
		return UINT_MAX;
	}
	n = div_u64_rem(n, c, &rem);
	if (n >= UINT_MAX) {
		*r = 0;
		return UINT_MAX;
	}
	*r = rem;
	return n;
}

static inline int __pcm_ffs(uint32_t value)
{
	uint32_t offset;

	for (offset = 0; offset < sizeof(value)*8; offset++) {
		if (value & (1<<offset))
			return offset;
	}
	return -1;
}

static inline int __ffs(uint32_t value)
{
	uint32_t offset;

	for (offset = 0; offset < sizeof(value)*8; offset++) {
		if (value & (1<<offset))
			return offset;
	}
	return -1;
}

static inline int __fls(uint32_t value)
{
	uint32_t offset;

	for (offset = sizeof(value)*8; offset > 0; offset--) {
		if (value & (1<<(offset - 1)))
			return offset;
	}
	return -1;
}

static inline sound_pcm_access_t params_access(const struct sound_pcm_hw_params *p)
{
	const union sound_interval *interval = NULL;
	interval = &p->intervals[SOUND_PCM_HW_PARAM_ACCESS -
				SOUND_PCM_HW_PARAM_FIRST_INTERVAL];

	if (interval->mask != 0)
		return (sound_pcm_access_t)__ffs(interval->mask);
	return (sound_pcm_access_t)-1;
}

static inline sound_pcm_format_t params_format(const struct sound_pcm_hw_params *p)
{
	const union sound_interval *interval = NULL;
	interval = &p->intervals[SOUND_PCM_HW_PARAM_FORMAT -
				SOUND_PCM_HW_PARAM_FIRST_INTERVAL];

	if (interval->mask != 0)
		return (sound_pcm_format_t)__ffs(interval->mask);
	return SOUND_PCM_FORMAT_UNKNOWN;
}

static inline sound_interval_t *hw_param_interval(struct sound_pcm_hw_params *params,
						sound_pcm_hw_param_t var)
{
	return &params->intervals[var];
}

static inline const union sound_interval *hw_param_interval_c(const struct sound_pcm_hw_params *params,
							int var)
{
	return &params->intervals[var - SOUND_PCM_HW_PARAM_FIRST_INTERVAL];
}

static inline unsigned int params_channels(const struct sound_pcm_hw_params *p)
{
	return hw_param_interval_c(p, SOUND_PCM_HW_PARAM_CHANNELS)->range.min;
}

static inline unsigned int params_rate(const struct sound_pcm_hw_params *p)
{
	return hw_param_interval_c(p, SOUND_PCM_HW_PARAM_RATE)->range.min;
}

static inline unsigned int params_period_size(const struct sound_pcm_hw_params *p)
{
	return hw_param_interval_c(p, SOUND_PCM_HW_PARAM_PERIOD_SIZE)->range.min;
}

static inline unsigned int params_period_time(const struct sound_pcm_hw_params *p)
{
	return hw_param_interval_c(p, SOUND_PCM_HW_PARAM_PERIOD_TIME)->range.min;
}

static inline unsigned int params_periods(const struct sound_pcm_hw_params *p)
{
	return hw_param_interval_c(p, SOUND_PCM_HW_PARAM_PERIODS)->range.min;
}

static inline unsigned int params_buffer_size(const struct sound_pcm_hw_params *p)
{
	return hw_param_interval_c(p, SOUND_PCM_HW_PARAM_BUFFER_SIZE)->range.min;
}

static inline unsigned int params_buffer_time(const struct sound_pcm_hw_params *p)
{
	return hw_param_interval_c(p, SOUND_PCM_HW_PARAM_BUFFER_TIME)->range.min;
}


int sound_pcm_format_physical_width(sound_pcm_format_t format);

#define SOUND_MASK_BITS 32

static inline void sound_mask_none(sound_interval_t *interval)
{
	interval->mask = 0U;
}

static inline void sound_mask_any(sound_interval_t *interval)
{
	interval->mask = ~0U;
}

static inline int sound_mask_empty(const sound_interval_t *interval)
{
	return !interval->mask;
}

static inline int sound_mask_full(const sound_interval_t *interval)
{
	return interval->mask == 0xffffffff;
}

static inline unsigned int sound_mask_min(const sound_interval_t *interval)
{
	unsigned int i;
	for (i = 0; i < sizeof(interval->mask) * 8; i++) {
		if (interval->mask & (1 << i))
			return i;
	}
	return 0;
}

static inline unsigned int sound_mask_max(const sound_interval_t *interval)
{
	unsigned int i;
	for (i = sizeof(interval->mask) * 8; i > 0; i--) {
		if (interval->mask & (1 << (i - 1)))
			return i - 1;
	}
	return 0;
}

static inline void sound_mask_set(sound_interval_t *interval, unsigned int val)
{
	interval->mask |= (1 << val);
}

static inline void sound_mask_reset(sound_interval_t *interval, unsigned int val)
{
	interval->mask &= ~(1 << val);
}

static inline void sound_mask_set_range(sound_interval_t *interval,
		unsigned int from, unsigned int to)
{
	unsigned int i;
	for (i = from; i <= to; i++)
		interval->mask |= (1 << i);
}

static inline void sound_mask_reset_range(sound_interval_t *interval,
		unsigned int from, unsigned int to)
{
	unsigned int i;
	for (i = from; i <= to; i++)
		interval->mask &= ~(1 << i);
}

static inline void sound_mask_leave(sound_interval_t *interval, unsigned int val)
{
	sound_mask_none(interval);
	interval->mask |= (1 << val);
}

static inline void sound_mask_intersect(sound_interval_t *interval, const sound_interval_t *v)
{
	interval->mask &= v->mask;
}

static inline int sound_mask_eq(sound_interval_t *interval, const sound_interval_t *v)
{
	return interval->mask == v->mask;
}

static inline void sound_mask_copy(sound_interval_t *interval, const sound_interval_t *v)
{
	interval->mask = v->mask;
}

static inline int sound_mask_test(const sound_interval_t *interval, unsigned int val)
{
	return interval->mask & (1 << val);
}

static inline int sound_mask_single(const sound_interval_t *interval)
{
	unsigned int bits = sizeof(interval->mask) * 8;
	unsigned int i;
	int c = 0;
	for (i = 0; i < bits; ++i) {
		if (!(interval->mask & (1 << i)))
			continue;
		if (c)
			return 0;
		c++;
	}
	return 1;
}

static inline int sound_mask_refine(sound_interval_t *interval, const sound_interval_t *v)
{
	sound_interval_t old;
	sound_mask_copy(&old, interval);
	sound_mask_intersect(interval, v);
	if (sound_mask_empty(interval))
		return -EINVAL;
	return !sound_mask_eq(interval, &old);
}

static inline int sound_mask_refine_first(sound_interval_t *interval)
{
	if (sound_mask_single(interval))
		return 0;
	sound_mask_leave(interval, sound_mask_min(interval));
	return 1;
}

static inline int sound_mask_refine_last(sound_interval_t *interval)
{
	if (sound_mask_single(interval))
		return 0;
	sound_mask_leave(interval, sound_mask_max(interval));
	return 1;
}

static inline int sound_mask_refine_min(sound_interval_t *interval, unsigned int val)
{
	if (sound_mask_min(interval) >= val)
		return 0;
	sound_mask_reset_range(interval, 0, val - 1);
	if (sound_mask_empty(interval))
		return -EINVAL;
	return 1;
}

static inline int sound_mask_refine_max(sound_interval_t *interval, unsigned int val)
{
	if (sound_mask_max(interval) <= val)
		return 0;
	sound_mask_reset_range(interval, val + 1, SOUND_MASK_BITS);
	if (sound_mask_empty(interval))
		return -EINVAL;
	return 1;
}

static inline int sound_mask_refine_set(sound_interval_t *interval, unsigned int val)
{
	int changed;
	changed = !sound_mask_single(interval);
	sound_mask_leave(interval, val);
	if (sound_mask_empty(interval))
		return -EINVAL;
	return changed;
}

static inline int sound_mask_value(const sound_interval_t *interval)
{
	return sound_mask_min(interval);
}

static inline int sound_mask_always_eq(const sound_interval_t *m1, const sound_interval_t *m2)
{
	return sound_mask_single(m1) && sound_mask_single(m2) &&
		sound_mask_value(m1) == sound_mask_value(m2);
}

static inline int sound_mask_never_eq(const sound_interval_t *m1, const sound_interval_t *m2)
{
	if (m1->mask & m2->mask)
		return 0;
	return 1;
}

static inline void sound_range_any(sound_interval_t *i)
{
	i->range.min = 0;
	i->range.openmin = 0;
	i->range.max = UINT_MAX;
	i->range.openmax= 0;
	i->range.integer = 0;
	i->range.empty = 0;
}

static inline void sound_range_none(sound_interval_t *i)
{
	i->range.empty = 1;
}

static inline int sound_range_checkempty(sound_interval_t *i)
{
	return (i->range.min > i->range.max ||
		(i->range.min == i->range.max && (i->range.openmin || i->range.openmax)));
}

static inline int sound_range_empty(const sound_interval_t *i)
{
	return i->range.empty;
}

static inline int sound_range_single(const sound_interval_t *i)
{
#if 0
	return (i->range.min == i->range.max ||
		(i->range.min + 1 == i->range.max && i->range.openmax));
#else
	/* fix hw params install error, maybe not the best way */
	return (i->range.min == i->range.max ||
		((i->range.min + 1 == i->range.max) && i->range.openmax) ||
		((i->range.min + 2 == i->range.max) && i->range.openmax && i->range.openmin));
#endif
}

static inline int sound_range_value(const sound_interval_t *i)
{
	return i->range.min;
}

static inline void sound_range_set_value(sound_interval_t *i, unsigned int val)
{
	i->range.openmax = i->range.openmin = 0;
	i->range.min = i->range.max = val;
	i->range.integer = 0;
	i->range.empty = 0;
}

static inline int sound_range_min(const sound_interval_t *i)
{
	return i->range.min;
}

static inline int sound_range_max(const sound_interval_t *i)
{
	unsigned int v;
	v = i->range.max;
	if (i->range.openmax)
		v--;
	return v;
}

static inline void sound_range_set_minmax(sound_interval_t *i, unsigned int min, unsigned int max)
{
	i->range.openmax = i->range.openmin = 0;
	i->range.min = min;
	i->range.max = max;
	i->range.integer = 0;
	i->range.empty = 0;
}

static inline int sound_range_test(const sound_interval_t *i, unsigned int val)
{
	return !((i->range.min > val || (i->range.min == val && i->range.openmin) ||
		i->range.max < val || (i->range.max == val && i->range.openmax)));
}

static inline void sound_range_copy(sound_interval_t *d, const sound_interval_t *s)
{
	d->range = s->range;
}

static inline int sound_range_setinteger(sound_interval_t *i)
{
	if (i->range.integer)
		return 0;
	if (i->range.openmin && i->range.openmax && i->range.min == i->range.max)
		return -EINVAL;
	i->range.integer = 1;
	return 1;
}

static inline void sound_range_floor(sound_interval_t *i)
{
	if (i->range.integer || sound_range_empty(i))
		return;
	i->range.openmin = 0;
	if (i->range.openmax) {
		i->range.max--;
		i->range.openmax = 0;
	}
	i->range.integer = 1;
}

static inline void sound_range_unfloor(sound_interval_t *i)
{
	if (sound_range_empty(i))
		return;
	if (i->range.max == UINT_MAX)
		return;
	if (i->range.openmax)
		return;
	i->range.max++;
	i->range.openmax = 1;
	i->range.integer = 0;
}

static inline int sound_range_eq(const sound_interval_t *i1, const sound_interval_t *i2)
{
	if (i1->range.empty)
		return i2->range.empty;
	if (i2->range.empty)
		return i1->range.empty;
	return i1->range.min == i2->range.min &&
		i1->range.openmin == i2->range.openmin &&
		i1->range.max == i2->range.max &&
		i1->range.openmax == i2->range.openmax;
}

static inline int sound_range_always_eq(const sound_interval_t *i1, const sound_interval_t *i2)
{
	return sound_range_single(i1) && sound_range_single(i2) &&
		sound_range_value(i1) == sound_range_value(i2);
}

static inline int sound_range_never_eq(const sound_interval_t *i1, const sound_interval_t *i2)
{

	return (i1->range.max < i2->range.min ||
		(i1->range.max == i2->range.min &&
		 (i1->range.openmax || i1->range.openmin)) ||
		i1->range.min > i2->range.max ||
		(i1->range.min == i2->range.max &&
		 (i1->range.openmin || i2->range.openmax)));
}

int sound_range_refine(sound_interval_t *i, const sound_interval_t *v);
int sound_range_refine_first(sound_interval_t *i);
int sound_range_refine_last(sound_interval_t *i);
int sound_range_refine_min(sound_interval_t *i, unsigned int min, int openmin);
int sound_range_refine_max(sound_interval_t *i, unsigned int max, int openmax);
int sound_range_refine_set(sound_interval_t *i, unsigned int val);
void sound_range_add(const sound_interval_t *a, const sound_interval_t *b, sound_interval_t *c);
void sound_range_sub(const sound_interval_t *a, const sound_interval_t *b, sound_interval_t *c);
void sound_range_mul(const sound_interval_t *a, const sound_interval_t *b, sound_interval_t *c);
void sound_range_div(const sound_interval_t *a, const sound_interval_t *b, sound_interval_t *c);
void sound_range_muldiv(const sound_interval_t *a, const sound_interval_t *b,
		const sound_interval_t *c, sound_interval_t *d);
void sound_range_muldivk(const sound_interval_t *a, const sound_interval_t *b,
		unsigned int k, sound_interval_t *c);
void sound_range_mulkdiv(const sound_interval_t *a, unsigned int k,
		const sound_interval_t *b, sound_interval_t *c);
int sound_range_list(sound_interval_t *i, unsigned int count,
		const unsigned int *list, unsigned int mask);

const char *sound_pcm_hw_param_name(sound_pcm_hw_param_t param);

struct sound_pcm_hw_rule;
typedef int (*sound_pcm_hw_rule_func_t)(struct sound_pcm_hw_params *params,
				      struct sound_pcm_hw_rule *rule);

struct sound_pcm_hw_rule {
	unsigned int cond;
	int var;
	int deps[4];
	sound_pcm_hw_rule_func_t func;
	void *private_data;
};

int sound_pcm_hw_rule_mul(struct sound_pcm_hw_params *params, struct sound_pcm_hw_rule *rule);
int sound_pcm_hw_rule_div(struct sound_pcm_hw_params *params, struct sound_pcm_hw_rule *rule);
int sound_pcm_hw_rule_muldivk(struct sound_pcm_hw_params *params, struct sound_pcm_hw_rule *rule);
int sound_pcm_hw_rule_mulkdiv(struct sound_pcm_hw_params *params, struct sound_pcm_hw_rule *rule);
int sound_pcm_hw_rule_format(struct sound_pcm_hw_params *params, struct sound_pcm_hw_rule *rule);
int sound_pcm_hw_rule_sample_bits(struct sound_pcm_hw_params *params, struct sound_pcm_hw_rule *rule);
int sound_pcm_hw_rule_rate(struct sound_pcm_hw_params *params, struct sound_pcm_hw_rule *rule);
int sound_pcm_hw_rule_buffer_bytes_max(struct sound_pcm_hw_params *params, struct sound_pcm_hw_rule *rule);

struct sunxi_dcache_params
{
    sound_pcm_uframes_t offset;
    sound_pcm_uframes_t size;
};

struct sunxi_wait_params
{
    int index;
    int timeout;
};

#endif /* __SOUND_PCM_COMMON_H */
