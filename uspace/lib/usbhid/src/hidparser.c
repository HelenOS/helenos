/*
 * Copyright (c) 2011 Matej Klonfar
 * Copyright (c) 2018 Ondrej Hlavaty
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

/** @addtogroup libusbhid
 * @{
 */
/** @file
 * USB HID report data parser implementation.
 */
#include <usb/hid/hidparser.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include <usb/debug.h>
#include <assert.h>
#include <bitops.h>
#include <macros.h>


/*
 * Data translation private functions
 */
uint32_t usb_hid_report_tag_data_uint32(const uint8_t *data, size_t size);

int usb_hid_translate_data(usb_hid_report_field_t *item, const uint8_t *data);

uint32_t usb_hid_translate_data_reverse(usb_hid_report_field_t *item,
    int32_t value);



static int usb_pow(int a, int b)
{
	switch (b) {
	case 0:
		return 1;
		break;
	case 1:
		return a;
		break;
	default:
		return a * usb_pow(a, b - 1);
		break;
	}
}


/** Returns size of report of specified report id and type in items
 *
 * @param parser Opaque report parser structure
 * @param report_id
 * @param type
 * @return Number of items in specified report
 */
size_t usb_hid_report_size(usb_hid_report_t *report, uint8_t report_id,
    usb_hid_report_type_t type)
{
	usb_hid_report_description_t *report_des;

	if (report == NULL) {
		return 0;
	}

	report_des = usb_hid_report_find_description(report, report_id, type);
	if (report_des == NULL) {
		return 0;
	} else {
		return report_des->item_length;
	}
}

/** Returns size of report of specified report id and type in bytes
 *
 * @param parser Opaque report parser structure
 * @param report_id
 * @param type
 * @return Number of items in specified report
 */
size_t usb_hid_report_byte_size(usb_hid_report_t *report, uint8_t report_id,
    usb_hid_report_type_t type)
{
	usb_hid_report_description_t *report_des;

	if (report == NULL) {
		return 0;
	}

	report_des = usb_hid_report_find_description(report, report_id, type);
	if (report_des == NULL) {
		return 0;
	} else {
		return ((report_des->bit_length + 7) / 8);
	}
}


/** Parse and act upon a HID report.
 *
 * @see usb_hid_parse_report_descriptor
 *
 * @param parser Opaque HID report parser structure.
 * @param data Data for the report.
 * @return Error code.
 */
errno_t usb_hid_parse_report(const usb_hid_report_t *report, const uint8_t *data,
    size_t size, uint8_t *report_id)
{
	usb_hid_report_description_t *report_des;
	usb_hid_report_type_t type = USB_HID_REPORT_TYPE_INPUT;

	if (report == NULL) {
		return EINVAL;
	}

	if (report->use_report_ids != 0) {
		*report_id = data[0];
	} else {
		*report_id = 0;
	}

	report_des = usb_hid_report_find_description(report, *report_id,
	    type);

	if (report_des == NULL) {
		return EINVAL;
	}

	/* read data */
	list_foreach(report_des->report_items, ritems_link,
	    usb_hid_report_field_t, item) {

		if (USB_HID_ITEM_FLAG_CONSTANT(item->item_flags) == 0) {

			if (USB_HID_ITEM_FLAG_VARIABLE(item->item_flags) == 0) {
				/* array */
				item->value =
				    usb_hid_translate_data(item, data);

				item->usage = USB_HID_EXTENDED_USAGE(
				    item->usages[item->value -
				    item->physical_minimum]);

				item->usage_page =
				    USB_HID_EXTENDED_USAGE_PAGE(
				    item->usages[item->value -
				    item->physical_minimum]);

				usb_hid_report_set_last_item(
				    item->collection_path,
				    USB_HID_TAG_CLASS_GLOBAL,
				    item->usage_page);

				usb_hid_report_set_last_item(
				    item->collection_path,
				    USB_HID_TAG_CLASS_LOCAL, item->usage);
			} else {
				/* variable item */
				item->value = usb_hid_translate_data(item,
				    data);
			}
		}
	}

	return EOK;
}


