/*
 * Copyright (c) 2011 Matej Klonfar
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
 * HID report descriptor and report data parser implementation.
 */
#include <usb/classes/hidparser.h>
#include <errno.h>
#include <stdio.h>
#include <malloc.h>
#include <mem.h>
#include <usb/debug.h>
#include <assert.h>


/*
 * Data translation private functions
 */
uint32_t usb_hid_report_tag_data_uint32(const uint8_t *data, size_t size);
inline size_t usb_hid_count_item_offset(usb_hid_report_item_t * report_item, size_t offset);
int usb_hid_translate_data(usb_hid_report_field_t *item, const uint8_t *data);
uint32_t usb_hid_translate_data_reverse(usb_hid_report_field_t *item, int32_t value);
int usb_pow(int a, int b);


// TODO: tohle ma bejt asi jinde
int usb_pow(int a, int b)
{
	switch(b) {
		case 0:
			return 1;
			break;
		case 1:
			return a;
			break;
		default:
			return a * usb_pow(a, b-1);
			break;
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
int usb_hid_parse_report(const usb_hid_report_t *report,  
    const uint8_t *data, size_t size, uint8_t *report_id)
{
	link_t *list_item;
	usb_hid_report_field_t *item;

	usb_hid_report_description_t *report_des;
	usb_hid_report_type_t type = USB_HID_REPORT_TYPE_INPUT;

	if(report == NULL) {
		return EINVAL;
	}

	if(report->use_report_ids != 0) {
		*report_id = data[0];
	}	
	else {
		*report_id = 0;
	}


	report_des = usb_hid_report_find_description(report, *report_id, type);

	/* read data */
	list_item = report_des->report_items.next;	   
	while(list_item != &(report_des->report_items)) {

		item = list_get_instance(list_item, usb_hid_report_field_t, link);

		if(USB_HID_ITEM_FLAG_CONSTANT(item->item_flags) == 0) {
			
			if(USB_HID_ITEM_FLAG_VARIABLE(item->item_flags) == 0) {

				// array
				item->value = usb_hid_translate_data(item, data);
			    item->usage = (item->value - item->physical_minimum) + item->usage_minimum;
			}
			else {
				// variable item
				item->value = usb_hid_translate_data(item, data);				
			}				
		}
		list_item = list_item->next;
	}
	   
	return EOK;
	
}

/**
 * Translate data from the report as specified in report descriptor item
 *
 * @param item Report descriptor item with definition of translation
 * @param data Data to translate
 * @param j Index of processed field in report descriptor item
 * @return Translated data
 */
int usb_hid_translate_data(usb_hid_report_field_t *item, const uint8_t *data)
{
	int resolution;
	int offset;
	int part_size;
	
	int32_t value=0;
	int32_t mask;
	const uint8_t *foo;

	// now only shot tags are allowed
	if(item->size > 32) {
		return 0;
	}

	if((item->physical_minimum == 0) && (item->physical_maximum == 0)){
		item->physical_minimum = item->logical_minimum;
		item->physical_maximum = item->logical_maximum;			
	}
	

	if(item->physical_maximum == item->physical_minimum){
	    resolution = 1;
	}
	else {
	    resolution = (item->logical_maximum - item->logical_minimum) / 
		((item->physical_maximum - item->physical_minimum) * 
		(usb_pow(10,(item->unit_exponent))));
	}

	offset = item->offset;
	// FIXME
	if((size_t)(offset/8) != (size_t)((offset+item->size-1)/8)) {
		
		part_size = ((offset+item->size)%8);

		size_t i=0;
		for(i=(size_t)(offset/8); i<=(size_t)(offset+item->size-1)/8; i++){
			if(i == (size_t)(offset/8)) {
				// the higher one
				foo = data + i;
				mask =  ((1 << (item->size-part_size))-1);
				value = (*foo & mask) << part_size;
			}
			else if(i == ((offset+item->size-1)/8)){
				// the lower one
				foo = data + i;
				mask =  ((1 << part_size)-1) << (8-part_size);
				value += ((*foo & mask) >> (8-part_size));
			}
			else {
				value = value << 8;
				value += *(data + 1);
			}
		}
	}
	else {		
		foo = data+(offset/8);
		mask =  ((1 << item->size)-1) << (8-((offset%8)+item->size));
		value = (*foo & mask) >> (8-((offset%8)+item->size));
	}

	if((item->logical_minimum < 0) || (item->logical_maximum < 0)){
		value = USB_HID_UINT32_TO_INT32(value, item->size);
	}

	return (int)(((value - item->logical_minimum) / resolution) + item->physical_minimum);
	
}

/**
 * Returns number of items in input report which are accessible by given usage path
 *
 * @param parser Opaque report descriptor structure
 * @param path Usage path specification
 * @param flags Usage path comparison flags
 * @return Number of items in input report
 */
size_t usb_hid_report_input_length(const usb_hid_report_t *report,
	usb_hid_report_path_t *path, int flags)
{	
	
	size_t ret = 0;

	if(report == NULL) {
		return 0;
	}

	usb_hid_report_description_t *report_des;
	report_des = usb_hid_report_find_description (report, path->report_id, USB_HID_REPORT_TYPE_INPUT);
	if(report_des == NULL) {
		return 0;
	}

	link_t *field_it = report_des->report_items.next;
	usb_hid_report_field_t *field;
	while(field_it != &report_des->report_items) {

		field = list_get_instance(field_it, usb_hid_report_field_t, link);
		if(USB_HID_ITEM_FLAG_CONSTANT(field->item_flags) == 0) {
			
			usb_hid_report_path_append_item (field->collection_path, field->usage_page, field->usage);
			if(usb_hid_report_compare_usage_path (field->collection_path, path, flags) == EOK) {
				ret++;
			}
			usb_hid_report_remove_last_item (field->collection_path);
		}
		
		field_it = field_it->next;
	}

	return ret;
	}

/*** OUTPUT API **/

/** 
 * Allocates output report buffer for output report
 *
 * @param parser Report parsed structure
 * @param size Size of returned buffer
 * @param report_id Report id of created output report
 * @return Returns allocated output buffer for specified output
 */
uint8_t *usb_hid_report_output(usb_hid_report_t *report, size_t *size, uint8_t report_id)
{
	if(report == NULL) {
		*size = 0;
		return NULL;
	}

	link_t *report_it = report->reports.next;
	usb_hid_report_description_t *report_des = NULL;
	while(report_it != &report->reports) {
		report_des = list_get_instance(report_it, usb_hid_report_description_t, link);
		if((report_des->report_id == report_id) && (report_des->type == USB_HID_REPORT_TYPE_OUTPUT)){
			break;
		}

		report_it = report_it->next;
	}

	if(report_des == NULL){
		*size = 0;
		return NULL;
	}
	else {
		*size = (report_des->bit_length + (8 - 1))/8;
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
	if(output != NULL) {
		free (output);
	}
}

/** Returns size of output for given usage path 
 *
 * @param parser Opaque report parser structure
 * @param path Usage path specified which items will be thought for the output
 * @param flags Flags of usage path structure comparison
 * @return Number of items matching the given usage path
 */
size_t usb_hid_report_output_size(usb_hid_report_t *report,
                                  usb_hid_report_path_t *path, int flags)
{
	size_t ret = 0;	
	usb_hid_report_description_t *report_des;

	if(report == NULL) {
		return 0;
	}

	report_des = usb_hid_report_find_description (report, path->report_id, USB_HID_REPORT_TYPE_OUTPUT);
	if(report_des == NULL){
		return 0;
	}
	
	link_t *field_it = report_des->report_items.next;
	usb_hid_report_field_t *field;
	while(field_it != &report_des->report_items) {

		field = list_get_instance(field_it, usb_hid_report_field_t, link);
		if(USB_HID_ITEM_FLAG_CONSTANT(field->item_flags) == 0){
			usb_hid_report_path_append_item (field->collection_path, field->usage_page, field->usage);
			if(usb_hid_report_compare_usage_path (field->collection_path, path, flags) == EOK) {
				ret++;
			}
			usb_hid_report_remove_last_item (field->collection_path);
		}
		
		field_it = field_it->next;
	}

	return ret;
	
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
int usb_hid_report_output_translate(usb_hid_report_t *report, uint8_t report_id,
                                    uint8_t *buffer, size_t size)
{
	link_t *item;	
	int32_t value=0;
	int offset;
	int length;
	int32_t tmp_value;
	
	if(report == NULL) {
		return EINVAL;
	}

	if(report->use_report_ids != 0) {
		buffer[0] = report_id;		
	}

	usb_log_debug("OUTPUT BUFFER: %s\n", usb_debug_str_buffer(buffer,size, 0));
	
	usb_hid_report_description_t *report_des;
	report_des = usb_hid_report_find_description (report, report_id, USB_HID_REPORT_TYPE_OUTPUT);
	if(report_des == NULL){
		return EINVAL;
	}

	usb_hid_report_field_t *report_item;	
	item = report_des->report_items.next;	
	while(item != &report_des->report_items) {
		report_item = list_get_instance(item, usb_hid_report_field_t, link);

			if(USB_HID_ITEM_FLAG_VARIABLE(report_item->item_flags) == 0) {
					
				// array
				value = usb_hid_translate_data_reverse(report_item, report_item->value);
				offset = report_item->offset;
				length = report_item->size;
			}
			else {
				// variable item
				value  = usb_hid_translate_data_reverse(report_item, report_item->value);
				offset = report_item->offset;
				length = report_item->size;
			}

			if((offset/8) == ((offset+length-1)/8)) {
				// je to v jednom bytu
				if(((size_t)(offset/8) >= size) || ((size_t)(offset+length-1)/8) >= size) {
					break; // TODO ErrorCode
				}

				size_t shift = 8 - offset%8 - length;

				value = value << shift;							
				value = value & (((1 << length)-1) << shift);
				
				uint8_t mask = 0;
				mask = 0xff - (((1 << length) - 1) << shift);
				buffer[offset/8] = (buffer[offset/8] & mask) | value;
			}
			else {
				int i = 0;
				uint8_t mask = 0;
				for(i = (offset/8); i <= ((offset+length-1)/8); i++) {
					if(i == (offset/8)) {
						tmp_value = value;
						tmp_value = tmp_value & ((1 << (8-(offset%8)))-1);				
						tmp_value = tmp_value << (offset%8);
	
						mask = ~(((1 << (8-(offset%8)))-1) << (offset%8));
						buffer[i] = (buffer[i] & mask) | tmp_value;			
					}
					else if (i == ((offset + length -1)/8)) {
						
						value = value >> (length - ((offset + length) % 8));
						value = value & ((1 << (length - ((offset + length) % 8))) - 1);
				
						mask = (1 << (length - ((offset + length) % 8))) - 1;
						buffer[i] = (buffer[i] & mask) | value;
					}
					else {
						buffer[i] = value & (0xFF << i);
					}
				}
			}


		// reset value
		report_item->value = 0;
		
		item = item->next;
	}
	
	usb_log_debug("OUTPUT BUFFER: %s\n", usb_debug_str_buffer(buffer,size, 0));

	return EOK;
}

/**
 * Translate given data for putting them into the outoput report
 * @param item Report item structure
 * @param value Value to translate
 * @return ranslated value
 */
uint32_t usb_hid_translate_data_reverse(usb_hid_report_field_t *item, int value)
{
	int ret=0;
	int resolution;

	if(USB_HID_ITEM_FLAG_CONSTANT(item->item_flags)) {
		ret = item->logical_minimum;
	}

	if((item->physical_minimum == 0) && (item->physical_maximum == 0)){
		item->physical_minimum = item->logical_minimum;
		item->physical_maximum = item->logical_maximum;			
	}
	

	if((USB_HID_ITEM_FLAG_VARIABLE(item->item_flags) == 0)) {

		// variable item
		if(item->physical_maximum == item->physical_minimum){
		    resolution = 1;
		}
		else {
		    resolution = (item->logical_maximum - item->logical_minimum) /
			((item->physical_maximum - item->physical_minimum) *
			(usb_pow(10,(item->unit_exponent))));
		}

		ret = ((value - item->physical_minimum) * resolution) + item->logical_minimum;
	}
	else {
		// bitmapa
		if(value == 0) {
			ret = 0;
		}
		else {
			size_t bitmap_idx = (value - item->usage_minimum);
			ret = 1 << bitmap_idx;
		}
	}

	if((item->logical_minimum < 0) || (item->logical_maximum < 0)){
		return USB_HID_INT32_TO_UINT32(ret, item->size);
	}
	return (int32_t)ret;
}

usb_hid_report_item_t *usb_hid_report_item_clone(const usb_hid_report_item_t *item)
{
	usb_hid_report_item_t *new_report_item;
	
	if(!(new_report_item = malloc(sizeof(usb_hid_report_item_t)))) {
		return NULL;
	}					
	memcpy(new_report_item,item, sizeof(usb_hid_report_item_t));
	link_initialize(&(new_report_item->link));

	return new_report_item;
}


usb_hid_report_field_t *usb_hid_report_get_sibling(usb_hid_report_t *report, 
							usb_hid_report_field_t *field, 
                            usb_hid_report_path_t *path, int flags, 
                            usb_hid_report_type_t type)
{
	usb_hid_report_description_t *report_des = usb_hid_report_find_description (report, path->report_id, type);
	link_t *field_it;
	
	if(report_des == NULL){
		return NULL;
	}

	if(field == NULL){
		// vezmu prvni co mathuje podle path!!
		field_it = report_des->report_items.next;
	}
	else {
		field_it = field->link.next;
	}

	while(field_it != &report_des->report_items) {
		field = list_get_instance(field_it, usb_hid_report_field_t, link);

		if(USB_HID_ITEM_FLAG_CONSTANT(field->item_flags) == 0) {
			usb_hid_report_path_append_item (field->collection_path, field->usage_page, field->usage);
			if(usb_hid_report_compare_usage_path (field->collection_path, path, flags) == EOK){
				usb_hid_report_remove_last_item (field->collection_path);
				return field;
			}
			usb_hid_report_remove_last_item (field->collection_path);
		}
		field_it = field_it->next;
	}

	return NULL;
}

uint8_t usb_hid_report_get_report_id(usb_hid_report_t *report, uint8_t report_id, usb_hid_report_type_t type)
{
	if(report == NULL){
		return 0;
	}

	usb_hid_report_description_t *report_des;
	link_t *report_it;
	
	if(report_id == 0) {
		report_it = usb_hid_report_find_description (report, report_id, type)->link.next;		
	}
	else {
		report_it = report->reports.next;
	}

	while(report_it != &report->reports) {
		report_des = list_get_instance(report_it, usb_hid_report_description_t, link);
		if(report_des->type == type){
			return report_des->report_id;
		}
	}

	return 0;
}

void usb_hid_report_reset_local_items(usb_hid_report_item_t *report_item)
{
	if(report_item == NULL)	{
		return;
	}
	
	report_item->usages_count = 0;
	memset(report_item->usages, 0, USB_HID_MAX_USAGES);
	
	report_item->extended_usage_page = 0;
	report_item->usage_minimum = 0;
	report_item->usage_maximum = 0;
	report_item->designator_index = 0;
	report_item->designator_minimum = 0;
	report_item->designator_maximum = 0;
	report_item->string_index = 0;
	report_item->string_minimum = 0;
	report_item->string_maximum = 0;

	return;
}
/**
 * @}
 */
