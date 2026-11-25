#ifndef __UVC_DRV_H__
#define __UVC_DRV_H__

#include <typedef.h>
#include <stdbool.h>
#include "aw_list.h"
#include "video.h"
#include "gadget.h"
#include "rtthread.h"
#include "rtdef.h"
#include "ch9.h"

/**
 *  
 */
struct usb_video {
    struct list_head                            list_entry;
    unsigned int                                id;
    unsigned int                                attr;
/* video streaming data trans-direction */
#define USB_VIDEO_DIRECTION                     (0x0)
#define usb_video_direction_set_in(usb_video) \
    ((usb_video)->attr |= (0x1 << USB_VIDEO_DIRECTION))
#define usb_video_direction_set_out(usb_video) \
    ((usb_video)->attr &= ~(0x1 << USB_VIDEO_DIRECTION))
#define usb_video_direction_is_in(usb_video) \
    (!!((usb_video)->attr & (0x1 << USB_VIDEO_DIRECTION)))
#define usb_video_direction_is_out(usb_video) \
    (!(usb_video_direction_is_in(usb_video)))
/* endpoint enable */
#define USB_VIDEO_ENDPOINT_ENABLE               (0x1)
#define usb_video_endpoint_is_enable(usb_video) \
    (((usb_video)->attr & (0x1 << USB_VIDEO_ENDPOINT_ENABLE))? true : false)
#define usb_video_endpoint_set_status(usb_video, status) \
    do{\
        if(status){\
            (usb_video)->attr |= (0x1 << USB_VIDEO_ENDPOINT_ENABLE);\
        }else{\
            (usb_video)->attr &= ~(0x1 << USB_VIDEO_ENDPOINT_ENABLE);\
        }\
    }while(0)
/* channel start */
#define USB_VIDEO_STARTUP                       (0x2)
#define usb_video_check_startup_status(usb_video) \
    (((usb_video)->attr & (0x1 << USB_VIDEO_STARTUP))? true : false)
#define usb_video_update_startup_flags(usb_video, status) \
    do{\
        if(status){\
            (usb_video)->attr |= (0x1 << USB_VIDEO_STARTUP);\
        }else{\
            (usb_video)->attr &= ~(0x1 << USB_VIDEO_STARTUP);\
        }\
    }while(0)

    int (*put)(struct usb_video *);
    void (*ctrl_notify)(struct usb_video *video, \
        unsigned char message, unsigned long data);
#define UVC_CONTROL_MESSAGE_UNKNOWEN        (0)
#define UVC_CONTROL_MESSAGE_PROBE_UPDATE    (1)
#define UVC_CONTROL_MESSAGE_COMMIT_UPDATE   (2)
#define UVC_CONTROL_MESSAGE_SET_INTERFACE   (50)
#define UVC_CONTROL_MESSAGE_RESET           (100)
#define UVC_CONTROL_MESSAGE_RESUME          (101)
#define UVC_CONTROL_MESSAGE_SUSPEND         (102)

    unsigned char                               vc_intfnum;
    unsigned char                               vs_intfnum;
    struct usb_endpoint_descriptor              *vc_epdesc;
    struct usb_endpoint_descriptor              *vs_epdesc;
    /* probe and commit */
    struct uvc_streaming_control                probe_and_commit;
    unsigned char                               *current_format_desc;
    unsigned char                               *current_frame_desc;
    unsigned int                                current_width;
    unsigned int                                current_height;
    unsigned char                               current_framerate;
    unsigned char                               current_format;

    unsigned char                               fid;
#define usb_video_get_current_fid(usb_video) \
    ((usb_video)->fid++)

    /* string descriptor */
    unsigned short                              *iad_string;
    unsigned short                              *vcintf_string;
    unsigned short                              *vsintf_string;

    void                                        *device;
    void                                        *user_data;
};

/**
 *  
 */
struct uvc_in {
    struct usb_video                            base;

    /**
     * vc
     */
	struct uvc_camera_terminal_descriptor		uvc_camera_terminal;
	struct uvc_processing_unit_descriptor		uvc_processing;
	struct uvc_output_terminal_descriptor		uvc_output_terminal;
#define uvc_in_get_vc_cs_intf_length(uvc_in, hsize) (\
    (hsize) + \
    (uvc_in)->uvc_camera_terminal.bLength + \
    (uvc_in)->uvc_processing.bLength + \
    (uvc_in)->uvc_output_terminal.bLength)

    /* vc endpoint */
    unsigned char                               vc_endpoint_num;
    unsigned short                              vc_endpoint_maxpacket;
    /* vs endpoint */
    unsigned char                               vs_endpoint_num;
    unsigned short                              vs_endpoint_maxpacket;
    /**
     * vs support
     */                      
    unsigned char                               vs_support_format_num;
    unsigned int                                vs_support_configuration_length;
    unsigned char                               *vs_support_configuration;

};

#define uvc_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define usb_video_get_uvc_in_structure(usb_video) \
    uvc_container_of(usb_video, struct uvc_in, base)

void uvc_descriptor_header_display(struct uvc_descriptor_header *header);
/**/
int uvc_drv_init(void);
int uvc_drv_deinit(void);
/**/
int uvc_function_in_channel_free(struct uvc_in *cin);
struct uvc_in *uvc_function_in_channel_alloc(void);
/**/
int uvc_drv_channel_add(struct usb_video *chn);
int uvc_drv_channel_detach(struct usb_video *chn);
/**/
int uvc_drv_function_driver_add(struct usb_function_driver *fd);
/**/
int uvc_drv_wait_startup(void);
int uvc_drv_startup(struct usb_video *video);
int uvc_drv_stop(void);
/**/
int uvc_streaming_pipe_write(struct usb_video *video,
    void *buf, int length);
/**/
int uvc_frame_encode(struct usb_video *video,
    unsigned char *dst, unsigned int dst_len,
    unsigned char *src, unsigned int src_len);
#endif