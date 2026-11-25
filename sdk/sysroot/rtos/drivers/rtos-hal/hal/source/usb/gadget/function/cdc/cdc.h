#ifndef  __CDC_H__
#define  __CDC_H__

#define USB_CDC_BUFSIZE                 0x40

#define USB_CDC_CLASS_COMM              0x02
#define USB_CDC_CLASS_DATA              0x0A

#define USB_CDC_SUBCLASS_NONE           0x00
#define USB_CDC_SUBCLASS_DLCM           0x01
#define USB_CDC_SUBCLASS_ACM            0x02
#define USB_CDC_SUBCLASS_TCM            0x03
#define USB_CDC_SUBCLASS_MCCM           0x04
#define USB_CDC_SUBCLASS_CCM            0x05
#define USB_CDC_SUBCLASS_ETH            0x06
#define USB_CDC_SUBCLASS_ATM            0x07
#define USB_CDC_SUBCLASS_EEM            0x0C

#define USB_CDC_PROTOCOL_NONE           0x00
#define USB_CDC_PROTOCOL_V25TER         0x01
#define USB_CDC_PROTOCOL_I430           0x30
#define USB_CDC_PROTOCOL_HDLC           0x31
#define USB_CDC_PROTOCOL_TRANS          0x32
#define USB_CDC_PROTOCOL_Q921M          0x50
#define USB_CDC_PROTOCOL_Q921           0x51
#define USB_CDC_PROTOCOL_Q921TM         0x52
#define USB_CDC_PROTOCOL_V42BIS         0x90
#define USB_CDC_PROTOCOL_Q931           0x91
#define USB_CDC_PROTOCOL_V120           0x92
#define USB_CDC_PROTOCOL_CAPI20         0x93
#define USB_CDC_PROTOCOL_HOST           0xFD
#define USB_CDC_PROTOCOL_PUFD           0xFE
#define USB_CDC_PROTOCOL_VENDOR         0xFF
#define USB_CDC_PROTOCOL_EEM            0x07

#define USB_CDC_CS_INTERFACE            0x24
#define USB_CDC_CS_ENDPOINT             0x25

#define USB_CDC_SCS_HEADER              0x00
#define USB_CDC_SCS_CALL_MGMT           0x01
#define USB_CDC_SCS_ACM                 0x02
#define USB_CDC_SCS_UNION               0x06
#define USB_CDC_SCS_ETH                 0x0F

#define CDC_SEND_ENCAPSULATED_COMMAND   0x00
#define CDC_GET_ENCAPSULATED_RESPONSE   0x01
#define CDC_SET_COMM_FEATURE            0x02
#define CDC_GET_COMM_FEATURE            0x03
#define CDC_CLEAR_COMM_FEATURE          0x04
#define CDC_SET_AUX_LINE_STATE          0x10
#define CDC_SET_HOOK_STATE              0x11
#define CDC_PULSE_SETUP                 0x12
#define CDC_SEND_PULSE                  0x13
#define CDC_SET_PULSE_TIME              0x14
#define CDC_RING_AUX_JACK               0x15
#define CDC_SET_LINE_CODING             0x20
#define CDC_GET_LINE_CODING             0x21
#define CDC_SET_CONTROL_LINE_STATE      0x22
#define CDC_SEND_BREAK                  0x23
#define CDC_SET_RINGER_PARMS            0x30
#define CDC_GET_RINGER_PARMS            0x31
#define CDC_SET_OPERATION_PARMS         0x32
#define CDC_GET_OPERATION_PARMS         0x33
#define CDC_SET_LINE_PARMS              0x34
#define CDC_GET_LINE_PARMS              0x35
#define CDC_DIAL_DIGITS                 0x36
#define CDC_SET_UNIT_PARAMETER          0x37
#define CDC_GET_UNIT_PARAMETER          0x38
#define CDC_CLEAR_UNIT_PARAMETER        0x39
#define CDC_GET_PROFILE                 0x3A
#define CDC_SET_ETH_MULTICAST_FILTERS   0x40
#define CDC_SET_ETH_POWER_MGMT_FILT     0x41
#define CDC_GET_ETH_POWER_MGMT_FILT     0x42
#define CDC_SET_ETH_PACKET_FILTER       0x43
#define CDC_GET_ETH_STATISTIC           0x44
#define CDC_SET_ATM_DATA_FORMAT         0x50
#define CDC_GET_ATM_DEVICE_STATISTICS   0x51
#define CDC_SET_ATM_DEFAULT_VC          0x52
#define CDC_GET_ATM_VC_STATISTICS       0x53

struct cdc_IAD_Descriptor {
    uint8_t  bLength ;//              0x08        //��������С
    uint8_t  bDescriptorType;//          0x0B        //IAD����������
    uint8_t  bFirstInterface;//        0x00        //��ʼ�ӿ�
    uint8_t  bInterfaceCount;//        0x02        //�ӿ���
    uint8_t  bFunctionClass;//        0x0E        //���ʹ���
    uint8_t  bFunctionSubClass;//        0x03        //�����ʹ���
    uint8_t  bFunctionProtocol;//      0x00        //Э�����
    uint8_t  iFunction;//        0x04        //�����ַ�������
}__attribute__ ((packed));

struct cdc_header_functional_descriptor {
	uint8_t  bFunctionLength;
	uint8_t  bDescriptorType;
	uint8_t  bDescriptorSubType;
	uint16_t  bcdCDC;
}__attribute__ ((packed));

struct cdc_ctrl_management_descriptor {
	uint8_t  bFunctionLength;
	uint8_t  bDescriptorType;
	uint8_t  bDescriptorSubType;
	uint8_t  bmCapabilities;
}__attribute__ ((packed));

struct cdc_union_functional_descriptor {
	uint8_t  bFunctionLength;
	uint8_t  bDescriptorType;
	uint8_t  bDescriptorSubType;
	uint8_t  bControlInterface;
	uint8_t  bSubordinateInterface[1];
}__attribute__ ((packed));

struct cdc_call_functional_descriptor {
	uint8_t  bFunctionLength;
	uint8_t  bDescriptorType;
	uint8_t  bDescriptorSubType;
	uint8_t  bmCapabilities;
	uint8_t  bDataInterface;
}__attribute__ ((packed));

struct cdc_line_coding_descriptor {
	uint32_t  dwDTERate;
	uint8_t  bCharFormat;
	uint8_t  bParityType;
	uint8_t  bDataBits;
}__attribute__ ((packed));

#define USB_DT_DEVICE_QUALIFIER_SIZE              10
struct USB_DeviceQualifierDescriptor {
    uint8_t  bLength;
    uint8_t bDescriptorType;// = USB_DT_DEVICE_QUALIFIER,
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint8_t  bNumConfigurations;
    uint8_t  Reserved;
}__attribute__ ((packed));
    
#endif