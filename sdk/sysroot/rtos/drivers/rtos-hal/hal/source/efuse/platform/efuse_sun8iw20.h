#ifndef __EFUSE_SUN8IW20_H
#define __EFUSE_SUN8IW20_H


#ifdef __cplusplus
extern "C"
{
#endif

/* Configure rtc domain efuse power switch */
#define EFUSE_HV_SWITCH         (0x07090000 + 0x204)

/* chip ver */
#define SUNXI_SYSCFG_VER        (0x03000000 + 0x24)
#define VER_MARK                (0x7)

/* sid register */
#define SUNXI_SID_BASE          0x03006000
#define SID_PRCTL               (SUNXI_SID_BASE + 0x40)
#define SID_PRKEY               (SUNXI_SID_BASE + 0x50)
#define SID_RDKEY               (SUNXI_SID_BASE + 0x60)
#define SJTAG_AT0               (SUNXI_SID_BASE + 0x80)
#define SJTAG_AT1               (SUNXI_SID_BASE + 0x84)
#define SJTAG_S                 (SUNXI_SID_BASE + 0x88)

#define EFUSE_SRAM              (SUNXI_SID_BASE + 0x200)
#define EFUSE_SECURE_MODE       (SUNXI_SID_BASE + 0xA0)

#define SID_OP_LOCK             (0xAC)
#define EFUSE_BIT_NUM           (2048)

#define EFUSE_CHIPD             (0x00)
#define SID_CHIPD_SIZE          (128)

#define EFUSE_BROM_CONFIG       (0x10)
#define SID_BROM_CONFIG_SIZE    (16)

#define EFUSE_BROM_TRY          (0X12)
#define SID_BROM_TRY_SIZE       (16)

#define EFUSE_THERMAL_SENSOR    (0x14)
#define SID_THERMAL_SENSOR_SIZE (32)

#define EFUSE_FT_ZONE           (0x1C)
#define SID_FT_ZONE_SIZE        (128)
#define SCC_FT_ZONE_BURNED_FLAG (3)

#define EFUSE_TVOUT             (0x2C)
#define SID_TVOUT_SIZE          (32)

#define EFUSE_TVOUT_GAMMA       (0x30)
#define SID_TVOUT_GAMMA_SIZE    (64)

#define EFUSE_OEM1              (0x38)
#define SID_OEM1_SIZE           (64)

/* write protect */
#define EFUSE_WRITE_PROTECT     (0x40)
/* read  protect */
#define EFUSE_READ_PROTECT      (0x44)

#define EFUSE_HUK               (0x50)
#define SID_HUK_SIZE            (192)
#define SCC_HUK_BURNED_FLAG     (9)

#define EFUSE_ROTPK             (0x70)
#define SID_ROTPK_SIZE          (256)
#define SCC_ROTPK_BURNED_FLAG   (12)

#define EFUSE_SSK               (0x90)
#define SID_SSK_SIZE            (256)
#define SCC_SSK_BURNED_FLAG     (13)

#define EFUSE_RSSK              (0xB0)
#define SID_RSSK_SIZE           (128)
#define SCC_RSSK_BURNED_FLAG    (14)

#define EFUSE_OEM2              (0xE4)
#define SID_OEM2_SIZE           (224)

/* It can not be seen. */
#define EFUSE_PRIVATE (0)
/* After burned ,cpu can not access. */
#define EFUSE_NACCESS (1)
/* burn read_protect bit disable */
#define EFUSE_RO      (2)
/* burn read/write_protect bit disable */
#define EFUSE_RW      (3)

#define EFUSE_ACL_SET_BRUN_BIT      (1<<29)
#define EFUSE_ACL_SET_RD_FORBID_BIT (1<<30)
#define EFUSE_BRUN_RD_OFFSET_MASK   (0xFFFFFF)

#endif
