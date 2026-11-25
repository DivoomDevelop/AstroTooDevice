#ifndef DIVOOM_BUILD_CONFIG_H
#define DIVOOM_BUILD_CONFIG_H

/*
 * 双构建判定：仅 F101 真机 RTOS 与 Windows 模拟器（无 Linux/RK 第三套）。
 *
 * 模拟器：VS -DDIVOOM_SIMULATOR（常伴 _WIN32）
 * 真机：  gerate_firmware / riscv 交叉编译，不定义上述宏
 */
#if defined(DIVOOM_SIMULATOR) || defined(_WIN32)
#define DIVOOM_BUILD_RTOS_DEVICE  0
#define DIVOOM_BUILD_SIMULATOR    1
#else
#define DIVOOM_BUILD_RTOS_DEVICE  1
#define DIVOOM_BUILD_SIMULATOR    0
#endif

/*
 * Finsh 系统自动化测试 CLI。由 menuconfig 控制：
 *   Divoom AstroToo → Enable Finsh autotest CLI commands (CONFIG_DIVOOM_FINSH_AUTOTEST)
 * 默认关闭；量产固件不注册测试命令。
 */
#if defined(CONFIG_DIVOOM_FINSH_AUTOTEST) && CONFIG_DIVOOM_FINSH_AUTOTEST
#define DIVOOM_FINSH_AUTOTEST 1
#else
#define DIVOOM_FINSH_AUTOTEST 0
#endif

/* OTA resume fault-injection CLI only; normal OTA resume is always available. */
#ifndef DIVOOM_OTA_RESUME_TEST_ENABLE
#define DIVOOM_OTA_RESUME_TEST_ENABLE 0
#endif

/* MQTT 1883 fault-injection CLI only; production fallback is always available. */
#ifndef DIVOOM_MQTT_PORT_TEST_ENABLE
#define DIVOOM_MQTT_PORT_TEST_ENABLE 0
#endif

#ifndef DIVOOM_PIXEL_EQ_ENABLE
#define DIVOOM_PIXEL_EQ_ENABLE  0
#endif

/*
 * 展示机总闸（与杰理 divoom_config_define.h 同名、同取值）。
 * 0 = 量产；1 = 展示机。编译命令 -DDIVOOM_SHOW_MODE=1 可覆盖（show_mode_firmware.sh）。
 * SHOW_TF_NAND / SHOW_MUSIC_FFT 挂在总闸下，量产即使误传子宏也强制关掉。
 * 展机完整版本由 DIVOOM_SHOW_OVERSEAS 决定：国外 533001 / 国内 534001；量产 530049。
 */
#ifndef DIVOOM_SHOW_MODE
#define DIVOOM_SHOW_MODE  0
#endif
/* 展机地区。1 = 国外（轮播国外表，版本 533001）；0 = 国内（轮播国内表，版本 534001）。 */
#ifndef DIVOOM_SHOW_OVERSEAS
#define DIVOOM_SHOW_OVERSEAS  0
#endif
#ifndef DIVOOM_SHOW_TF_NAND
#define DIVOOM_SHOW_TF_NAND DIVOOM_SHOW_MODE
#endif
#if !DIVOOM_SHOW_MODE
#undef DIVOOM_SHOW_TF_NAND
#define DIVOOM_SHOW_TF_NAND 0
#endif
#ifndef DIVOOM_SHOW_MUSIC_FFT
#define DIVOOM_SHOW_MUSIC_FFT DIVOOM_SHOW_TF_NAND
#endif
#if !DIVOOM_SHOW_TF_NAND
#undef DIVOOM_SHOW_MUSIC_FFT
#define DIVOOM_SHOW_MUSIC_FFT 0
#endif

#endif /* DIVOOM_BUILD_CONFIG_H */
