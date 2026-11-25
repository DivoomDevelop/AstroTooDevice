#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "RTOS device-only stub; exclude from simulator VS project."
#endif

#include "divoom_sd_card_manage.h"

int divoom_sd_card_manage_init(DIVOOM_SD_CARD_CALLBACK callback)
{
	(void)callback;
	return 1;
}
void divoom_sd_card_manage_deinit(void) {}
DIVOOM_SD_CARD_STATE divoom_sd_card_manage_get_state(void) { return DIVOOM_SD_CARD_STATE_NONE; }
int divoom_sd_card_manage_is_mounted(void) { return 0; }
int divoom_sd_card_manage_mount(void) { return 0; }
int divoom_sd_card_manage_unmount(void) { return 0; }
int divoom_sd_card_manage_scan_music(void) { return 0; }
const char *divoom_sd_card_manage_get_mount_path(void) { return "/mnt/sdcard"; }
