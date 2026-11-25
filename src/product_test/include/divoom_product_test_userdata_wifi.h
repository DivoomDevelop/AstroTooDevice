#ifndef DIVOOM_PRODUCT_TEST_USERDATA_WIFI_H
#define DIVOOM_PRODUCT_TEST_USERDATA_WIFI_H

#define DIVOOM_PT_USERDATA_WIFI_BACKUP "/usr/.userdata_wifi_recovery.json"
/* Presence is a durable automatic-userdata-recovery intent. Never store a
 * backup on the volume which is about to be erased. */
int divoom_product_test_userdata_wifi_pending(void);
/* Returns a caller-owned, validated system configuration with device_id reset. */
char *divoom_product_test_userdata_config_snapshot(void);
int divoom_product_test_userdata_wifi_prepare(void);
int divoom_product_test_userdata_wifi_restore(void);
int divoom_product_test_userdata_wifi_finish(void);
#endif
