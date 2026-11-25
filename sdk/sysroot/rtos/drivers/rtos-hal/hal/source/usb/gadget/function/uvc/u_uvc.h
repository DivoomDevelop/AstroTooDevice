#ifndef __U_UVC_H__
#define __U_UVC_H__

#define UVC_FUNCTION_SINGLE_IN

struct u_uvc_msg{
    unsigned int message;
    unsigned int param1;
    unsigned int param2;
    unsigned int param3;
    unsigned int param4;
};

typedef enum {
    U_UVC_CONTROL_MSG_UNKNOWEN = 0,

    /* param1=format, param2=framerate, param3=frame-width, param4=frame-height */
    U_UVC_CONTROL_MSG_PROBE_UPDATE,
    /* param1=format, param2=framerate, param3=frame-width, param4=frame-height */
    U_UVC_CONTROL_MSG_COMMIT_UPDATE,

    /* param1=interface_num, param2=alternate-setting */
    U_UVC_CONTROL_MSG_SET_INTERFACE = 50,
    /* no param */
    U_UVC_CONTROL_MSG_DEV_RESET,
    /* no param */
    U_UVC_CONTROL_MSG_DEV_RESUME,
    /* no param */
    U_UVC_CONTROL_MSG_DEV_SUSPEND,

    /* no param */
    U_UVC_CONTROL_STREAMING_STARTUP = 100,
    /* no param */
    U_UVC_CONTROL_STREAMING_STOP,

    /* no param */
    U_UVC_CONTROL_DEVICE_DEREGISTER,
} u_uvc_control_msg;

#define UVC_FORMAT_MJPEG    (100)
#define UVC_FORMAT_H265     (101)
typedef enum {
    U_UVC_STREAMING_FORMAT_MJPEG    = UVC_FORMAT_MJPEG,
    U_UVC_STREAMING_FORMAT_H265     = UVC_FORMAT_H265,
} u_uvc_streaming_format;

typedef enum {
    IOCTL_CMD_UVC_GET_MESSAGE,
    IOCTL_CMD_UVC_SET_MAX_FRAMESIZE,
    IOCTL_CMD_UVC_CHANNEL_ENABLE,
    IOCTL_CMD_UVC_GET_CURRENT_STREAMING_FORMAT,
} u_uvc_ioctl_cmd;

#endif