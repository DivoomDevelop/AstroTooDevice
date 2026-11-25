#include "divoom_build_config.h"
#if !DIVOOM_BUILD_RTOS_DEVICE
#error "RTOS device-only stub; exclude from simulator VS project."
#endif

#include "divoom_line_in.h"
#include "divoom_type.h"

void divoom_line_in_init(void) {}
void divoom_line_in_deinit(void) {}
int divoom_line_in_is_initialized(void) { return 0; }
void divoom_line_in_start(void) {}
void divoom_line_in_stop(void) {}
void divoom_line_in_pause(void) {}
void divoom_line_in_resume(void) {}
void divoom_line_in_set_volume(uint8 volume) { (void)volume; }
uint8 divoom_line_in_get_volume(void) { return 0; }
LineInStatus divoom_line_in_get_status(void) { return LINE_IN_STATUS_IDLE; }
