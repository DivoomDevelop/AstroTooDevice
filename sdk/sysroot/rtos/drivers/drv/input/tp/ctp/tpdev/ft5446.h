/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.
 *
 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _FT5446_H_
#define _FT5446_H_

#include <typedef.h>

#include "../sunxi_ctp_dev.h"

// FT5446 I2C Address (7-bit address)
// #define FT5446_I2C_ADDR          0x38

// FT5446 I2C Address (8-bit address)
#define FT5446_I2C_ADDR 0x70
#define FT5446_MAX_FINGER_NUM 1

// FT5446 Register Addresses
#define FT5446_TD_STATUS_REG 0x02      // Touch point status
#define FT5446_TOUCH1_XH_REG 0x03      // Touch point 1 X high byte
#define FT5446_TOUCH1_XL_REG 0x04      // Touch point 1 X low byte
#define FT5446_TOUCH1_YH_REG 0x05      // Touch point 1 Y high byte
#define FT5446_TOUCH1_YL_REG 0x06      // Touch point 1 Y low byte
#define FT5446_TOUCH1_WEIGHT_REG 0x07  // Touch point 1 weight
#define FT5446_TOUCH1_MISC_REG 0x08    // Touch point 1 misc

#define FT5446_ID_G_MODE_REG 0xA4          // Interrupt mode register
#define FT5446_ID_G_THGROUP_REG 0x80       // Touch threshold
#define FT5446_ID_G_PERIODACTIVE_REG 0x88  // Active period

// FT5446 Chip ID and Firmware registers
#define FT5446_CHIP_ID_REG 0xA3       // Chip ID register
#define FT5446_FIRMWARE_ID_REG 0xA6   // Firmware ID register
#define FT5446_RELEASE_CODE_REG 0xA1  // Release code register
#define FT5446_VENDOR_ID_REG 0xA8     // Vendor ID register
#define FT5446_FW_VER_REG 0xA6        // Firmware version register (alias)

// FT5446 Mode settings
#define FT5446_MODE_NORMAL 0x00   // Normal operation mode
#define FT5446_MODE_TEST 0x04     // Test mode
#define FT5446_MODE_FACTORY 0x40  // Factory mode

// FT5446 Interrupt settings
#define FT5446_INT_MODE_POLLING 0x00  // Polling mode
#define FT5446_INT_MODE_TRIGGER 0x01  // Trigger mode
#define FT5446_INT_MODE_MASK 0x03     // Interrupt mode mask

// FT5446 Touch event status
#define FT5446_EVENT_PRESS_DOWN 0x00  // Press down event
#define FT5446_EVENT_LIFT_UP 0x01     // Lift up event
#define FT5446_EVENT_CONTACT 0x02     // Contact event
#define FT5446_EVENT_NO_EVENT 0x03    // No event

// FT5446 Data ready flag
#define FT5446_DATA_READY 0x80         // Data ready flag in TD_STATUS_REG
#define FT5446_TOUCH_POINTS_MASK 0x0F  // Touch points number mask

// Additional register addresses (reference edt-ft5x06)
#define FT5446_DEVICE_MODE_REG 0x00    // Device mode register
#define FT5446_GESTURE_ID_REG 0x01     // Gesture ID register
#define FT5446_TOUCH_STATUS_REG 0x02   // Touch status register (same as TD_STATUS)
#define FT5446_TOUCH1_XH_REG 0x03      // Touch 1 X high byte
#define FT5446_TOUCH1_XL_REG 0x04      // Touch 1 X low byte
#define FT5446_TOUCH1_YH_REG 0x05      // Touch 1 Y high byte
#define FT5446_TOUCH1_YL_REG 0x06      // Touch 1 Y low byte
#define FT5446_TOUCH1_WEIGHT_REG 0x07  // Touch 1 weight
#define FT5446_TOUCH1_MISC_REG 0x08    // Touch 1 misc

// Configuration registers
#define FT5446_THRESHOLD_REG 0x80       // Touch threshold
#define FT5446_FILTER_COEF_REG 0x81     // Filter coefficient
#define FT5446_CTRL_REG 0x86            // Control register
#define FT5446_TIME_ENTER_MONITOR 0x87  // Time to enter monitor mode
#define FT5446_ACTIVE_MODE_RATE 0x88    // Active mode report rate
#define FT5446_MONITOR_MODE_RATE 0x89   // Monitor mode report rate

// Power mode settings
#define FT5446_POWER_MODE_REG 0xA5      // Power mode register
#define FT5446_POWER_MODE_ACTIVE 0x00   // Active mode
#define FT5446_POWER_MODE_MONITOR 0x01  // Monitor mode
#define FT5446_POWER_MODE_STANDBY 0x02  // Standby mode

// Interrupt mode settings
#define FT5446_INT_MODE_REG 0xA6      // Interrupt mode register
#define FT5446_INT_MODE_POLLING 0x00  // Polling mode
#define FT5446_INT_MODE_TRIGGER 0x01  // Trigger mode

#define FT_TP1_REG 0X03  // 第一个触摸点数据地址
#define FT_TP2_REG 0X09  // 第二个触摸点数据地址
#define FT_TP3_REG 0X0F  // 第三个触摸点数据地址
#define FT_TP4_REG 0X15  // 第四个触摸点数据地址
#define FT_TP5_REG 0X1B  // 第五个触摸点数据地址

// FT5446 register definitions
#define FT5446_FW_VER_REG 0xA6           // Firmware version register
#define FT5446_TD_STATUS_REG 0x02        // Touch points status register
#define FT5446_TOUCH_THRESHOLD_REG 0x80  // Touch threshold register
#define FT5446_CHIP_ID_REG 0xA3          // Chip ID register

#endif /* _FT5446_H_ */
