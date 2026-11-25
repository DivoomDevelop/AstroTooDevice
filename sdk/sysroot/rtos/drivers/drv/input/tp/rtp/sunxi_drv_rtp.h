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
#ifndef SUNXI_DRV_RTP_H
#define SUNXI_DRV_RTP_H

#include <sunxi_hal_tpadc.h>
#include <sunxi-input.h>

#define RTP_DRIVER_NAME "touchscreen"

/* Touch panel message types */
#define EV_TP_ACTION_NULL     0
#define EV_TP_PRESS_START     1
#define EV_TP_PRESS_HOLD      2
#define EV_TP_PRESS_MOVE      3

#ifdef CONFIG_DRIVERS_TEST_TOUCHSCREEN
#define RTP_LOG(fmt, ...) printf("[RTP]    " fmt, ##__VA_ARGS__)
#endif

/* Touch panel message structure */
typedef struct __EV_TP_MSG {
    __u8 msg_type;    /* Message type */
    __s16 xpoint;      /* X coordinate */
    __s16 ypoint;      /* Y coordinate */
    __s16 speed_dir;   /* Direction of movement */
    __s16 speed_val;   /* Speed value */
} __ev_tp_msg_t;

/* Touch panel position structure */
typedef struct __EV_TP_POS {
    __u16 disp_xpos;   /* Display X coordinate */
    __u16 disp_ypos;   /* Display Y coordinate */
} __ev_tp_pos_t;

/////////触摸屏动作////////
#define TCHSCR_ACTION_NULL				0	    //空白动作或无法识别的动作
#define TCHSCR_ACTION_CLICK				1	    //触摸屏单击
#define TCHSCR_ACTION_DOWN				(0x02)	//触摸屏下滑
#define TCHSCR_ACTION_UP				(0x12)	//触摸屏上滑
#define TCHSCR_ACTION_LEFT				(0x03)	//触摸屏左滑
#define TCHSCR_ACTION_RIGHT				(0x13)	//触摸屏右滑
#define TCHSCR_ACTION_ANTICLOCKWISE		(0x04)	//触摸屏逆时针滑动
#define TCHSCR_ACTION_CLOCKWISE			(0x14)	//触摸屏顺时针滑动
#define TCHSCR_ACTION_LD2RU				(0x05)	//触摸屏斜线滑动,左下到右上
#define TCHSCR_ACTION_RU2LD      		(0x15)	//触摸屏斜线滑动,右上到左下
#define TCHSCR_ACTION_LU2RD	    		(0x06)	//触摸屏反斜线滑动,左上到右下
#define TCHSCR_ACTION_RD2LU 			(0x16)	//触摸屏反斜线滑动,右下到左上

int sunxi_rtp_init(void);
int drv_tpadc_register_callback(tpadc_usercallback_t callback);

void tp_send_msg(__ev_tp_msg_t msg);
#endif
