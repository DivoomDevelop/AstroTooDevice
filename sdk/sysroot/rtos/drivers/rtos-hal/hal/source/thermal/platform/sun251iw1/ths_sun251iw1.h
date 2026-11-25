#ifndef THS_SUN251IW1_H
#define THS_SUN251IW1_H

#define SUN251IW1_THS_EFUSE_OFF (0x03006214)
#define SUN251IW1_CAL_COM (0)
#define SUN251IW1_SENSOR_DATA_CODE (1769)
#define SUN251IW1_OFFSET_BELOW (-2822)
#define SUN251IW1_SCALE_BELOW (-62)
#define SUN251IW1_OFFSET_ABOVE (-2841)
#define SUN251IW1_SCALE_ABOVE (-58)

int hal_ths_reg2temp(int reg);
int hal_ths_drv_init(void);

#endif