#ifndef __F_HID_H__
#define __F_HID_H__

#include <typedef.h>
#include <stdbool.h>
#include "aw_list.h"
#include "gadget.h"
#include "rtthread.h"
#include "rtdef.h"
#include "ch9.h"

#define HID_FUNCTION_IO_BUFFER

struct f_hid {
    struct list_head                            list_entry;
    int                                         interface_num;
    int                                         id;
    unsigned int                                attr;
/* endpoint enable */
#define USB_HID_INTRIN_ENDPOINT_ENABLE          (0x1)
#define usb_hid_intrin_endpoint_is_enable(f) \
    (((f)->attr & (0x1 << USB_HID_INTRIN_ENDPOINT_ENABLE))? true : false)
#define usb_hid_intrin_endpoint_set_status(f, status) do{\
        if(status)  (f)->attr |= (0x1 << USB_HID_INTRIN_ENDPOINT_ENABLE);\
        else        (f)->attr &= ~(0x1 << USB_HID_INTRIN_ENDPOINT_ENABLE);\
    }while(0)
#define USB_HID_INTROUT_ENDPOINT_ENABLE         (0x2)
#define usb_hid_introut_endpoint_is_enable(f) \
    (((f)->attr & (0x1 << USB_HID_INTROUT_ENDPOINT_ENABLE))? true : false)
#define usb_hid_introut_endpoint_set_status(f, status) do{\
        if(status)  (f)->attr |= (0x1 << USB_HID_INTROUT_ENDPOINT_ENABLE);\
        else        (f)->attr &= ~(0x1 << USB_HID_INTROUT_ENDPOINT_ENABLE);\
    }while(0)
/* func start */
#define USB_HID_FUNC_STARTUP                    (0x0)
#define usb_hid_func_check_startup_status(f) \
    (((f)->attr & (0x1 << USB_HID_FUNC_STARTUP))? true : false)
#define usb_hid_func_update_startup_flags(f, status) do{\
        if(status)  (f)->attr |= (0x1 << USB_HID_FUNC_STARTUP);\
        else        (f)->attr &= ~(0x1 << USB_HID_FUNC_STARTUP);\
    }while(0)

    /* descriptor */
    /* hid descriptor */
    unsigned char                               *hid_descriptor;
    unsigned int                                hid_desclen;
    /* endpoint descriptor */
    struct usb_endpoint_descriptor              *intr_inep;
    struct usb_endpoint_descriptor              *intr_outep;
    /* report descriptor */
    unsigned char                               *report_descriptor;
    unsigned int                                report_desclen;
    /* phyiscal descriptor */
    struct hid_physical_descriptor_zero         *phydesc_zero;
    unsigned char                               *phydesc_array;
    /* string descriptor */
    unsigned short                              *iad_string;
    unsigned short                              *intf_string;

    /* report */
    unsigned char                               report_outupdate;
    unsigned char *                             report_outbuf;
    unsigned int                                report_outlen;

    unsigned char                               report_inrate;
    unsigned char                               report_inupdate;
    unsigned char *                             report_inbuf;
    unsigned int                                report_inlen;

    unsigned char                               report_featupdate;
    unsigned char *                             report_featbuf;
    unsigned int                                report_featlen;

    unsigned char                               current_protocol;

    void                                        *device;
    void                                        *user_data;

    /* intr-in */
    int (*report_input_send)(struct f_hid *f, void *data, unsigned int len);
    /* intr-out */
    int (*report_output_infifo)(struct f_hid *f, void *output_report);
    int (*report_output_outfifo)(struct f_hid *f, void *output_report);

    int (*put)(struct f_hid *);
    void (*ctrl_notify)(struct f_hid *hid_func, \
        unsigned char message, unsigned long data);
#define HID_CONTROL_MESSAGE_UNKNOWEN                (0)
#define HID_CONTROL_MESSAGE_RESET                   (1)
#define HID_CONTROL_MESSAGE_RESUME                  (2)
#define HID_CONTROL_MESSAGE_SUSPEND                 (3)
#define HID_CONTROL_MESSAGE_SET_INTERFACE           (4)
#define HID_CONTROL_MESSAGE_REPORT_OUTPUT_UPDATE    (5)
#define HID_CONTROL_MESSAGE_CHANGE_INTRIN_RATE      (6)
#define HID_CONTROL_MESSAGE_CHANGE_REPORT_PROTOCOL  (7)
};

int hid_driver_stop(void);
int hid_driver_startup(void);

int hid_report_intrpipe_read(struct f_hid *func, void *buf, int length);
int hid_report_intrpipe_write(struct f_hid *func, void *buf, int length);

int hid_function_detach(struct f_hid *func);
int hid_function_add(struct f_hid *func);

struct f_hid *hid_function_alloc(void);
int hid_function_free(struct f_hid *func);

int hid_drv_deinit(void);
int hid_drv_init(void);

int hid_drv_function_driver_add(struct usb_function_driver *fd);

#endif