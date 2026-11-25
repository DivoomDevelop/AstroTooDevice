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
#ifndef _SUNXI_CTP_DEV_H_
#define _SUNXI_CTP_DEV_H_
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <hal_cfg.h>
#include <hal_gpio.h>
#include <script.h>
#include <sunxi-input.h>
#include <hal_osal.h>
#include <sunxi_hal_twi.h>
#include <typedef.h>

#ifdef __cplusplus
extern "C" {
#endif

// #define CTP_DEBUG

#define __log CTP_LOG
#define __wrn CTP_LOG
#define __msg CTP_LOG
#define __err CTP_ERR

#ifdef  CTP_DEBUG
#define CTP_LOG(fmt, args...) printf("[CTP]" fmt, ##args)
#else
#define CTP_LOG(fmt, args...)
#endif
#define CTP_ERR(fmt, args...) printf("[CTP ERR]" fmt, ##args)

/* 错误码定义 */
#define ENOMEM 12
#define EINVAL 22
#define EEXIST 17

#define MAX_FINGER_NUM 5
#define CTP_NAME_LEN 32

#define CTP_PARA "ctp_para"
#define CTP_INDEV_NAME "touchscreen"
#define CTP_REGLEN_1     1
#define CTP_REGLEN_2     2
#define CTP_REGLEN_3     3

// Event types
#define CTP_EVENT_PRESS_DOWN 0x01
#define CTP_EVENT_MOVE 0x02
#define CTP_EVENT_RELEASE 0x04
#define CTP_EVENT_NONE 0x00

// Touch data
struct sunxi_ctp_touch_data_t {
    __u8 status;
    __s16 x;
    __s16 y;
    __u8 pressure;
    __u8 id;
};

typedef struct sunxi_ctp_hw {
    __u8 twi_port;
    __u16 i2c_addr;
    __s32 reset_gpio;
    __s32 interrupt_gpio;
    __u32 irq_num;
} sunxi_ctp_hw_t;

// Driver interface
struct sunxi_ctp_driver {
    const char *name;
    struct sunxi_ctp_driver *next;

    __s32 (*init)(void);
    __s32 (*deinit)(void);
    __s32 (*reset)(void);
    void (*bind)(sunxi_ctp_hw_t *dev);
    void (*unbind)(void);

    void (*read_touch_data)(struct sunxi_ctp_touch_data_t *data);
    __s32 (*read_multi_touch)(struct sunxi_ctp_touch_data_t *data, __u8 max_points);
    __s32 (*clear_interrupt)(void);
    __s32 (*check_data_ready)(void);

    __s32 (*set_power_mode)(__u8 mode);
    __s32 (*get_chip_info)(__u8 *info, __u32 len);
    __s32 (*update_firmware)(const __u8 *fw_data, __u32 fw_size);

    __s32 (*start_calibration)(void);
    __s32 (*set_calibration_data)(const __u8 *data, __u32 len);
    __s32 (*get_calibration_data)(__u8 *data, __u32 len);

    __s32 (*set_gesture_mode)(__u8 enable);
    __s32 (*read_gesture_data)(__u8 *gesture);
    __s32 (*set_report_rate)(__u8 rate);
};

typedef struct sunxi_ctp {
    struct sunxi_input_dev *input_dev;
    char name[CTP_NAME_LEN];
    char drv_name[CTP_NAME_LEN];

    sunxi_ctp_hw_t *ctp_hw_info;
    __u8 mapping_en;
    __s32 screen_max_x;
    __s32 screen_max_y;
    bool swap_xy;
    uint8_t reverse_x;
    uint8_t reverse_y;
    uint8_t screen_direction;

    void *ctp_thread;
    hal_sem_t interrupt_sem;

    void (*reset_cb)(void);
    void (*irq_cb)(void);

    struct sunxi_ctp_driver *ctp_driver;
} sunxi_ctp_t;

// Device driver registration and matching
int sunxi_ctp_driver_register(struct sunxi_ctp_driver *driver);
int sunxi_ctp_driver_unregister(struct sunxi_ctp_driver *driver);
struct sunxi_ctp_driver *sunxi_ctp_driver_find_by_name(const char *name);

// I2C communication functions
int iic_read_bytes_end_cmd(__u8 port, __u16 addr, uint32_t reg, __s32 reg_len, uint8_t *data,
                           uint32_t len);
int iic_write_bytes_end_cmd(__u8 port, __u16 addr, uint32_t reg, __s32 reg_len, const uint8_t *data,
                            uint32_t len);
__s32 sunxi_ctp_regmap_read(sunxi_ctp_hw_t *ctp_hw, __s32 reg_len, __u32 reg, __u8 *data);
__s32 sunxi_ctp_regmap_write(sunxi_ctp_hw_t *ctp_hw, __s32 reg_len, __u32 reg, __u8 data);
int sunxi_ctp_driver_register(struct sunxi_ctp_driver *driver);
int sunxi_ctp_driver_unregister(struct sunxi_ctp_driver *driver);

#ifdef __cplusplus
}
#endif

#endif /* _SUNXI_CTP_DEV_H_ */