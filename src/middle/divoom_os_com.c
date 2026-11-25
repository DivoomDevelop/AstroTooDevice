#include "divoom.h"
#include "divoom_os_com.h"
#if defined(CONFIG_COMPONENTS_LVGL9)
#include "divoom_lvgl_init.h"
#endif

#if DIVOOM_BUILD_RTOS_DEVICE && defined(CONFIG_DISP2_SUNXI)
#include <video/sunxi_display2.h>
extern s32 bsp_disp_device_switch(int disp, enum disp_output_type output_type,
				  enum disp_tv_mode mode);
extern void disp_disable_update(void);
#endif

typedef struct _divoom_os_com_ctrl {
	int lcd_disabled;
} divoom_os_com_ctrl, *Pdivoom_os_com_ctrl;

static divoom_os_com_ctrl s_os_com = {0};
static Pdivoom_os_com_ctrl pdivoom_os_com_handle = &s_os_com;

int divoom_os_com_lcd_is_disabled(void)
{
	return pdivoom_os_com_handle->lcd_disabled;
}

void divoom_os_com_lcd_disable(void)
{
#if DIVOOM_BUILD_RTOS_DEVICE && defined(CONFIG_DISP2_SUNXI)
#if defined(CONFIG_COMPONENTS_LVGL9)
	/* Drain the handler's in-flight flush before detaching its display device. */
	divoom_get_lvgl_lock();
#endif
	if (pdivoom_os_com_handle->lcd_disabled)
	{
#if defined(CONFIG_COMPONENTS_LVGL9)
		divoom_free_lvgl_lock();
#endif
		return;
	}
	pdivoom_os_com_handle->lcd_disabled = 1;
	disp_disable_update();
	divoom_print("[LCD] shutdown close_flow\n");
	(void)bsp_disp_device_switch(0, DISP_OUTPUT_TYPE_NONE, 0);
#if defined(CONFIG_COMPONENTS_LVGL9)
	divoom_free_lvgl_lock();
#endif
#else
	(void)pdivoom_os_com_handle;
#endif
}
