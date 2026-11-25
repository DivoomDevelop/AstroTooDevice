#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "RTOS device-only stub; exclude from simulator VS project."
#endif

#include "divoom_uac.h"
#include "divoom_type.h"

void divoom_uac_init(void) {}
void divoom_uac_deinit(void) {}
void divoom_uac_start(void) {}
void divoom_uac_stop(void) {}
void divoom_uac_pause(void) {}
void divoom_uac_resume(void) {}
void divoom_uac_set_volume(uint8 volume) { (void)volume; }
uint8 divoom_uac_get_volume(void) { return 0; }
UacStatus divoom_uac_get_status(void) { return UAC_STATUS_IDLE; }
