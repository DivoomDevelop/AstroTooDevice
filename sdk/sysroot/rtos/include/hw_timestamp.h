#ifndef __HW_TIMESTAMP_H__
#define __HW_TIMESTAMP_H__

#include <stdint.h>
#include <inttypes.h>

struct clock_source;

typedef struct clock_source {
	uint64_t (*read)(struct clock_source *cs);

	uint32_t cyc2us_mult;
	uint32_t us2cyc_mult;
	uint32_t cyc2ns_mult;
	uint32_t ns2cyc_mult;

	uint8_t cyc2us_shift;
	uint8_t us2cyc_shift;
	uint8_t cyc2ns_shift;
	uint8_t ns2cyc_shift;

	const char *name;
	uint64_t mask;
	uint32_t freq;
} clock_source_t;

int hw_timestamp_init(clock_source_t *cs);

uint64_t hw_get_timestamp_us(void);
uint64_t hw_get_timestamp_ns(void);
uint64_t hw_get_count_value(void);
uint32_t hw_get_count_freq(void);

#endif  /* __HW_TIMESTAMP_H__ */
