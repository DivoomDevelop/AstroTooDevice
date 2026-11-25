#include "divoom.h"
#if 0
int divoom_spiffs_manage_init(void)
{
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 5,
		.format_if_mount_failed = true
	};

	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	if (ret != ESP_OK) 
	{
		if (ret == ESP_FAIL) 
		{
			divoom_print("Failed to mount or format filesystem\n");
		} 
		else if (ret == ESP_ERR_NOT_FOUND)
		{
			divoom_print("Failed to find SPIFFS partition\n");
		} 
		else 
		{
			divoom_print("Failed to initialize SPIFFS %x\n", ret);
		}
		return 0;
	}
	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK) 
	{
		divoom_print("Failed to get SPIFFS partition information %x\n", ret);
	} 
	else
	{
		divoom_print("Partition size: total: %d, used: %d\n", total, used);
	}
	return 1;
}

void divoom_spiffs_manage_exit(void)
{
	esp_vfs_spiffs_unregister(NULL);
}
#endif


