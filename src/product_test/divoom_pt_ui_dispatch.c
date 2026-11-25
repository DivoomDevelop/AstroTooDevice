#include "divoom.h"
#include "divoom_product_test.h"
#include "divoom_pt_ui.h"
#include "divoom_product_test_ui.h"
#include "divoom_lvgl_product_test_menu.h"
#include "divoom_user_key.h"
#include "lvgl.h"

typedef struct {
	divoom_pt_ui_event_type type;
	uint32_t param;
	uint8_t key_id;
	uint8_t press_type;
} divoom_pt_ui_event;

typedef struct _divoom_pt_ui_ctrl {
	void *event_queue;
	lv_timer_t *timer;
	void *screen;
	int active;
} divoom_pt_ui_ctrl, *Pdivoom_pt_ui_ctrl;

void *divoom_product_test_ui_lvgl_show(void);
void divoom_product_test_ui_lvgl_close(void);

static Pdivoom_pt_ui_ctrl pdivoom_pt_ui_handle = NULL;

static void divoom_pt_ui_timer_cb(lv_timer_t *timer)
{
	Pdivoom_pt_ui_ctrl phandle = lv_timer_get_user_data(timer);
	divoom_pt_ui_event *event;

	if (!phandle || !phandle->event_queue)
		return;

	while (divoom_queue_items(phandle->event_queue)) {
		event = divoom_queue_pop(phandle->event_queue);
		if (!event)
			continue;

		switch (event->type) {
		case PT_UI_SHOW:
            /* Showing again (including aging) keeps the screen and its RAM
             * fonts. Only an explicit close may release this boot's cache. */
			phandle->screen = divoom_product_test_ui_lvgl_show();
			phandle->active = phandle->screen != NULL;
			break;

		case PT_UI_KEY:
			if (!divoom_product_test_is_upgrade_aging() && phandle->screen
			    && (event->press_type == DIVOOM_PRESS_SHORT
				|| event->press_type == DIVOOM_PRESS_UP
				|| event->press_type == DIVOOM_PRESS_LONG))
				divoom_product_test_screen_on_key(phandle->screen,
								  event->key_id,
								  event->press_type);
			break;

		default:
			break;
		}

		divoom_system_free(event);
	}
}

int divoom_pt_ui_init(void)
{
	if (pdivoom_pt_ui_handle)
		return pdivoom_pt_ui_handle->event_queue != NULL
			&& pdivoom_pt_ui_handle->timer != NULL;

	pdivoom_pt_ui_handle = divoom_system_malloc(sizeof(divoom_pt_ui_ctrl));
	if (!pdivoom_pt_ui_handle)
		return 0;

	memset(pdivoom_pt_ui_handle, 0, sizeof(divoom_pt_ui_ctrl));
	pdivoom_pt_ui_handle->event_queue = divoom_queue_init(32, __func__, __LINE__);
	if (!pdivoom_pt_ui_handle->event_queue) {
		divoom_system_free(pdivoom_pt_ui_handle);
		pdivoom_pt_ui_handle = NULL;
		return 0;
	}

	pdivoom_pt_ui_handle->timer = lv_timer_create(divoom_pt_ui_timer_cb, 20,
		pdivoom_pt_ui_handle);
	if (!pdivoom_pt_ui_handle->timer) {
		divoom_queue_destroy(pdivoom_pt_ui_handle->event_queue, NULL);
		divoom_system_free(pdivoom_pt_ui_handle);
		pdivoom_pt_ui_handle = NULL;
		return 0;
	}
	return 1;
}

void divoom_pt_ui_post(divoom_pt_ui_event_type type, uint32_t param)
{
	divoom_pt_ui_event *event;

	if (!pdivoom_pt_ui_handle)
		divoom_pt_ui_init();

	if (!pdivoom_pt_ui_handle || !pdivoom_pt_ui_handle->event_queue)
		return;

	event = divoom_system_malloc(sizeof(*event));
	if (!event)
		return;

	event->type = type;
	event->param = param;
	event->key_id = 0;
	event->press_type = 0;
	divoom_queue_push(pdivoom_pt_ui_handle->event_queue, event);
}

void divoom_pt_ui_post_key(uint8_t key_id, uint8_t press_type)
{
	divoom_pt_ui_event *event;

	if (!pdivoom_pt_ui_handle || !pdivoom_pt_ui_handle->event_queue)
		return;

	event = divoom_system_malloc(sizeof(*event));
	if (!event)
		return;

	event->type = PT_UI_KEY;
	event->param = 0;
	event->key_id = key_id;
	event->press_type = press_type;
	divoom_queue_push(pdivoom_pt_ui_handle->event_queue, event);
}

int divoom_pt_ui_is_active(void)
{
	return pdivoom_pt_ui_handle && pdivoom_pt_ui_handle->active;
}
