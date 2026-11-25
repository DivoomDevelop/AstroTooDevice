#ifndef _AIC_FW_H_
#define _AIC_FW_H_

#include <stddef.h>
#include <stdint.h>

/* Byte-wise sum modulo 2^32, shared by loading and file recovery. */
static inline uint32_t aic8800_fw_sum32_update(uint32_t sum,
    const uint8_t *data, size_t size)
{
    while (size--) sum += *data++;
    return sum;
}

/* External D80 firmware is read from the mounted filesystem in small blocks. */
#ifndef AIC8800_FW_DIRECTORY
#define AIC8800_FW_DIRECTORY "/usr/aic"
#endif

#define AIC8800M40_FW_FILE        AIC8800_FW_DIRECTORY "/fmacfw_8800m40.bin"
#define AIC8800D80_U02_FW_FILE    AIC8800_FW_DIRECTORY "/fmacfw_8800d80_u02.bin"
#define AIC8800D80_H_U02_FW_FILE  AIC8800_FW_DIRECTORY "/fmacfw_8800d80_h_u02.bin"
/* Byte sum of the shipped image; update both values when replacing it. */
#define AIC8800D80_H_U02_FW_SIZE  343824U
#define AIC8800D80_H_U02_FW_SUM32 0x020d72ccU
/* CRC32 of the same shipped image, used by the streaming loader. */
#define AIC8800D80_H_U02_FW_CRC32 0xea449bbeU
#define AIC8800D80_H_U02_FW_BACKUP_FILE "/userdata/fmacfw_8800d80_h_u02.bin"
/* Recovery may use a new name when the usual directory entry cannot be written. */
#define AIC8800D80_H_U02_FW_USR_ALT "/usr/.aic-fw-backup."
#define AIC8800D80_H_U02_FW_USERDATA_ALT "/userdata/.aic-fw-backup."
#define AIC8800D80_H_U02_FW_ALT_COUNT 5
#define AIC8800D80_RF_U02_FW_FILE AIC8800_FW_DIRECTORY "/lmacfw_rf_8800d80_u02.bin"
#define AIC8800D80_BT_ADID_U02_FILE AIC8800_FW_DIRECTORY "/fw_adid_8800d80_u02.bin"
#define AIC8800D80_BT_PATCH_U02_FILE AIC8800_FW_DIRECTORY "/fw_patch_8800d80_u02.bin"
#define AIC8800D80_BT_PATCH_TABLE_U02_FILE AIC8800_FW_DIRECTORY "/fw_patch_table_8800d80_u02.bin"
#define AIC8800D80_BT_PATCH_EXT_U02_FILE AIC8800_FW_DIRECTORY "/fw_patch_8800d80_u02_ext0.bin"

enum aic_fw {
    FW_UNKNOWN = 0,
    FMACFW,
    FMACFW_RF,
    FW_ADID,
    FW_PATCH,
    FW_PATCH_TABLE,
    FW_ADID_U03,
    FW_PATCH_U03,
    FW_PATCH_TABLE_U03,
    FMACFW_8800DC,
    FMACFW_RF_8800DC,
    FMACFW_PATCH_8800DC,
    FMACFW_RF_PATCH_8800DC,
    LMACFW_RF_8800DC,
    FMACFW_PATCH_8800DC_U02,
    FMACFW_PATCH_TBL_8800DC_U02,
    FW_ADID_8800DC,
    FW_PATCH_8800DC,
    FW_PATCH_TABLE_8800DC,
    FW_ADID_8800DC_U02,
    FW_PATCH_8800DC_U02,
    FW_PATCH_TABLE_8800DC_U02,
    FW_PATCH_8800DC_U02_EXT,
    FW_ADID_8800D80,
    FW_ADID_8800D80_U02,
    FMACFW_8800D80,
    FMACFW_8800D80_U02,
    FMACFW_RF_8800D80,
    FMACFW_RF_8800D80_U02,
    FW_PATCH_8800D80,
    FW_PATCH_8800D80_U02,
    FW_PATCH_TABLE_8800D80,
    FW_PATCH_TABLE_8800D80_U02,
    FW_PATCH_8800D80_U02_EXT,
    FW_PATCH_8800D80_U02_EXT1,
};

