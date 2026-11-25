#ifndef __EFUSE_SUN252IW2_H
#define __EFUSE_SUN252IW2_H


#ifdef __cplusplus
extern "C"
{
#endif

/* efuse power */
#define EFUSE_HV_SWITCH		(0x03090000 + 0x204)

/* chip ver */
#define SUNXI_SYSCFG_VER	(0x03000000 + 0x24)
#define VER_MARK		(0x7)

/* sid register */
#define SUNXI_SID_BASE		0x03006000
#define SID_PRCTL		(SUNXI_SID_BASE + 0x00)
#define SID_PR_ADDR		(SUNXI_SID_BASE + 0x04)
#define SID_PRKEY		(SUNXI_SID_BASE + 0x08)
#define SID_RDKEY		(SUNXI_SID_BASE + 0x0C)
#define SID_WRITE_OP_LOCK  	(0xE0C9)
#define SID_READ_OP_LOCK  	(0xADBF)
#define SID_OP_LOCK_OFFSET	(16)
#define SID_OP_LOCK_MASK	(0xffff << SID_OP_LOCK_OFFSET)
#define SID_PG_START		(0x1)
#define SID_READ_START		(0x2)
#define SID_PG_INDEX_OFFSET	(0)
#define SID_PG_INDEX_MASK	(0xF)
#define SID_PR_START_MASK	(0x3)
#define SID_PR_CTRL_MASK	(SID_OP_LOCK_MASK | SID_PR_START_MASK)
#define SID_PG_INDEX_ENABLE_MASK SID_PG_INDEX_MASK
#define SID_PG_START_ENABLE_MASK SID_PR_CTRL_MASK
#define SID_BYTES_PER_WORD	(4)

/* efuse size */
#define EFUSE_BIT_NUM		(512)

/* efuse map */
#define EFUSE_CHIPD             (0x00)
#define SID_CHIPD_SIZE          (128)

#define EFUSE_BROM_CONF		(0x10)
#define SID_BROM_CONF_SIZE	(32)

#define EFUSE_THERMAL_SENSOR	(0x14)
#define SID_THERMAL_SENSOR_SIZE	(32)

#define EFUSE_RES		(0x18)
#define SID_RES_SIZE		(16)

#define EFUSE_SEC_CTRL		(0x1C)
#define SID_SEC_CTRL_SIZE	(32)

#define EFUSE_SSK		(0x20)
#define SID_SSK_SIZE		(128)
#define SCC_SSK_BURNED_FLAG	(-1)

#define EFUSE_OEM1		(0x30)
#define SID_OEM1_SIZE		(96)

#define EFUSE_FT_ZONE		(0x3C)
#define SID_FT_ZONE_SIZE	(16)

/* efuse access level */
#define EFUSE_PRIVATE (0)
#define EFUSE_NACCESS (1)
#define EFUSE_RO (2)
#define EFUSE_RW (3)

#define EFUSE_ACL_SET_BRUN_BIT      (0)
#define EFUSE_ACL_SET_RD_FORBID_BIT (0)
#define EFUSE_BRUN_RD_OFFSET_MASK    (0x0)

#endif
