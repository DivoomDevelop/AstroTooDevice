/**
 * hid1_11 defined
 */

#ifndef __HID_H__
#define __HID_H__

#include "aw_common.h"

/**
 * generic item format
 */
#define __item_prefix_btag(bTag) \
	((unsigned char)(((bTag) & 0xf) << 4))
#define __item_prefix_btype(bType) \
	((unsigned char)(((bType) & 0x3) << 2))
#define __item_prefix_bsize(bSize) \
	((unsigned char)((bSize) & 0x3))
#define __item_prefix(bTag, bType, bSize) \
	((unsigned char)(__item_prefix_btag(bTag) | __item_prefix_btype(bType) | __item_prefix_bsize(bSize)))

/* bType */
#define ITEM_TYPE_MAIN		(0)
#define ITEM_TYPE_GLOBAL	(1)
#define ITEM_TYPE_LOCAL		(2)
/* short item bSize */
#define SHORT_BSIZE_0BYTES	(0)
#define SHORT_BSIZE_1BYTES	(1)
#define SHORT_BSIZE_2BYTES	(2)
#define SHORT_BSIZE_4BYTES	(3)

/* short item */
#define __perfix_short_item_main(bTag, bSize) \
	__item_prefix(bTag, ITEM_TYPE_MAIN, bSize)
#define __perfix_short_item_global(bTag, bSize) \
	__item_prefix(bTag, ITEM_TYPE_GLOBAL, bSize)
#define __perfix_short_item_local(bTag, bSize) \
	__item_prefix(bTag, ITEM_TYPE_LOCAL, bSize)

/* long item */
#define __perfix_long_item() \
	__item_prefix(2, 3, 0xf)
#define __declear_long_item(bLongItemTag, bDataSize) \
	{__perfix_long_item(), (bDataSize) & 0xff, (bLongItemTag) & 0xff}

/**
 * There are three item types: Main, Global, and Local.
 */
/**
 * There are five Main item tags currently defined:
 *		Input
 *		Output
 *		Feature
 *		Collection
 *		End Collection
 */
/* main tag */
#define TAG_MAIN_INPUT			(0x8)
#define TAG_MAIN_OUTPUT			(0x9)
#define TAG_MAIN_FEATURE		(0xb)
#define TAG_MAIN_COLLECTION		(0xa)
#define TAG_MAIN_END_COLLECTION	(0xc)

/* input */
#define DECLERA_ITEM_SHORT_MAIN_INPUT(size) \
	__perfix_short_item_main(TAG_MAIN_INPUT, size)
/* output */
#define DECLERA_ITEM_SHORT_MAIN_OUTPUT(size) \
	__perfix_short_item_main(TAG_MAIN_OUTPUT, size)
/* feature */
#define DECLERA_ITEM_SHORT_MAIN_FEATURE(size) \
	__perfix_short_item_main(TAG_MAIN_FEATURE, size)
/* collection */
#define DECLERA_ITEM_SHORT_MAIN_COLLECTION(size) \
	__perfix_short_item_main(TAG_MAIN_COLLECTION, size)
/* end collection */
#define DECLERA_ITEM_SHORT_MAIN_END_COLLECTION(size) \
	__perfix_short_item_main(TAG_MAIN_END_COLLECTION, size)

/* type of collection */
#define MAIN_COLLECTION_PHYSICAL		(0x00)
#define MAIN_COLLECTION_APPLICATION		(0x01)
#define MAIN_COLLECTION_LOGICAL			(0x02)
#define MAIN_COLLECTION_REPORT			(0x03)
#define MAIN_COLLECTION_NAME_ARRAY		(0x04)
#define MAIN_COLLECTION_USAGE_SWITCH	(0x05)
#define MAIN_COLLECTION_USAGE_MODIFIER	(0x06)

/**
 * global
 */
/* global tag */
#define TAG_GLOBAL_USAGE_PAGE			(0x0)
#define TAG_GLOBAL_LOGICAL_MINIMUM		(0x1)
#define TAG_GLOBAL_LOGICAL_MAXUMUM		(0x2)
#define TAG_GLOBAL_PHYSICAL_MINIMUM		(0x3)
#define TAG_GLOBAL_PHYSICAL_MAXIMUM		(0x4)
#define TAG_GLOBAL_UNIT_EXPONENT		(0x5)
#define TAG_GLOBAL_UNIT					(0x6)
#define TAG_GLOBAL_REPORT_SIZE			(0x7)
#define TAG_GLOBAL_REPORT_ID			(0x8)
#define TAG_GLOBAL_REPORT_COUNT			(0x9)
#define TAG_GLOBAL_PUSH					(0xa)
#define TAG_GLOBAL_POP					(0xb)

