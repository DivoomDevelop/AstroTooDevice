/*
 * AstroToo LCD 背光硬件控制（PWM + DISP_LCD_SET_BRIGHTNESS）
 * 由 divoom_screen_bright() 或 LVGL 启动阶段调用。
 *
 * 业务层 0–100 按人眼明度（CIE L*）理解，再反变换成相对亮度去驱动 PWM。
 * 线性占空会让低档偏亮、高档几乎无差别；L* 反变换后滑条步进更均匀。
 */
#include <string.h>

#if defined(CONFIG_DRIVERS_SUNXI_PWM)
#include <sunxi_hal_pwm_ng.h>
#endif
#if defined(CONFIG_DISP2_SUNXI)
#include <video/sunxi_display2.h>
extern int disp_ioctl(int cmd, void *arg);
#endif

#define DIVOOM_SCREEN_BL_PWM_PORT		0
#define DIVOOM_SCREEN_BL_PWM_CH			2
#define DIVOOM_SCREEN_BL_PWM_PERIOD_NS	20000
#define DIVOOM_SCREEN_BL_PWM_POLARITY	1

/*
 * UI 0–100 → PWM 0–255。Y = CIE L* 反变换：
 *   L* <= 8: Y = L* / 903.3
 *   L*  > 8: Y = ((L* + 16) / 116)^3
 * 非 0 至少 1/255，避免灯驱吃掉极窄脉冲。
 */
static const unsigned char divoom_screen_bl_perc_to_pwm255[101] = {
	  0,   1,   1,   1,   1,   1,   2,   2,   2,   3,
	  3,   3,   4,   4,   4,   5,   5,   6,   6,   7,
	  8,   8,   9,  10,  10,  11,  12,  13,  14,  15,
	 16,  17,  18,  19,  20,  22,  23,  24,  26,  27,
	 29,  30,  32,  34,  35,  37,  39,  41,  43,  45,
	 47,  49,  51,  54,  56,  58,  61,  64,  66,  69,
	 72,  75,  78,  81,  84,  87,  90,  93,  97, 100,
	104, 108, 111, 115, 119, 123, 127, 131, 136, 140,
	145, 149, 154, 159, 163, 168, 173, 179, 184, 189,
	195, 200, 206, 212, 217, 223, 230, 236, 242, 248,
	255
};

static unsigned int divoom_screen_bl_percent_to_pwm255(int brightness)
{
	if (brightness <= 0)
		return 0;
	if (brightness > 100)
		brightness = 100;
	return (unsigned int)divoom_screen_bl_perc_to_pwm255[brightness];
}

void divoom_screen_hw_backlight_set(int brightness)
{
	unsigned int pwm255;

	pwm255 = divoom_screen_bl_percent_to_pwm255(brightness);

#if defined(CONFIG_DRIVERS_SUNXI_PWM)
	{
		struct pwm_config cfg;
		unsigned int duty_ns;
		static int pwm_inited = 0;

		duty_ns = pwm255 * DIVOOM_SCREEN_BL_PWM_PERIOD_NS / 255;
		if (!pwm_inited) {
			hal_pwm_init(DIVOOM_SCREEN_BL_PWM_PORT);
			pwm_inited = 1;
		}
		memset(&cfg, 0, sizeof(cfg));
		cfg.period_ns = DIVOOM_SCREEN_BL_PWM_PERIOD_NS;
		cfg.duty_ns = duty_ns;
		cfg.polarity = DIVOOM_SCREEN_BL_PWM_POLARITY;
		cfg.output_mode = false;
		hal_pwm_control(DIVOOM_SCREEN_BL_PWM_PORT, DIVOOM_SCREEN_BL_PWM_CH, &cfg);
	}
#endif

#if defined(CONFIG_DISP2_SUNXI)
	{
		unsigned long args[2];

		/* 与 PWM 同一套物理值，避免 DISP 再按线性覆盖占空 */
		args[0] = 0;
		args[1] = (unsigned long)pwm255;
		disp_ioctl(DISP_LCD_SET_BRIGHTNESS, args);
	}
#endif
}
