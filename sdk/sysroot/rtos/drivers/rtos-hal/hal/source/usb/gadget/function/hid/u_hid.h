#ifndef __U_HID_H__
#define __U_HID_H__

struct u_hid_msg{
    unsigned int message;
    unsigned int param1;
    unsigned int param2;
    unsigned int param3;
    unsigned int param4;
};

typedef enum {
    U_HID_CONTROL_MSG_UNKNOWEN = 0,

    /* param1=interface_num, param2=alternate-setting */
    U_HID_CONTROL_MSG_SET_INTERFACE = 50,
    /* no param */
    U_HID_CONTROL_MSG_DEV_RESET,
    /* no param */
    U_HID_CONTROL_MSG_DEV_RESUME,
    /* no param */
    U_HID_CONTROL_MSG_DEV_SUSPEND,
    /* output report update */
    U_HID_CONTROL_MSG_REPORT_OUTPUT_UPDATE,

    /* no param */
    U_HID_CONTROL_DEVICE_DEREGISTER,
} u_hid_control_msg;

typedef enum {
    IOCTL_CMD_HID_GET_MESSAGE,
} u_hid_ioctl_cmd;

#endif