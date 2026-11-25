#ifndef _HAL_SDPIN_H_
#define _HAL_SDPIN_H_

extern uint32_t sdmmc_pinctrl_init(struct mmc_host *host);
extern int mmc_gpiod_request_cd_irq(struct mmc_host* host);
extern void sdmmc_reset_card(struct mmc_host *host, bool enable);
#endif /* _ROM_DRIVER_CHIP_SDMMC__SDIO_H_ */