/* usage page */
#define DECLEAR_ITEM_SHORT_GLOBAL_USAGE_PAGE(size) \
	__perfix_short_item_global(TAG_GLOBAL_USAGE_PAGE, size)
/* logocal minimum */
#define DECLEAR_ITEM_SHORT_GLOBAL_LOGICAL_MINIMUM(size) \
	__perfix_short_item_global(TAG_GLOBAL_LOGICAL_MINIMUM, size)
/* logical maximum */
#define DELCEAR_ITEM_SHORT_GLOBAL_LOGICAL_MAXUMUM(size) \
	__perfix_short_item_global(TAG_GLOBAL_LOGICAL_MAXUMUM, size)
/* physical minimum */
#define DECLEAR_ITEM_SHORT_GLOBAL_PHYSICAL_MINIMUM(size) \
	__perfix_short_item_global(TAG_GLOBAL_PHYSICAL_MINIMUM, size)
/* physical maximum */
#define DECLEAR_ITEM_SHORT_GLOBAL_PHYSICAL_MAXIMUM(size) \
	__perfix_short_item_global(TAG_GLOBAL_PHYSICAL_MAXIMUM, size)
/* unit exponent */
#define DECLEAR_ITEM_SHORT_GLOBAL_UNIT_EXPONENT(size) \
	__perfix_short_item_global(TAG_GLOBAL_UNIT_EXPONENT, size)
/* unit */
#define DECLEAR_ITEM_SHORT_GLOBAL_UNIT(size) \
	__perfix_short_item_global(TAG_GLOBAL_UNIT, size)
/* report size */
#define DECLEAR_ITEM_SHORT_GLOBAL_REPORT_SIZE(size) \
	__perfix_short_item_global(TAG_GLOBAL_REPORT_SIZE, size)
/* report id */
#define DECLEAR_ITEM_SHORT_GLOBAL_REPORT_ID(size) \
	__perfix_short_item_global(TAG_GLOBAL_REPORT_ID, size)
/* report count */
#define DECLEAR_ITEM_SHORT_GLOBAL_REPORT_COUNT(size) \
	__perfix_short_item_global(TAG_GLOBAL_REPORT_COUNT, size)
/* push */
#define DECLEAR_ITEM_SHORT_GLOBAL_PUSH(size) \
	__perfix_short_item_global(TAG_GLOBAL_PUSH, size)
/* pop */
#define DECLEAR_ITEM_SHORT_GLOBAL_POP(size) \
	__perfix_short_item_global(TAG_GLOBAL_POP, size)

/**
 * local
 */
/* local tag */
#define TAG_LOCAL_USAGE					(0x0)
#define TAG_LOCAL_USAGE_MINIMUM			(0x1)
#define TAG_LOCAL_USAGE_MAXiMUM			(0x2)
#define TAG_LOCAL_DESIGNATOR_INDEX		(0x3)
#define TAG_LOCAL_DESIGNATOR_MINIMUM	(0x4)
#define TAG_LOCAL_DESIGNATOR_MAXIMUM	(0x5)
#define TAG_LOCAL_STRING_INDEX			(0x7)
#define TAG_LOCAL_STRING_MINIMUM		(0x8)
#define TAG_LOCAL_STRING_MAXIMUM		(0x9)
#define TAG_LOCAL_DELIMITER				(0xa)

/* usage */
#define DECLEAR_ITEM_SHORT_LOCAL_USAGE(size) \
	__perfix_short_item_local(TAG_LOCAL_USAGE, size)
/* usage minimum */
#define DECLEAR_ITEM_SHORT_LOCAL_USAGE_MINIMUM(size) \
	__perfix_short_item_local(TAG_LOCAL_USAGE_MINIMUM, size)
/* usage maximum */
#define DECLEAR_ITEM_SHORT_LOCAL_USAGE_MAXIMUM(size) \
	__perfix_short_item_local(TAG_LOCAL_USAGE_MAXiMUM, size)