void *aic8800d_fw_ptr_get(void);
uint32_t aic8800d_fw_size_get(void);
void *aic8800d_rf_fw_ptr_get(void);
uint32_t aic8800d_rf_fw_size_get(void);
void *aic8800dc_u01_fw_ptr_get(void);
uint32_t aic8800dc_u01_fw_size_get(void);
void *aic8800dc_u02_fw_ptr_get(void);
uint32_t aic8800dc_u02_fw_size_get(void);
void *aic8800dc_u02_calib_fw_ptr_get(void);
uint32_t aic8800dc_u02_calib_fw_size_get(void);
void *aic8800dc_h_u02_fw_ptr_get(void);
uint32_t aic8800dc_h_u02_fw_size_get(void);
void *aic8800dc_h_u02_calib_fw_ptr_get(void);
uint32_t aic8800dc_h_u02_calib_fw_size_get(void);
void *aic8800dc_rf_lmacfw_ptr_get(void);
uint32_t aic8800dc_rf_lmacfw_size_get(void);
void *aic8800dc_rf_fmacfw_ptr_get(void);
uint32_t aic8800dc_rf_fmacfw_size_get(void);
void *aic8800dc_u02_patch_tbl_ptr_get(void);
uint32_t aic8800dc_h_u02_patch_tbl_size_get(void);
void *aic8800dc_h_u02_patch_tbl_ptr_get(void);
uint32_t aic8800dc_u02_patch_tbl_size_get(void);
void *aic8800d80_fw_ptr_get(void);
uint32_t aic8800d80_fw_size_get(void);
void *aic8800d80_rf_fw_ptr_get(void);
uint32_t aic8800d80_rf_fw_size_get(void);
void *aic8800d80_u02_fw_ptr_get(void);
uint32_t aic8800d80_u02_fw_size_get(void);
void *aic8800d80_u02_rf_fw_ptr_get(void);
uint32_t aic8800d80_u02_rf_fw_size_get(void);

void *aic8800d80_h_u02_fw_ptr_get(void);
uint32_t aic8800d80_h_u02_fw_size_get(void);
void *aic_fw_ptr_get(enum aic_fw name);
uint32_t aic_fw_size_get(enum aic_fw name);

void *aic8800d80n_u02_fw_patch_ptr_get(void);
uint32_t aic8800d80n_u02_fw_size_get(void);
void *aic8800d80n_u02_patch_tbl_ptr_get(void);
uint32_t aic8800d80n_u02_patch_tbl_size_get(void);
void *aic8800d80n_u02_calib_fw_ptr_get(void);
uint32_t aic8800d80n_u02_calib_fw_size_get(void);
void *aic8800d80n_u02_cinit_fw_ptr_get(void);
uint32_t aic8800d80n_u02_cinit_fw_size_get(void);
#if defined(CONFIG_WIFI_MODE_RFTEST)
void *aic8800d80n_rf_fw_ptr_get(void);
uint32_t aic8800d80n_rf_fw_size_get(void);
#endif /* CONFIG_WIFI_MODE_RFTEST */

/* Chip info accessors used by the F101 platform integration. */
uint8_t aic_get_chip_sub_id(void);
uint8_t aic_get_chip_mcu_id(void);
uint8_t aic_get_chip_id(void);
int aic_is_chip_id_h(void);

/* F101/M40 firmware accessors retained for the existing platform driver. */
void *aic8800m40_fw_ptr_get(void);
uint32_t aic8800m40_fw_size_get(void);
void *fmac_aic8800m40_fw_ptr_get(void);
uint32_t fmac_aic8800m40_fw_size_get(void);
#endif
