#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "RTOS device-only stub; exclude from simulator VS project."
#endif

#include "divoom.h"
#include "divoom_main.h"
#include "sunxifb.h"
#include "lvgl.h"

void divoom_set_mirror_mode(int mirror_flag)
{
	int enable = mirror_flag ? 1 : 0;

	if (sunxifb_get_mirror() == enable)
		return;

	sunxifb_set_mirror(enable);

	/* MQTT/config may call off LVGL thread; lock before invalidate. */
	divoom_get_lvgl_lock();
	if (lv_display_get_default() != NULL) {
		lv_obj_t *scr = lv_screen_active();

		if (scr != NULL)
			lv_obj_invalidate(scr);
	}
	divoom_free_lvgl_lock();
}
