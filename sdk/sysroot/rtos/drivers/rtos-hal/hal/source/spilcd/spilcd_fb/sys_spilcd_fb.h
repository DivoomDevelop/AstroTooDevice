/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __SYS_SPILCD_FB_H__
#define __SYS_SPILCD_FB_H__

#include <stdbool.h>
#include <stdint.h>
#include <hal_spilcd_fb.h>

/**
 * @brief SPI LCD帧缓冲区信息结构体
 *
 * 该结构体用于描述SPI LCD显示设备的帧缓冲区属性，
 * 包括分辨率、内存地址、缓冲区大小和像素格式等信息。
 */
typedef struct spilcd_fb_info {
    uint32_t width;                     /**< 屏幕宽度（像素数） */
    uint32_t height;                    /**< 屏幕高度（像素数） */
    uint8_t *addr;                      /**< 帧缓冲区内存地址指针 */
    uint32_t size;                      /**< 帧缓冲区总大小（字节） */
    enum spilcdfb_pixel_format fmt;     /**< 像素格式（如RGB565、RGB888等） */
} spilcd_fb_t;

/**
 * @brief SPI LCD命令枚举
 *
 * 定义SPI LCD设备支持的各种控制命令，用于设备配置和状态查询。
 * 这些命令通过ioctl系统调用或类似的设备控制接口使用。
 */
typedef enum spilcd_cmd {
    SPILCD_GET_SCN_WIDTH,       /**< 获取屏幕宽度命令 */
    SPILCD_GET_SCN_HEIGHT,      /**< 获取屏幕高度命令 */
    SPILCD_SET_BRIGHTNESS,      /**< 设置屏幕亮度命令 */
    SPILCD_GET_BRIGHTNESS,      /**< 获取当前亮度值命令 */
    SPILCD_SET_FB,              /**< 设置帧缓冲区命令 */
    SPILCD_GET_FB,              /**< 获取帧缓冲区信息命令 */
    SPILCD_GET_DATA_FORMAT,     /**< 获取像素数据格式命令 */
    SPILCD_FB_CMD_INVALID,      /**< 无效命令标识（边界检查用） */
} spilcd_cmd_e;

#endif /* __SYS_SPILCD_FB_H__ */


