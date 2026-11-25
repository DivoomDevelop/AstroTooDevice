#ifndef _divoom_pt_ui_h_
#define _divoom_pt_ui_h_

#include <stdint.h>

typedef enum {
	PT_UI_SHOW = 0,
	PT_UI_KEY,
} divoom_pt_ui_event_type;

int divoom_pt_ui_init(void);
void divoom_pt_ui_post(divoom_pt_ui_event_type type, uint32_t param);
void divoom_pt_ui_post_key(uint8_t key_id, uint8_t press_type);
int divoom_pt_ui_is_active(void);

#endif
