/*
 * Copyright (c) 2010 Vojtech Horky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup libusb
 * @{
 */
/** @file
 * USB HID report descriptor and report data parser
 */
#ifndef LIBUSB_HIDPARSER_H_
#define LIBUSB_HIDPARSER_H_

#include <stdint.h>
#include <adt/list.h>
#include <usb/classes/hid_report_items.h>

/**
 * Item prefix
 */
#define USB_HID_ITEM_SIZE(data) 	((uint8_t)(data & 0x3))
#define USB_HID_ITEM_TAG(data) 		((uint8_t)((data & 0xF0) >> 4))
#define USB_HID_ITEM_TAG_CLASS(data)	((uint8_t)((data & 0xC) >> 2))
#define USB_HID_ITEM_IS_LONG(data)	(data == 0xFE)


/**
 * Input/Output/Feature Item flags
 */
/** Constant (1) / Variable (0) */
#define USB_HID_ITEM_FLAG_CONSTANT(flags) 	((flags & 0x1) == 0x1)
/** Variable (1) / Array (0) */
#define USB_HID_ITEM_FLAG_VARIABLE(flags) 	((flags & 0x2) == 0x2)
/** Absolute / Relative*/
#define USB_HID_ITEM_FLAG_RELATIVE(flags) 	((flags & 0x4) == 0x4)
/** Wrap / No Wrap */
#define USB_HID_ITEM_FLAG_WRAP(flags)		((flags & 0x8) == 0x8)
#define USB_HID_ITEM_FLAG_LINEAR(flags)		((flags & 0x10) == 0x10)
#define USB_HID_ITEM_FLAG_PREFERRED(flags)	((flags & 0x20) == 0x20)
#define USB_HID_ITEM_FLAG_POSITION(flags)	((flags & 0x40) == 0x40)
#define USB_HID_ITEM_FLAG_VOLATILE(flags)	((flags & 0x80) == 0x80)
#define USB_HID_ITEM_FLAG_BUFFERED(flags)	((flags & 0x100) == 0x100)


/**
 * Description of path of usage pages and usages in report descriptor
 */
#define USB_HID_PATH_COMPARE_STRICT				0
#define USB_HID_PATH_COMPARE_END				1
#define USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY	4
#define USB_HID_PATH_COMPARE_COLLECTION_ONLY	2 /* porovnava jenom cestu z Kolekci */


#define USB_HID_MAX_USAGES	20

typedef enum {
	USB_HID_REPORT_TYPE_INPUT = 1,
	USB_HID_REPORT_TYPE_OUTPUT = 2,
	USB_HID_REPORT_TYPE_FEATURE = 3
} usb_hid_report_type_t;

/** Collection usage path structure */
typedef struct {
	/** */
	uint32_t usage_page;
	/** */	
	uint32_t usage;

	uint8_t flags;
	/** */
	link_t link;
} usb_hid_report_usage_path_t;

/** */
typedef struct {
	/** */	
	int depth;	
	uint8_t report_id;
	
	/** */	
	link_t link; /* list */

	link_t head; /* head of list of usage paths */

} usb_hid_report_path_t;


typedef struct {
	/** */
	int report_count;
	link_t reports;		/** list of usb_hid_report_description_t */

	link_t collection_paths;
	int collection_paths_count;

	int use_report_ids;
	
} usb_hid_report_t;

typedef struct {
	uint8_t report_id;
	usb_hid_report_type_t type;

	size_t bit_length;
	size_t item_length;
	
	link_t report_items;	/** list of report items (fields) */

	link_t link;
} usb_hid_report_description_t;

typedef struct {

	int offset;
	size_t size;

	uint16_t usage_page;
	uint16_t usage;

	uint8_t item_flags;
	usb_hid_report_path_t *collection_path;

	int32_t logical_minimum;
	int32_t logical_maximum;
	int32_t physical_minimum;
	int32_t physical_maximum;
	uint32_t usage_minimum;
	uint32_t usage_maximum;
	uint32_t unit;
	uint32_t unit_exponent;
	

	int32_t value;

	link_t link;
} usb_hid_report_field_t;



/**
 * state table
 */
