#ifndef __USB_CHARGE_H__
#define __USB_CHARGE_H__

#include "../include/usb_os_platform.h"
#include "../host/sunxi-hci.h"

/*USB BC reg */
#define USB_HCI_BC_CTRL			0x82c

#define USB_BC_DP_DBC_BYP_BIT		28
#define USB_BC_DPDM_DLY_SEL_BIT		24
#define USB_BC_DP_VOL_BIT		10
#define USB_BC_DM_VOL_BIT		4
#define USB_BC_DP_DLY_BYP_BIT		2
#define USB_BC_DM_DRV_EN_BIT		1
#define USB_BC_DP_DET_EN_BIT		0

enum usb_bc_12_dp_det_vol {
	BC_12_DP_DET_042_062V = 0,
	BC_12_DP_DET_044_064V,
	BC_12_DP_DET_046_066V,
	BC_12_DP_DET_048_068V,
	BC_12_DP_DET_050_070V,
	BC_12_DP_DET_052_072V,
	BC_12_DP_DET_054_074V,
	BC_12_DP_DET_056_076V,
};

enum usb_bc_12_dm_out_vol {
	BC_12_DM_OUT_052V = 0,
	BC_12_DM_OUT_054V,
	BC_12_DM_OUT_056V,
	BC_12_DM_OUT_058V,
	BC_12_DM_OUT_060V,
	BC_12_DM_OUT_062V,
	BC_12_DM_OUT_064V,
	BC_12_DM_OUT_066V,
};

#ifdef CONFIG_USB_HOST_BC_CHARGE
void sunxi_start_host_bc_12_cdp(uint8_t dp_det_vol, uint8_t dm_out_vol, bool dpdm_dly_en, uint8_t dpdm_dly_sel);
void sunxi_close_host_bc_12_cdp(void);
#else
static inline void sunxi_start_host_bc_12_cdp(uint8_t dp_det_vol, uint8_t dm_out_vol, bool dpdm_dly_en, uint8_t dpdm_dly_sel) {}
static inline void sunxi_close_host_bc_12_cdp(void) {}

#endif

#endif /* CONFIG_USB_HOST_BC_CHARGE */
