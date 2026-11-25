#ifndef __MMC_SUN252IW2_H__
#define __MMC_SUN252IW2_H__

#include <hal_gpio.h>

//#define SDC_FPGA

#define SMC0_BASE                       (0x04020000)
#define SMC1_BASE                       (0x04021000)
#define SMC2_BASE                       (0x04022000)

#define SDC_CCM_BASE                    (0x2001000)
#define SDC_GPIO_BASE                   (0x2000000)

#ifdef CONFIG_ARCH_SUN252IW2
#define __GIC_SRC_MMC0        40
#define __GIC_SRC_MMC1        41
#define __GIC_SRC_MMC2	      42
#else
#define __GIC_SRC_MMC0        72
#define __GIC_SRC_MMC1        73
#define __GIC_SRC_MMC2	      74
#endif

#define  SDC0_IRQn  __GIC_SRC_MMC0
#define  SDC1_IRQn  __GIC_SRC_MMC1
#define  SDC2_IRQn  __GIC_SRC_MMC2

#define SDMMC_MUXSEL 2
#define SDMMC_DRVSEL 3
#define SDMMC_PULL GPIO_PULL_DOWN_DISABLE

#define SDC_PLL_CLK (600*1000*1000)

#ifdef HAL_SetPin
/*sdc0 pin*/
#define SDC0_NUM 6
#define SDC0_CLK    GPIO_PF2
#define SDC0_CMD    GPIO_PF3
#define SDC0_D0    GPIO_PF1
#define SDC0_D1    GPIO_PF0
#define SDC0_D2    GPIO_PF5
#define SDC0_D3    GPIO_PF4
#define SDC0_DET    GPIO_PF6

/*sdc1 pin*/
#define SDC1_NUM 6
#define SDC1_CLK    GPIO_PG0
#define SDC1_CMD    GPIO_PG1
#define SDC1_D0    GPIO_PG2
#define SDC1_D1    GPIO_PG3
#define SDC1_D2    GPIO_PG4
#define SDC1_D3    GPIO_PG5

#define SDC2_NUM 6
#endif

#define SDC_DES_ADDR_SHIFT (2)

#define SDC_MAX_FREQ_SDR_1V8 100000000

#endif
