/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 *
 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the people's Republic of China and other countries.
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
#ifndef __USB_USB_H__
#define __USB_USB_H__

/*
 * urb->transfer_flags:
 *
 * Note: URB_DIR_IN/OUT is automatically set in usb_submit_urb().
 */
#define URB_SHORT_NOT_OK	        0x0001	    /* report short reads as errors */
#define URB_ISO_ASAP		        0x0002	    /* iso-only; use the first unexpired
					                             * slot in the schedule */
#define URB_NO_TRANSFER_DMA_MAP	    0x0004	    /* urb->transfer_dma valid on submit */
#define URB_NO_SETUP_DMA_MAP	    0x0008      /* urb->setup_dma valid on submit */
#define URB_ASYNC_UNLINK	        0x0010      /* usb_unlink_urb() returns asap */
#define URB_NO_FSBR		            0x0020      /* UHCI-specific */
#define URB_ZERO_PACKET		        0x0040	    /* Finish bulk OUT with short packet */
#define URB_NO_INTERRUPT	        0x0080	    /* HINT: no non-error interrupt
					                             * needed */
#define URB_FREE_BUFFER		        0x0100	    /* Free transfer buffer with the URB */

/* The following flags are used internally by usbcore and HCDs */
#define URB_DIR_IN		            0x0200	    /* Transfer from device to host */
#define URB_DIR_OUT		            0
#define URB_DIR_MASK		        URB_DIR_IN

#define URB_DMA_MAP_SINGLE	        0x00010000	/* Non-scatter-gather mapping */
#define URB_DMA_MAP_PAGE	        0x00020000	/* HCD-unsupported S-G */
#define URB_DMA_MAP_SG		        0x00040000	/* HCD-supported S-G */
#define URB_MAP_LOCAL		        0x00080000	/* HCD-local-memory mapping */
#define URB_SETUP_MAP_SINGLE	    0x00100000	/* Setup packet DMA mapped */
#define URB_SETUP_MAP_LOCAL	        0x00200000	/* HCD-local setup packet */
#define URB_DMA_SG_COMBINED	        0x00400000	/* S-G entries were combined */
#define URB_ALIGNED_TEMP_BUFFER	    0x00800000	/* Temp buffer was alloc'd */

#endif