#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "interrupt.h"
#include <portmacro.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sound_adf_ksound.h"
#ifdef CONFIG_DRIVERS_SOUND_LITE
#include "sunxi_sound_lite_core.h"
#endif
#include <console.h>
#include <divoom_crash_log.h>
#include "divoom_boot_health.h"
#include "divoom_recovery_journal.h"
#include "storage_stress_test.h"

#ifdef CONFIG_DRIVERS_SPINOR
#include <sunxi_hal_spinor.h>
#endif

#ifdef CONFIG_COMPONENT_LITTLEFS
#include <littlefs.h>
#endif

#ifdef CONFIG_COMPONENTS_AW_DEVFS
#include <devfs.h>
#endif

#ifdef CONFIG_COMPONENTS_PM
extern int pm_init(int argc, char **argv);
#endif



#ifdef CONFIG_COMMAND_AUTO_START_MEMTESTER
static void auto_memtester_thread(void *param)
{
	int argc=3;
	char *argv[] ={"", "-s", "128K"};
	extern int cmd_memtest();
	cmd_memtest(argc, &argv);
	hal_thread_stop(NULL);
}
#endif

void cpu0_app_entry(void *param)
{
	(void)param;

#ifdef CONFIG_DRIVER_G2D_200
extern int g2d_probe(void);
	g2d_probe();
#endif

#ifdef CONFIG_DISP2_SUNXI
extern int disp_probe(void);
	disp_probe();
#endif

#ifdef CONFIG_DRIVERS_SPILCD
extern int spilcd_fb_probe(void);
       spilcd_fb_probe();
#endif

#ifdef CONFIG_COMPONENTS_PM
	pm_init(1, NULL);
#endif

#ifdef CONFIG_COMPONENTS_AW_DEVFS
	devfs_mount("/dev");
#endif

#if defined(CONFIG_COMPONENT_CLI) && !defined(CONFIG_UART_MULTI_CONSOLE_AS_MAIN) && !defined(CONFIG_DRIVERS_SDMMC_DEVFS)
	vCommandConsoleStart(0x1000, HAL_THREAD_PRIORITY_CLI, NULL);
#endif

#ifdef CONFIG_DRIVERS_SPINOR
    int flash_ready = !hal_spinor_init(0);
#elif defined(CONFIG_DRIVERS_NAND_FLASH)
    extern int hal_nand_init(void);
    int flash_ready = !hal_nand_init();
#endif

    /*
     * SD NAND GPT: usr -> /usr, UDISK -> /userdata
     */
#ifdef CONFIG_DRIVERS_SDMMC_DEVFS
    extern int sdmmc_boot_devfs_init(void);
    extern int sdmmc_mount_devpath(const char *devpath, const char *mount_path);
    extern void divoom_boot_ab_check(void);
    extern void divoom_uboot_check(void);
    extern void divoom_boot_ab_storage_unavailable(void);
    if (sdmmc_boot_devfs_init() == 0)
    {
        divoom_uboot_check();
        divoom_boot_ab_check();
        divoom_crash_log_init();
        /* Establish pending-volume write barriers before logs, Wi-Fi or
         * firmware maintenance can allocate clusters on a damaged volume. */
        divoom_recovery_journal_boot();
        int mount_result;
        /* Try both volumes independently; recovery repairs userdata first.
         * The FAT hook still refuses an automatic /usr erase if userdata
         * cannot hold its durable recovery marker. */
        mount_result = sdmmc_mount_devpath("/dev/UDISK", "/userdata");
        divoom_boot_health_set_mount_failed(0, mount_result != 0);
        printf("boot_mount: /userdata result=%d\n", mount_result);
        mount_result = sdmmc_mount_devpath("/dev/usr", "/usr");
        divoom_boot_health_set_mount_failed(1, mount_result != 0);
        printf("boot_mount: /usr result=%d\n", mount_result);
        /* Pending crash stays protected until the connected upload path reads it. */
    }
    else
        divoom_boot_ab_storage_unavailable();
#endif

#if defined(CONFIG_COMPONENT_CLI) && !defined(CONFIG_UART_MULTI_CONSOLE_AS_MAIN) && defined(CONFIG_DRIVERS_SDMMC_DEVFS)
    /* Capture the boot selection before CLI env commands can change it. */
    vCommandConsoleStart(0x1000, HAL_THREAD_PRIORITY_CLI, NULL);
#endif

#ifdef CONFIG_VFS_SUPPORT_NOR
    int elmfat_mount_spinor(const char *dev, const char *base_path);
    elmfat_mount_spinor("usr", "/usr");
#endif
#ifdef CONFIG_COMPONENT_LITTLEFS
    if (flash_ready)
    {
        littlefs_mount("/dev/UDISK", "/userdata", true);
        #ifdef CONFIG_COMPONENT_SECURE_STORAGE
        littlefs_mount("/dev/secret", "/secret", true);
        #endif
    }
    else
    {
        printf("flash not ready, skip mount\n");
    }
#endif

#if 0
#ifdef CONFIG_LVGL9_METER_DEMO
extern int lv9_meter_demo_main(int argc, char **argv);
    lv9_meter_demo_main(1, NULL);
#endif
#endif

#ifdef CONFIG_COMPONENTS_RAMFS_AUTO_MOUNT
extern int ramfs_create_mount(char *path, size_t size);
    ramfs_create_mount(CONFIG_COMPONENTS_RAMFS_PATH, CONFIG_COMPONENTS_RAMFS_SIZE * 1024);
#endif

#ifdef CONFIG_CHERRYUSB
#ifdef CONFIG_CHERRYUSB_DEVICE
#if defined(CONFIG_CHERRYUSB_DEVICE_TEMPLATE_ADB)
        extern int cherryadb_startup(void);
        cherryadb_startup();
#elif defined(CONFIG_CHERRYUSB_DEVICE_TEMPLATE_MSC_DUAL)
        #if 0
        extern int msc_dual_startup(void);
        msc_dual_startup();
        #endif
#endif
#endif
#endif

#ifdef CONFIG_COMMAND_AUTO_START_MEMTESTER
	void *autotest_thread;
	autotest_thread = hal_thread_create(auto_memtester_thread, NULL,
			"auto_memtester", 8 * 1024, HAL_THREAD_PRIORITY_SYS);

	if (autotest_thread != NULL)
		hal_thread_start(autotest_thread);
#endif

#ifdef CONFIG_DRIVERS_SOUND_V2
	ksound_card_initialize();
#elif defined(CONFIG_DRIVERS_SOUND_LITE)
	sound_lite_load_card();
#endif

#ifndef CONFIG_DRIVERS_SDMMC_DEVFS
#if (defined CONFIG_DRIVERS_SDMMC) && (defined CONFIG_COMPONENT_ELMFAT)
    extern int sunxi_driver_sdmmc_init(void);
    sunxi_driver_sdmmc_init();
#endif
#endif
    /* PNG/JPEG hardware decoder handles live for the entire system lifetime. */
    extern int divoom_aw_pic_decoder_global_init(void);
    if (divoom_aw_pic_decoder_global_init() != 0)
    {
        printf("[PIC DECODER] global decoder init failed\n");
    }
    extern void divoom_lvgl_boot(void);
	divoom_lvgl_boot();
	vTaskDelete(NULL);
}
