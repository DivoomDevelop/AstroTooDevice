#ifndef DIVOOM_PRODUCT_TEST_WIFI_FW_H
#define DIVOOM_PRODUCT_TEST_WIFI_FW_H

#define DIVOOM_WIFI_FW_SOURCE "/usr/aic/fmacfw_8800d80_h_u02.bin"
#define DIVOOM_WIFI_FW_BACKUP_DIR "/userdata/.wifi_fw_backup"
#define DIVOOM_WIFI_FW_BACKUP DIVOOM_WIFI_FW_BACKUP_DIR "/fmacfw_8800d80_h_u02.bin"
#define DIVOOM_WIFI_FW_BACKUP_META DIVOOM_WIFI_FW_BACKUP_DIR "/firmware.sum32"
#define DIVOOM_WIFI_FW_DOWNLOAD_PENDING "/userdata/.wifi_fw_download_pending"
#define DIVOOM_WIFI_FW_DOWNLOAD_STAGE "/userdata/tmp/.wifi_fw_download.bin"
#define DIVOOM_WIFI_FW_REMOTE "AstroToo/usr/aic/fmacfw_8800d80_h_u02.bin"

/* All functions return 1 on success, 0 on I/O/verification failure. */
/* Normal-app boot only, after selecting the boot mode: validate or repair the
 * permanent /userdata copy. Never run this before entering product test. */
int divoom_product_test_wifi_fw_maintain_backup(void);
/* Quiesce recovery I/O before either partition is unmounted/formatted. */
int divoom_product_test_wifi_fw_recovery_pause(void);
void divoom_product_test_wifi_fw_recovery_resume(void);
/* 1 verified, 0 no valid file, -1 temporarily unable to verify. */
int divoom_product_test_wifi_fw_has_copy(int usr);
/* Ensure a verified file survives erasing the selected volume. */
int divoom_product_test_wifi_fw_safe_to_erase(int target_usr);
/* Make the driver fallback usable without writing to the system volume. */
int divoom_product_test_wifi_fw_prepare_network(void);
/* Normal-app startup, or the format-recovery worker after test UI entry. */
int divoom_product_test_wifi_fw_restore(void);
/* Only call after the complete system resource restore succeeds. */
int divoom_product_test_wifi_fw_cleanup(void);

#endif