typedef struct {
	/** report id */	
	int32_t id;
	
	/** */
	uint16_t extended_usage_page;
	uint32_t usages[USB_HID_MAX_USAGES];
	int usages_count;

	/** */
	uint32_t usage_page;

	/** */	
	uint32_t usage_minimum;
	/** */	
	uint32_t usage_maximum;
	/** */	
	int32_t logical_minimum;
	/** */	
	int32_t logical_maximum;
	/** */	
	int32_t size;
	/** */	
	int32_t count;
	/** */	
	size_t offset;
	/** */	
	int32_t unit_exponent;
	/** */	
	int32_t unit;

	/** */
	uint32_t string_index;
	/** */	
	uint32_t string_minimum;
	/** */	
	uint32_t string_maximum;
	/** */	
	uint32_t designator_index;
	/** */	
	uint32_t designator_minimum;
	/** */	
	uint32_t designator_maximum;
	/** */	
	int32_t physical_minimum;
	/** */	
	int32_t physical_maximum;

	/** */	
	uint8_t item_flags;

	usb_hid_report_type_t type;

	/** current collection path*/	
	usb_hid_report_path_t *usage_path;
	/** */	
	link_t link;
} usb_hid_report_item_t;

/** HID parser callbacks for IN items. */
typedef struct {
	/** Callback for keyboard.
	 *
	 * @param key_codes Array of pressed key (including modifiers).
	 * @param count Length of @p key_codes.
	 * @param arg Custom argument.
	 */
	void (*keyboard)(const uint8_t *key_codes, size_t count, const uint8_t report_id, void *arg);
} usb_hid_report_in_callbacks_t;


typedef enum {
	USB_HID_MOD_LCTRL = 0x01,
	USB_HID_MOD_LSHIFT = 0x02,
	USB_HID_MOD_LALT = 0x04,
	USB_HID_MOD_LGUI = 0x08,
	USB_HID_MOD_RCTRL = 0x10,
	USB_HID_MOD_RSHIFT = 0x20,
	USB_HID_MOD_RALT = 0x40,
	USB_HID_MOD_RGUI = 0x80,
	USB_HID_MOD_COUNT = 8
} usb_hid_modifiers_t;

static const usb_hid_modifiers_t 
    usb_hid_modifiers_consts[USB_HID_MOD_COUNT] = {
	USB_HID_MOD_LCTRL,
	USB_HID_MOD_LSHIFT,
	USB_HID_MOD_LALT,
	USB_HID_MOD_LGUI,
	USB_HID_MOD_RCTRL,
	USB_HID_MOD_RSHIFT,
	USB_HID_MOD_RALT,
	USB_HID_MOD_RGUI
};

/*
 * Descriptor parser functions
 */

/** */
int usb_hid_parse_report_descriptor(usb_hid_report_t *report, 
    const uint8_t *data, size_t size);

/** */
void usb_hid_free_report(usb_hid_report_t *report);

/** */
void usb_hid_descriptor_print(usb_hid_report_t *report);


/*
 * Input report parser functions
 */
/** */
int usb_hid_parse_report(const usb_hid_report_t *report, const uint8_t *data, size_t size);

/** */
size_t usb_hid_report_input_length(const usb_hid_report_t *report,
	usb_hid_report_path_t *path, int flags);



/* 
 * usage path functions 
 */
/** */
usb_hid_report_path_t *usb_hid_report_path(void);

/** */
void usb_hid_report_path_free(usb_hid_report_path_t *path);

/** */
int usb_hid_report_path_set_report_id(usb_hid_report_path_t *usage_path, uint8_t report_id);

/** */
int usb_hid_report_path_append_item(usb_hid_report_path_t *usage_path, int32_t usage_page, int32_t usage);

/** */
void usb_hid_report_remove_last_item(usb_hid_report_path_t *usage_path);

/** */
void usb_hid_report_null_last_item(usb_hid_report_path_t *usage_path);

/** */
void usb_hid_report_set_last_item(usb_hid_report_path_t *usage_path, int32_t tag, int32_t data);

/** */
int usb_hid_report_compare_usage_path(usb_hid_report_path_t *report_path, usb_hid_report_path_t *path, int flags);

/** */
usb_hid_report_path_t *usb_hid_report_path_clone(usb_hid_report_path_t *usage_path);


/*
 * Output report parser functions
 */
/** Allocates output report buffer*/
uint8_t *usb_hid_report_output(usb_hid_report_t *report, size_t *size, uint8_t report_id);

/** Frees output report buffer*/
void usb_hid_report_output_free(uint8_t *output);

/** Returns size of output for given usage path */
size_t usb_hid_report_output_size(usb_hid_report_t *report,
                                  usb_hid_report_path_t *path, int flags);

/** Sets data in report structure */
int usb_hid_report_output_set_data(usb_hid_report_t *report, 
                                   usb_hid_report_path_t *path, int flags, 
                                  int *data, size_t data_size);

/** Makes the output report buffer by translated given data */
int usb_hid_report_output_translate(usb_hid_report_t *report, uint8_t report_id, uint8_t *buffer, size_t size);
#endif
/**
 * @}
 */
