#ifndef __UVC_DEV_H__
#define __UVC_DEV_H__

#include "rtthread.h"
#include "hal_queue.h"
#include "uvc_drv.h"

/**
 * 
 */
struct uvc_device {
    rt_device_t os_dev;
    unsigned char name[32];
    unsigned char id;

    struct usb_video *video;
    unsigned int flags;
/* streaming connect */
#define UVC_DEVICE_STREAMING_CHN_CONNECT_STATUS (0)
#define uvc_device_check_streaming_chn_is_connect(uvc_device) \
    (!!((uvc_device)->flags & (0x1 << UVC_DEVICE_STREAMING_CHN_CONNECT_STATUS)))
#define uvc_device_set_streaming_chn_connect_status(uvc_device) \
    ((uvc_device)->flags |= (0x1 << UVC_DEVICE_STREAMING_CHN_CONNECT_STATUS))
#define uvc_device_set_streaming_chn_disconnect_status(uvc_device) \
    ((uvc_device)->flags &= ~(0x1 << UVC_DEVICE_STREAMING_CHN_CONNECT_STATUS))
/* device open */
#define UVC_DEVICE_OPEN                         (1)
#define uvc_device_check_is_open(uvc_device) \
    (!!((uvc_device)->flags & (0x1 << UVC_DEVICE_OPEN)))
#define uvc_device_set_open_status(uvc_device) \
    ((uvc_device)->flags |= (0x1 << UVC_DEVICE_OPEN))
#define uvc_device_set_close_status(uvc_device) \
    ((uvc_device)->flags &= ~(0x1 << UVC_DEVICE_OPEN))

    hal_queue_t msg_queue;

    unsigned int max_framesize;
    unsigned int tx_bufsize;
    unsigned char *tx_buffer;
};

int uvc_device_register(struct usb_video *video, unsigned int device_id);
int uvc_device_unregister(struct usb_video *video);
#endif