/**
 * Translate data from the report as specified in report descriptor item
 *
 * @param item Report descriptor item with definition of translation
 * @param data Data to translate
 * @return Translated data
 */
int usb_hid_translate_data(usb_hid_report_field_t *item, const uint8_t *data)
{
	/* now only short tags are allowed */
	if (item->size > 32) {
		return 0;
	}

	if ((item->physical_minimum == 0) && (item->physical_maximum == 0)) {
		item->physical_minimum = item->logical_minimum;
		item->physical_maximum = item->logical_maximum;
	}

	int resolution;
	if (item->physical_maximum == item->physical_minimum) {
		resolution = 1;
	} else {
		resolution = (item->logical_maximum - item->logical_minimum) /
		    ((item->physical_maximum - item->physical_minimum) *
		    (usb_pow(10, (item->unit_exponent))));
	}

	int32_t value = 0;

	/* First, skip all bytes we don't care */
	data += item->offset / 8;

	int bits = item->size;
	int taken = 0;

	/* Than we take the higher bits from the LSB */
	const unsigned bit_offset = item->offset % 8;
	const int lsb_bits = min(bits, 8);

	value |= (*data >> bit_offset) & BIT_RRANGE(uint8_t, lsb_bits);
	bits -= lsb_bits;
	taken += lsb_bits;
	data++;

	/* Then there may be bytes, which we take as a whole. */
	while (bits > 8) {
		value |= *data << taken;
		taken += 8;
		bits -= 8;
		data++;
	}

	/* And, finally, lower bits from HSB. */
	if (bits > 0) {
		value |= (*data & BIT_RRANGE(uint8_t, bits)) << taken;
	}

	if ((item->logical_minimum < 0) || (item->logical_maximum < 0)) {
		value = USB_HID_UINT32_TO_INT32(value, item->size);
	}

	return (int) (((value - item->logical_minimum) / resolution) +
	    item->physical_minimum);
}


/* OUTPUT API */

/**
 * Allocates output report buffer for output report
 *
 * @param parser Report parsed structure
 * @param size Size of returned buffer
 * @param report_id Report id of created output report
 * @return Returns allocated output buffer for specified output
 */
uint8_t *usb_hid_report_output(usb_hid_report_t *report, size_t *size,
    uint8_t report_id)
{
	if (report == NULL) {
		*size = 0;
		return NULL;
	}

	usb_hid_report_description_t *report_des = NULL;

	list_foreach(report->reports, reports_link,
	    usb_hid_report_description_t, rdes) {
		if ((rdes->report_id == report_id) &&
		    (rdes->type == USB_HID_REPORT_TYPE_OUTPUT)) {
			report_des = rdes;
			break;
		}
	}

	if (report_des == NULL) {
		*size = 0;
		return NULL;
	} else {
		*size = (report_des->bit_length + (8 - 1)) / 8;
		uint8_t *ret = malloc((*size) * sizeof(uint8_t));
		memset(ret, 0, (*size) * sizeof(uint8_t));
		return ret;
	}
}


/** Frees output report buffer
 *
 * @param output Output report buffer
 * @return void
 */
void usb_hid_report_output_free(uint8_t *output)
{
	if (output != NULL) {
		free(output);
	}
}

/** Makes the output report buffer for data given in the report structure
 *
 * @param parser Opaque report parser structure
 * @param path Usage path specifing which parts of output will be set
 * @param flags Usage path structure comparison flags
 * @param buffer Output buffer
 * @param size Size of output buffer
 * @return Error code
 */
