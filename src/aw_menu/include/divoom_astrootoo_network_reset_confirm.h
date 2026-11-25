#ifndef _divoom_astrootoo_network_reset_confirm_h_
#define _divoom_astrootoo_network_reset_confirm_h_

#include "divoom.h"

#define ASTRO_NETWORK_RESET_HOLD_MS         3000    // 长按多少秒，就改这里，但是一定要比 ASTRO_NETWORK_RESET_LONG_MS 大
#define ASTRO_NETWORK_RESET_LONG_MS         2000
#define ASTRO_NETWORK_RESET_HOLD_DELAY_MS   (ASTRO_NETWORK_RESET_HOLD_MS - ASTRO_NETWORK_RESET_LONG_MS)

int divoom_astrootoo_network_reset_confirm_is_open(void);

void divoom_astrootoo_network_reset_confirm_arm_hold(void);
void divoom_astrootoo_network_reset_confirm_disarm_hold(void);
void divoom_astrootoo_network_reset_confirm_on_pressing(void);

#endif
