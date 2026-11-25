/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PART'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNER'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PART'S TECHNOLOGY.


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
/*
********************************************************************************************************************
*                                              usb host driver
*
*                              (c) Copyright 2007-2010, javen.China
*                                       All Rights Reserved
*
* File Name     : error.h
*
* Author        : javen
*
* Version       : 2.0
*
* Date          : 2010.03.02
*
* Description   : 函数返回值说明表
*
* History       :
*
********************************************************************************************************************
*/
#ifndef __USB_ERROR_H__
#define __USB_ERROR_H__

#define USB_ERR_SUCCESS			        0	/* 成功 */
#define USB_ERR_UNKOWN_ERROR		    -1	/* 未知错误 */

/* 软件控制相关 */
#define USB_ERR_BAD_ARGUMENTS		    1	/* 参数错误 */
#define USB_ERR_DATA_OVERFLOW		    2	/* 数据溢出 */

/* 硬件操作相关 */
#define USB_ERR_IO_DEVICE_OFFLINE	    500	/* 设备不在线 */
#define USB_ERR_IO_DEVICE_DIEAD		    501	/* 设备不可用 */
#define USB_ERR_IO_DEVICE_BUSY		    502	/* 设备不可用 */
#define USB_ERR_COMMAND_NEED_RETRY	    503	/* 命令需要重发 */
#define USB_ERR_COMMAND_SEND_FAILED	    504	/* 命令发送失败 */
#define USB_ERR_COMMAND_EXECUTE_FAILED	505	/* 命令执行失败 */
#define USB_ERR_RESET_POERT_FAILED	    506	/* reset端口失败 */
#define USB_ERR_UNKOWN_DEVICE		    507	/* 未知设备 */
#define USB_ERR_DEVICE_PROBE_FAILED	    508	/* 设备初始化失败 */
#define USB_ERR_DEVICE_REMOVE_FAILED	509	/* 设备移除失败 */

#define USB_ERR_MEDIA_NOT_PRESENT	    510	/* 介质没有准备好 */
#define USB_ERR_NOT_SUPPORT_COMMAND	    511	/* 不支持的命令 */


/* 操作系统相关 */
#define USB_ERR_CREATE_SIME_FAILED	    1000	/* 信号量操作失败 */
#define USB_ERR_MALLOC_FAILED		    1001	/* 内存分配失败 */
#define USB_ERR_CREATE_THREAD_FAILED	1002	/* 创建线程失败 */
#define USB_ERR_REG_BLK_DEV_FAILED	    1003	/* 注册块设备失败 */
#define USB_ERR_CREATE_TIMER_FAILED	    1004	/* 创建timer失败 */

/* USB */
#define USB_ERR_ALLOC_URB_FAILED	    2000	/* 分配URB失败 */

#endif