errno_t usb_hid_report_output_translate(usb_hid_report_t *report,
    uint8_t report_id, uint8_t *buffer, size_t size)
{
	int32_t value = 0;
	int offset;
	int length;
	int32_t tmp_value;

	if (report == NULL) {
		return EINVAL;
	}

	if (report->use_report_ids != 0) {
		buffer[0] = report_id;
	}

	usb_hid_report_description_t *report_des;
	report_des = usb_hid_report_find_description(report, report_id,
	    USB_HID_REPORT_TYPE_OUTPUT);

	if (report_des == NULL) {
		return EINVAL;
	}

	list_foreach(report_des->report_items, ritems_link,
	    usb_hid_report_field_t, report_item) {
		value = usb_hid_translate_data_reverse(report_item,
		    report_item->value);

		offset = report_des->bit_length - report_item->offset - 1;
		length = report_item->size;

		usb_log_debug("\ttranslated value: %x", value);

		if ((offset / 8) == ((offset + length - 1) / 8)) {
			if (((size_t) (offset / 8) >= size) ||
			    ((size_t) (offset + length - 1) / 8) >= size) {
				break; // TODO ErrorCode
			}
			size_t shift = 8 - offset % 8 - length;
			value = value << shift;
			value = value & (((1 << length) - 1) << shift);

			uint8_t mask = 0;
			mask = 0xff - (((1 << length) - 1) << shift);
			buffer[offset / 8] = (buffer[offset / 8] & mask) |
			    value;
		} else {
			int i = 0;
			uint8_t mask = 0;
			for (i = (offset / 8);
			    i <= ((offset + length - 1) / 8); i++) {
				if (i == (offset / 8)) {
					tmp_value = value;
					tmp_value = tmp_value &
					    ((1 << (8 - (offset % 8))) - 1);

					tmp_value = tmp_value << (offset % 8);

					mask = ~(((1 << (8 - (offset % 8))) - 1) << (offset % 8));

					buffer[i] = (buffer[i] & mask) |
					    tmp_value;
				} else if (i == ((offset + length - 1) / 8)) {

					value = value >> (length -
					    ((offset + length) % 8));

					value = value & ((1 << (length -
					    ((offset + length) % 8))) - 1);

					mask = (1 << (length -
					    ((offset + length) % 8))) - 1;

					buffer[i] = (buffer[i] & mask) | value;
				} else {
					buffer[i] = value & (0xff << i);
				}
			}
		}

		/* reset value */
		report_item->value = 0;
	}

	return EOK;
}


/**
 * Translate given data for putting them into the outoput report
 * @param item Report item structure
 * @param value Value to translate
 * @return ranslated value
 */
uint32_t usb_hid_translate_data_reverse(usb_hid_report_field_t *item,
    int value)
{
	int ret = 0;
	int resolution;

	if (USB_HID_ITEM_FLAG_CONSTANT(item->item_flags)) {
		return item->logical_minimum;
	}

	if ((item->physical_minimum == 0) && (item->physical_maximum == 0)) {
		item->physical_minimum = item->logical_minimum;
		item->physical_maximum = item->logical_maximum;
	}

	/* variable item */
	if (item->physical_maximum == item->physical_minimum) {
		resolution = 1;
	} else {
		resolution = (item->logical_maximum - item->logical_minimum) /
		    ((item->physical_maximum - item->physical_minimum) *
		    (usb_pow(10, (item->unit_exponent))));
	}

	ret = ((value - item->physical_minimum) * resolution) +
	    item->logical_minimum;

	usb_log_debug("\tvalue(%x), resolution(%x), phymin(%x) logmin(%x), "
	    "ret(%x)\n", value, resolution, item->physical_minimum,
	    item->logical_minimum, ret);

	if ((item->logical_minimum < 0) || (item->logical_maximum < 0)) {
		return USB_HID_INT32_TO_UINT32(ret, item->size);
	}

	return (int32_t) 0 + ret;
}


/**
 * Clones given state table
 *
 * @param item State table to clone
 * @return Pointer to the cloned item
 */