/* designator index */
#define DECLEAR_ITEM_SHORT_LOCAL_DESIGNATOR_INDEX(size) \
	__perfix_short_item_local(TAG_LOCAL_DESIGNATOR_INDEX, size)
/* designator minimum */
#define DECLEAR_ITEM_SHORT_LOCAL_DESIGNATOR_MINIMUM(size) \
	__perfix_short_item_local(TAG_LOCAL_DESIGNATOR_MINIMUM, size)
/* designator maximum */
#define DECLEAR_ITEM_SHORT_LOCAL_DESIGNATOR_MAXIMUM(size) \
	__perfix_short_item_local(TAG_LOCAL_DESIGNATOR_MAXIMUM, size)
/* string index */
#define DECLEAR_ITEM_SHORT_LOCAL_STRING_INDEX(size) \
	__perfix_short_item_local(TAG_LOCAL_STRING_INDEX, size)
/* string minimum */
#define DECLEAR_ITEM_SHORT_LOCAL_STRING_MINIMUM(size) \
	__perfix_short_item_local(TAG_LOCAL_STRING_MINIMUM, size)
/* string maximum */
#define DECLEAR_ITEM_SHORT_LOCAL_STRING_MAXIMUM(size) \
	__perfix_short_item_local(TAG_LOCAL_STRING_MAXIMUM, size)
/* delimiter */
#define DECLEAR_ITEM_SHORT_LOCAL_DELIMITER(size) \
	__perfix_short_item_local(TAG_LOCAL_DELIMITER, size)

struct part_of_hid_descriptor {
	u8 		bDescriptorType;
	u16		wDescriptorLength;
} __attribute__((packed));

/* hid descriptor */
#define HID_DESCRIPTOR(n) \
	hid_descriptor_##n
#define DECLEAR_HID_DESCRIPTOR(n) \
struct HID_DESCRIPTOR(n) { \
	u8								bLength; \
	u8   							bDescriptorType; \
	u16	 							bcdHID;	\
	u8								bCountryCode; \
	u8 								bNumDescriptors; \
	struct part_of_hid_descriptor 	hid_descs[n]; \
} __attribute__((packed))

/* hid physical descriptor */
struct hid_physical_descriptor_zero {
	u8 	bNumber;
	u16	bLength;
} __attribute__((packed));

struct hid_physical {
	u8 	bDesignator;
	u8 	bFlags;
} __attribute__((packed));

#define HID_PHYISCAL_DESCRIPTOR(n) \
	hid_physical_descriptor_##n
#define DECLEAR_HID_PHYSICAL_DESCRIPTOR(n) \
struct HID_PHYISCAL_DESCRIPTOR(n) { \
	u8 					bPhysicalInfo; \
	struct hid_physical dPhysical[n]; \
} __attribute__((packed))

/**
 * hid class request
 */
/* get_report request allows the host to receive a report via the control pipe */
#define HID_CLASS_GET_REPORT			(0x01)
/* The Get_Idle request reads the current idle rate for a particular Input report */
#define HID_CLASS_GET_IDLE				(0x02)
/* The Get_Protocol request reads which protocol is currently active (either the boot 
 * protocol or the report protocol.) 
 */
#define HID_CLASS_GET_PROTOCOL			(0x03)
/* The Set_Report request allows the host to send a report to the device,
 * possibly setting the state of input, output, or feature controls
 */
#define	HID_CLASS_SET_REPORT			(0x09)
/* The Set_Idle request silences a particular report on the Interrupt In pipe until a 
 * new event occurs or the specified amount of time passes */
#define HID_CLASS_SET_IDLE				(0x0A)
/* The Set_Protocol switches between the boot protocol and the report protocol (or 
 * vice versa).
 */
#define HID_CLASS_SET_PROTOCOL			(0x0B)

/* report type */
#define HID_CLASS_REPORT_TYPE_INPUT		(0x01)
#define HID_CLASS_REPORT_TYPE_OUTPUT	(0x02)
#define HID_CLASS_REPORT_TYPE_FEATURE	(0x03)

/* hid class descriptor type */
#define HID_CLASS_DESCRIPTOR_TYPE_HID		(0x21)
#define	HID_CLASS_DESCRIPTOR_TYPE_REPORT	(0x22)
#define HID_CLASS_DESCRIPTOR_TYPE_PHYSICAL	(0x23)

#endif