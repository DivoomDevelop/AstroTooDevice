#ifndef THS_SUN252IW2_H
#define THS_SUN252IW2_H

#include "../../../ccu/platform/sun252iw2/sun252iw2_sys_ccu.h"

#define SUN252IW2_THS_EFUSE_OFF (0x03006214)
#define SUN252IW2_CAL_COM (0)
#define SUN252IW2_SENSOR_DATA_CODE (1839)
#define SUN252IW2_OFFSET_BELOW (-2822)
#define SUN252IW2_SCALE_BELOW (-66)
#define SUN252IW2_OFFSET_ABOVE (-2835)
#define SUN252IW2_SCALE_ABOVE (-65)

#define THS_BGR_REG		(SYS_CCU_REG_BASE + 0x9fc)

#define GATING_CLK_SHIFT		(0)
#define GATING_RST_SHIFT		(16)

#define GATING_CLK_MASK			(0x1 << GATING_CLK_SHIFT)
#define GATING_RST_MASK			(0x1 << GATING_RST_SHIFT)

#define THERMAL_GPADC_PORT	(0)

int hal_ths_reg2temp(int reg);
int hal_ths_drv_init(void);
int hal_ths_drv_deinit(void);

#endif