usb_hid_report_item_t *usb_hid_report_item_clone(
    const usb_hid_report_item_t *item)
{
	usb_hid_report_item_t *new_report_item;

	if (!(new_report_item = malloc(sizeof(usb_hid_report_item_t)))) {
		return NULL;
	}
	memcpy(new_report_item, item, sizeof(usb_hid_report_item_t));
	link_initialize(&(new_report_item->link));

	return new_report_item;
}


/**
 * Function for sequence walking through the report. Returns next field in the
 * report or the first one when no field is given.
 *
 * @param report Searched report structure
 * @param field Current field. If NULL is given, the first one in the report
 * is returned. Otherwise the next one i nthe list is returned.
 * @param path Usage path specifying which fields wa are interested in.
 * @param flags Flags defining mode of usage paths comparison
 * @param type Type of report we search.
 * @retval NULL if no field is founded
 * @retval Pointer to the founded report structure when founded
 */
usb_hid_report_field_t *usb_hid_report_get_sibling(usb_hid_report_t *report,
    usb_hid_report_field_t *field, usb_hid_report_path_t *path, int flags,
    usb_hid_report_type_t type)
{
	usb_hid_report_description_t *report_des =
	    usb_hid_report_find_description(report, path->report_id, type);

	link_t *field_it;

	if (report_des == NULL) {
		return NULL;
	}

	if (field == NULL) {
		field_it = report_des->report_items.head.next;
	} else {
		field_it = field->ritems_link.next;
	}

	while (field_it != &report_des->report_items.head) {
		field = list_get_instance(field_it, usb_hid_report_field_t,
		    ritems_link);

		if (USB_HID_ITEM_FLAG_CONSTANT(field->item_flags) == 0) {
			usb_hid_report_path_append_item(field->collection_path,
			    field->usage_page, field->usage);

			if (usb_hid_report_compare_usage_path(
			    field->collection_path, path, flags) == 0) {
				usb_hid_report_remove_last_item(
				    field->collection_path);
				return field;
			}
			usb_hid_report_remove_last_item(field->collection_path);
		}
		field_it = field_it->next;
	}

	return NULL;
}


/**
 * Returns next report_id of report of specified type. If zero is given than
 * first report_id of specified type is returned (0 is not legal value for
 * repotr_id)
 *
 * @param report_id Current report_id, 0 if there is no current report_id
 * @param type Type of searched report
 * @param report Report structure inwhich we search
 * @retval 0 if report structure is null or there is no specified report
 * @retval report_id otherwise
 */
uint8_t usb_hid_get_next_report_id(usb_hid_report_t *report, uint8_t report_id,
    usb_hid_report_type_t type)
{
	if (report == NULL) {
		return 0;
	}

	usb_hid_report_description_t *report_des;
	link_t *report_it;

	if (report_id > 0) {
		report_des = usb_hid_report_find_description(report, report_id,
		    type);
		if (report_des == NULL) {
			return 0;
		} else {
			report_it = report_des->reports_link.next;
		}
	} else {
		report_it = report->reports.head.next;
	}

	while (report_it != &report->reports.head) {
		report_des = list_get_instance(report_it,
		    usb_hid_report_description_t, reports_link);

		if (report_des->type == type) {
			return report_des->report_id;
		}

		report_it = report_it->next;
	}

	return 0;
}


/**
 * Reset all local items in given state table
 *
 * @param report_item State table containing current state of report
 * descriptor parsing
 *
 * @return void
 */
void usb_hid_report_reset_local_items(usb_hid_report_item_t *report_item)
{
	if (report_item == NULL) {
		return;
	}

	report_item->usages_count = 0;
	memset(report_item->usages, 0, sizeof(report_item->usages));

	report_item->extended_usage_page = 0;
	report_item->usage_minimum = 0;
	report_item->usage_maximum = 0;
	report_item->designator_index = 0;
	report_item->designator_minimum = 0;
	report_item->designator_maximum = 0;
	report_item->string_index = 0;
	report_item->string_minimum = 0;
	report_item->string_maximum = 0;
}

/**
 * @}
 */
