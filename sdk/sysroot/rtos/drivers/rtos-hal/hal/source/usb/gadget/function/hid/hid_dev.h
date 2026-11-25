#ifndef __HID_DEV_H__
#define __HID_DEV_H__

#include "rtthread.h"
#include "hal_queue.h"
#include "f_hid.h"

struct hid_device {
    rt_device_t os_dev;
    unsigned char name[32];

    struct f_hid *f;
    unsigned int flags;
/* device open */
#define HID_DEVICE_OPEN                         (0)
#define hid_device_check_is_open(hid_device) \
    (!!((hid_device)->flags & (0x1 << HID_DEVICE_OPEN)))
#define hid_device_set_open_status(hid_device) \
    ((hid_device)->flags |= (0x1 << HID_DEVICE_OPEN))
#define hid_device_set_close_status(hid_device) \
    ((hid_device)->flags &= ~(0x1 << HID_DEVICE_OPEN))

    hal_queue_t msg_queue;
    void *thread;
};

int hid_device_register(struct f_hid *f, unsigned int device_id);
int hid_device_unregister(struct f_hid *f);

#endif