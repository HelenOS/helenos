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
 * HID report descriptor and report data parser implementation.
 */
#include <usb/classes/hidparser.h>
#include <errno.h>
#include <stdio.h>
#include <malloc.h>
#include <mem.h>
#include <usb/debug.h>

/** The new report item flag. Used to determine when the item is completly
 * configured and should be added to the report structure
 */
#define USB_HID_NEW_REPORT_ITEM 1

/** No special action after the report descriptor tag is processed should be
 * done
 */
#define USB_HID_NO_ACTION	2

#define USB_HID_RESET_OFFSET	3

/** Unknown tag was founded in report descriptor data*/
#define USB_HID_UNKNOWN_TAG		-99

/*
 * Private descriptor parser functions
 */
int usb_hid_report_init(usb_hid_report_t *report);
int usb_hid_report_append_fields(usb_hid_report_t *report, usb_hid_report_item_t *report_item);
usb_hid_report_description_t * usb_hid_report_find_description(const usb_hid_report_t *report, uint8_t report_id, usb_hid_report_type_t type);
int usb_hid_report_parse_tag(uint8_t tag, uint8_t class, const uint8_t *data, size_t item_size,
                             usb_hid_report_item_t *report_item, usb_hid_report_path_t *usage_path);
int usb_hid_report_parse_main_tag(uint8_t tag, const uint8_t *data, size_t item_size,
                             usb_hid_report_item_t *report_item, usb_hid_report_path_t *usage_path);
int usb_hid_report_parse_global_tag(uint8_t tag, const uint8_t *data, size_t item_size,
                             usb_hid_report_item_t *report_item, usb_hid_report_path_t *usage_path);
int usb_hid_report_parse_local_tag(uint8_t tag, const uint8_t *data, size_t item_size,
                             usb_hid_report_item_t *report_item, usb_hid_report_path_t *usage_path);

void usb_hid_print_usage_path(usb_hid_report_path_t *path);
void usb_hid_descriptor_print_list(link_t *head);
int usb_hid_report_reset_local_items();
void usb_hid_free_report_list(link_t *head);
usb_hid_report_item_t *usb_hid_report_item_clone(const usb_hid_report_item_t *item);
/*
 * Data translation private functions
 */
int32_t usb_hid_report_tag_data_int32(const uint8_t *data, size_t size);
inline size_t usb_hid_count_item_offset(usb_hid_report_item_t * report_item, size_t offset);
int usb_hid_translate_data(usb_hid_report_field_t *item, const uint8_t *data, size_t j);
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

/**
 * Initialize the report descriptor parser structure
 *
 * @param parser Report descriptor parser structure
 * @return Error code
 */
int usb_hid_report_init(usb_hid_report_t *report)
{
	if(report == NULL) {
		return EINVAL;
	}

	memset(report, 0, sizeof(usb_hid_report_t));
	list_initialize(&report->reports);
	list_initialize(&report->collection_paths);

	report->use_report_ids = 0;
    return EOK;   
}

int usb_hid_report_append_fields(usb_hid_report_t *report, usb_hid_report_item_t *report_item)
{
	usb_hid_report_field_t *field;
	int i;


	/* find or append current collection path to the list */
	link_t *path_it = report->collection_paths.next;
	usb_hid_report_path_t *path = NULL;
	while(path_it != &report->collection_paths) {
		path = list_get_instance(path_it, usb_hid_report_path_t, link);
		
		if(usb_hid_report_compare_usage_path(path, report_item->usage_path, USB_HID_PATH_COMPARE_STRICT) == EOK){
			break;
		}			
		path_it = path_it->next;
	}
	if(path_it == &report->collection_paths) {
		path = usb_hid_report_path_clone(report_item->usage_path);			

		usb_log_debug("PUVODNI: \n");
		usb_hid_print_usage_path (report_item->usage_path);
		usb_log_debug("KLON - path: \n");
		usb_hid_print_usage_path (path);		
		usb_log_debug("KLON - clone: \n");
		usb_hid_print_usage_path (usb_hid_report_path_clone(report_item->usage_path));		

		list_append(&path->link, &report->collection_paths);			

		// PROC SE KRUVA VLOZI NESMYSLY??
		usb_log_debug("VLOZENO: \n");
		usb_hid_print_usage_path (list_get_instance(report->collection_paths.prev, usb_hid_report_path_t, link));
		usb_log_debug("\n");
		report->collection_paths_count++;
	}

	
	for(i=0; i<report_item->count; i++){

		field = malloc(sizeof(usb_hid_report_field_t));
		memset(field, 0, sizeof(usb_hid_report_field_t));
		list_initialize(&field->link);

		/* fill the attributes */
		field->collection_path = path;
		field->logical_minimum = report_item->logical_minimum;
		field->logical_maximum = report_item->logical_maximum;
		field->physical_minimum = report_item->physical_minimum;
		field->physical_maximum = report_item->physical_maximum;
		field->usage_minimum = report_item->usage_minimum;
		field->usage_maximum = report_item->usage_maximum;
		if(report_item->extended_usage_page != 0){
			field->usage_page = report_item->extended_usage_page;
		}
		else {
			field->usage_page = report_item->usage_page;
		}
		
		if(report_item->usages_count > 0 && ((report_item->usage_minimum = 0) && (report_item->usage_maximum = 0))) {
			if(i < report_item->usages_count){
				if((report_item->usages[i] & 0xFF00) > 0){
					field->usage_page = (report_item->usages[i] >> 16);					
					field->usage = (report_item->usages[i] & 0xFF);
				}
				else {
					field->usage = report_item->usages[i];
				}
			}
			else {
				field->usage = report_item->usages[report_item->usages_count - 1];
			}
		}		
		
		field->size = report_item->size;
		field->offset = report_item->offset + (i * report_item->size);
		if(report_item->id != 0) {
			field->offset += 8;
			report->use_report_ids = 1;
		}
		field->item_flags = report_item->item_flags;

		/* find the right report list*/
		usb_hid_report_description_t *report_des;
		report_des = usb_hid_report_find_description(report, report_item->id, report_item->type);
		if(report_des == NULL){
			report_des = malloc(sizeof(usb_hid_report_description_t));
			memset(report_des, 0, sizeof(usb_hid_report_description_t));

			report_des->type = report_item->type;
			report_des->report_id = report_item->id;
			list_initialize (&report_des->link);
			list_initialize (&report_des->report_items);

			list_append(&report_des->link, &report->reports);
			report->report_count++;
		}

		/* append this field to the end of founded report list */
		list_append (&field->link, &report_des->report_items);
		
		/* update the sizes */
		report_des->bit_length += field->size;
		report_des->item_length++;

	}


	return EOK;
}

usb_hid_report_description_t * usb_hid_report_find_description(const usb_hid_report_t *report, uint8_t report_id, usb_hid_report_type_t type)
{
	link_t *report_it = report->reports.next;
	usb_hid_report_description_t *report_des = NULL;
	
	while(report_it != &report->reports) {
		report_des = list_get_instance(report_it, usb_hid_report_description_t, link);
		if((report_des->report_id == report_id) && (report_des->type == type)){
			break;
		}
		report_it = report_it->next;
	}
	if(report_it ==&report->reports){
		return NULL;
	}

	return report_des;
}

/** Parse HID report descriptor.
 *
 * @param parser Opaque HID report parser structure.
 * @param data Data describing the report.
 * @return Error code.
 */
int usb_hid_parse_report_descriptor(usb_hid_report_t *report, 
    const uint8_t *data, size_t size)
{
	size_t i=0;
	uint8_t tag=0;
	uint8_t item_size=0;
	int class=0;
	int ret;
	usb_hid_report_item_t *report_item=0;
	usb_hid_report_item_t *new_report_item;	
	usb_hid_report_path_t *usage_path;

	size_t offset_input=0;
	size_t offset_output=0;
	size_t offset_feature=0;

	link_t stack;
	list_initialize(&stack);	

	/* parser structure initialization*/
	if(usb_hid_report_init(report) != EOK) {
		return EINVAL;
	}
	
	/*report item initialization*/
	if(!(report_item=malloc(sizeof(usb_hid_report_item_t)))){
		return ENOMEM;
	}
	memset(report_item, 0, sizeof(usb_hid_report_item_t));
	list_initialize(&(report_item->link));	

	/* usage path context initialization */
	if(!(usage_path=usb_hid_report_path())){
		return ENOMEM;
	}
	
	while(i<size){	
		if(!USB_HID_ITEM_IS_LONG(data[i])){

			if((i+USB_HID_ITEM_SIZE(data[i]))>= size){
				return EINVAL;
			}
			
			tag = USB_HID_ITEM_TAG(data[i]);
			item_size = USB_HID_ITEM_SIZE(data[i]);
			class = USB_HID_ITEM_TAG_CLASS(data[i]);
			
			ret = usb_hid_report_parse_tag(tag,class,data+i+1,
			                               item_size,report_item, usage_path);
			switch(ret){
				case USB_HID_NEW_REPORT_ITEM:
					// store report item to report and create the new one
					// store current collection path
					report_item->usage_path = usage_path;
					
					usb_hid_report_path_set_report_id(report_item->usage_path, report_item->id);	
					if(report_item->id != 0){
						report->use_report_ids = 1;
					}
					
					switch(tag) {
						case USB_HID_REPORT_TAG_INPUT:
							report_item->type = USB_HID_REPORT_TYPE_INPUT;
							report_item->offset = offset_input;
							offset_input += report_item->count * report_item->size;
							break;
						case USB_HID_REPORT_TAG_OUTPUT:
							report_item->type = USB_HID_REPORT_TYPE_OUTPUT;
							report_item->offset = offset_output;
							offset_output += report_item->count * report_item->size;

							break;
						case USB_HID_REPORT_TAG_FEATURE:
							report_item->type = USB_HID_REPORT_TYPE_FEATURE;
							report_item->offset = offset_feature;
							offset_feature += report_item->count * report_item->size;
							break;
						default:
						    usb_log_debug("\tjump over - tag %X\n", tag);
						    break;
					}
					
					/* 
					 * append new fields to the report
					 * structure 					 
					 */
					usb_log_debug("PRED: \n");
					if(report->collection_paths.next == &report->collection_paths) {
						usb_log_debug("PRAZDNY\n");
					}
					else {
						usb_hid_print_usage_path (list_get_instance(report->collection_paths.prev,usb_hid_report_path_t, link));
					}
					usb_log_debug("-----------\n");
					usb_hid_report_append_fields(report, report_item);

					/* reset local items */
					while(report_item->usages_count > 0){
						report_item->usages[--(report_item->usages_count)] = 0;
					}

					report_item->extended_usage_page = 0;
					report_item->usage_minimum = 0;
					report_item->usage_maximum = 0;
					report_item->designator_index = 0;
					report_item->designator_minimum = 0;
					report_item->designator_maximum = 0;
					report_item->string_index = 0;
					report_item->string_minimum = 0;
					report_item->string_maximum = 0;

					break;

				case USB_HID_RESET_OFFSET:
					offset_input = 0;
					offset_output = 0;
					offset_feature = 0;
					break;

				case USB_HID_REPORT_TAG_PUSH:
					// push current state to stack
					new_report_item = usb_hid_report_item_clone(report_item);
					usb_hid_report_path_t *tmp_path = usb_hid_report_path_clone(usage_path);
					new_report_item->usage_path = tmp_path; 

					list_prepend (&new_report_item->link, &stack);
					break;
				case USB_HID_REPORT_TAG_POP:
					// restore current state from stack
					if(list_empty (&stack)) {
						return EINVAL;
					}
					free(report_item);
						
					report_item = list_get_instance(stack.next, usb_hid_report_item_t, link);
					
					usb_hid_report_usage_path_t *tmp_usage_path;
					tmp_usage_path = list_get_instance(report_item->usage_path->link.prev, usb_hid_report_usage_path_t, link);
					
					usb_hid_report_set_last_item(usage_path, tmp_usage_path->usage_page, tmp_usage_path->usage);

					usb_hid_report_path_free(report_item->usage_path);
					list_initialize(&report_item->usage_path->link);
					list_remove (stack.next);
					
					break;
					
				default:
					// nothing special to do					
					break;
			}

			/* jump over the processed block */
			i += 1 + USB_HID_ITEM_SIZE(data[i]);
		}
		else{
			// TBD
			i += 3 + USB_HID_ITEM_SIZE(data[i+1]);
		}
		

	}
	
	return EOK;
}


/**
 * Parse one tag of the report descriptor
 *
 * @param Tag to parse
 * @param Report descriptor buffer
 * @param Size of data belongs to this tag
 * @param Current report item structe
 * @return Code of action to be done next
 */
int usb_hid_report_parse_tag(uint8_t tag, uint8_t class, const uint8_t *data, size_t item_size,
                             usb_hid_report_item_t *report_item, usb_hid_report_path_t *usage_path)
{	
	int ret;
	
	switch(class){
		case USB_HID_TAG_CLASS_MAIN:

			if((ret=usb_hid_report_parse_main_tag(tag,data,item_size,report_item, usage_path)) == EOK) {
				return USB_HID_NEW_REPORT_ITEM;
			}
			else {
				/*TODO process the error */
				return ret;
			   }
			break;

		case USB_HID_TAG_CLASS_GLOBAL:	
			return usb_hid_report_parse_global_tag(tag,data,item_size,report_item, usage_path);
			break;

		case USB_HID_TAG_CLASS_LOCAL:			
			return usb_hid_report_parse_local_tag(tag,data,item_size,report_item, usage_path);
			break;
		default:
			return USB_HID_NO_ACTION;
	}
}

/**
 * Parse main tags of report descriptor
 *
 * @param Tag identifier
 * @param Data buffer
 * @param Length of data buffer
 * @param Current state table
 * @return Error code
 */

int usb_hid_report_parse_main_tag(uint8_t tag, const uint8_t *data, size_t item_size,
                             usb_hid_report_item_t *report_item, usb_hid_report_path_t *usage_path)
{		
	switch(tag)
	{
		case USB_HID_REPORT_TAG_INPUT:
		case USB_HID_REPORT_TAG_OUTPUT:
		case USB_HID_REPORT_TAG_FEATURE:
			report_item->item_flags = *data;			
			return EOK;			
			break;
			
		case USB_HID_REPORT_TAG_COLLECTION:
			// TODO usage_path->flags = *data;
			usb_hid_report_path_append_item(usage_path, report_item->usage_page, report_item->usages[report_item->usages_count-1]);						
			return USB_HID_NO_ACTION;
			break;
			
		case USB_HID_REPORT_TAG_END_COLLECTION:
			usb_hid_report_remove_last_item(usage_path);
			return USB_HID_NO_ACTION;
			break;
		default:
			return USB_HID_NO_ACTION;
	}

	return EOK;
}

/**
 * Parse global tags of report descriptor
 *
 * @param Tag identifier
 * @param Data buffer
 * @param Length of data buffer
 * @param Current state table
 * @return Error code
 */
int usb_hid_report_parse_global_tag(uint8_t tag, const uint8_t *data, size_t item_size,
                             usb_hid_report_item_t *report_item, usb_hid_report_path_t *usage_path)
{
	// TODO take care about the bit length of data
	switch(tag)
	{
		case USB_HID_REPORT_TAG_USAGE_PAGE:
			report_item->usage_page = usb_hid_report_tag_data_int32(data, item_size);
			break;
		case USB_HID_REPORT_TAG_LOGICAL_MINIMUM:
			report_item->logical_minimum = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_LOGICAL_MAXIMUM:
			report_item->logical_maximum = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_PHYSICAL_MINIMUM:
			report_item->physical_minimum = usb_hid_report_tag_data_int32(data,item_size);
			break;			
		case USB_HID_REPORT_TAG_PHYSICAL_MAXIMUM:
			report_item->physical_maximum = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_UNIT_EXPONENT:
			report_item->unit_exponent = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_UNIT:
			report_item->unit = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_REPORT_SIZE:
			report_item->size = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_REPORT_COUNT:
			report_item->count = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_REPORT_ID:
			report_item->id = usb_hid_report_tag_data_int32(data,item_size);
			return USB_HID_RESET_OFFSET;
			break;
		case USB_HID_REPORT_TAG_PUSH:
		case USB_HID_REPORT_TAG_POP:
			/* 
			 * stack operations are done in top level parsing
			 * function
			 */
			return tag;
			break;
			
		default:
			return USB_HID_NO_ACTION;
	}
	
	return EOK;
}

/**
 * Parse local tags of report descriptor
 *
 * @param Tag identifier
 * @param Data buffer
 * @param Length of data buffer
 * @param Current state table
 * @return Error code
 */
int usb_hid_report_parse_local_tag(uint8_t tag, const uint8_t *data, size_t item_size,
                             usb_hid_report_item_t *report_item, usb_hid_report_path_t *usage_path)
{
	switch(tag)
	{
		case USB_HID_REPORT_TAG_USAGE:
			report_item->usages[report_item->usages_count++] = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_USAGE_MINIMUM:
			if (item_size == 3) {
				// usage extended usages
				report_item->extended_usage_page = (usb_hid_report_tag_data_int32(data,item_size) & 0xFF00) >> 16; 
				report_item->usage_minimum = usb_hid_report_tag_data_int32(data,item_size) & 0xFF;
			}
			else {
				report_item->usage_minimum = usb_hid_report_tag_data_int32(data,item_size);
			}
			break;
		case USB_HID_REPORT_TAG_USAGE_MAXIMUM:
			if (item_size == 3) {
				// usage extended usages
				report_item->extended_usage_page = (usb_hid_report_tag_data_int32(data,item_size) & 0xFF00) >> 16; 
				report_item->usage_maximum = usb_hid_report_tag_data_int32(data,item_size) & 0xFF;
			}
			else {
				report_item->usage_maximum = usb_hid_report_tag_data_int32(data,item_size);
			}
			break;
		case USB_HID_REPORT_TAG_DESIGNATOR_INDEX:
			report_item->designator_index = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_DESIGNATOR_MINIMUM:
			report_item->designator_minimum = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_DESIGNATOR_MAXIMUM:
			report_item->designator_maximum = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_STRING_INDEX:
			report_item->string_index = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_STRING_MINIMUM:
			report_item->string_minimum = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_STRING_MAXIMUM:
			report_item->string_maximum = usb_hid_report_tag_data_int32(data,item_size);
			break;			
		case USB_HID_REPORT_TAG_DELIMITER:
			//report_item->delimiter = usb_hid_report_tag_data_int32(data,item_size);
			//TODO: 
			//	DELIMITER STUFF
			break;
		
		default:
			return USB_HID_NO_ACTION;
	}
	
	return EOK;
}

/**
 * Converts raw data to int32 (thats the maximum length of short item data)
 *
 * @param Data buffer
 * @param Size of buffer
 * @return Converted int32 number
 */
int32_t usb_hid_report_tag_data_int32(const uint8_t *data, size_t size)
{
	unsigned int i;
	int32_t result;

	result = 0;
	for(i=0; i<size; i++) {
		result = (result | (data[i]) << (i*8));
	}

	return result;
}



/**
 * Prints content of given list of report items.
 *
 * @param List of report items (usb_hid_report_item_t)
 * @return void
 */
void usb_hid_descriptor_print_list(link_t *head)
{
	usb_hid_report_field_t *report_item;
	link_t *item;


	if(head == NULL || list_empty(head)) {
	    usb_log_debug("\tempty\n");
	    return;
	}
        
	for(item = head->next; item != head; item = item->next) {
                
		report_item = list_get_instance(item, usb_hid_report_field_t, link);

		usb_log_debug("\t\tOFFSET: %X\n", report_item->offset);
		usb_log_debug("\t\tSIZE: %X\n", report_item->size);				
		usb_log_debug("\t\tLOGMIN: %X\n", report_item->logical_minimum);
		usb_log_debug("\t\tLOGMAX: %X\n", report_item->logical_maximum);		
		usb_log_debug("\t\tPHYMIN: %X\n", report_item->physical_minimum);		
		usb_log_debug("\t\tPHYMAX: %X\n", report_item->physical_maximum);				
		usb_log_debug("\t\ttUSAGEMIN: %X\n", report_item->usage_minimum);
		usb_log_debug("\t\tUSAGEMAX: %X\n", report_item->usage_maximum);

		usb_log_debug("\t\ttUSAGE: %X\n", report_item->usage);
		usb_log_debug("\t\tUSAGE PAGE: %X\n", report_item->usage_page);
						
		usb_log_debug("\n");		

	}


}
/**
 * Prints content of given report descriptor in human readable format.
 *
 * @param parser Parsed descriptor to print
 * @return void
 */
void usb_hid_descriptor_print(usb_hid_report_t *report)
{
	if(report == NULL) {
		return;
	}

	link_t *report_it = report->reports.next;
	usb_hid_report_description_t *report_des;

	while(report_it != &report->reports) {
		report_des = list_get_instance(report_it, usb_hid_report_description_t, link);
		usb_log_debug("Report ID: %d\n", report_des->report_id);
		usb_log_debug("\tType: %d\n", report_des->type);
		usb_log_debug("\tLength: %d\n", report_des->bit_length);		
		usb_log_debug("\tItems: %d\n", report_des->item_length);		

		usb_hid_descriptor_print_list(&report_des->report_items);


		link_t *path_it = report->collection_paths.next;
		while(path_it != &report->collection_paths) {
			usb_hid_print_usage_path (list_get_instance(path_it, usb_hid_report_path_t, link));
			path_it = path_it->next;
		}
		
		report_it = report_it->next;
	}
}

/**
 * Releases whole linked list of report items
 *
 * @param head Head of list of report descriptor items (usb_hid_report_item_t)
 * @return void
 */
void usb_hid_free_report_list(link_t *head)
{
	return; 
	
	usb_hid_report_item_t *report_item;
	link_t *next;
	
	if(head == NULL || list_empty(head)) {		
	    return;
	}
	
	next = head->next;
	while(next != head) {
	
	    report_item = list_get_instance(next, usb_hid_report_item_t, link);

		while(!list_empty(&report_item->usage_path->link)) {
			usb_hid_report_remove_last_item(report_item->usage_path);
		}

		
	    next = next->next;
	    
	    free(report_item);
	}
	
	return;
	
}

/** Frees the HID report descriptor parser structure 
 *
 * @param parser Opaque HID report parser structure
 * @return void
 */
void usb_hid_free_report(usb_hid_report_t *report)
{
	if(report == NULL){
		return;
	}

	// free collection paths
	usb_hid_report_path_t *path;
	while(!list_empty(&report->collection_paths)) {
		path = list_get_instance(report->collection_paths.next, usb_hid_report_path_t, link);
		usb_hid_report_path_free(path);		
	}
	
	// free report items
	usb_hid_report_description_t *report_des;
	usb_hid_report_field_t *field;
	while(!list_empty(&report->reports)) {
		report_des = list_get_instance(report->reports.next, usb_hid_report_description_t, link);
		list_remove(&report_des->link);
		
		while(!list_empty(&report_des->report_items)) {
			field = list_get_instance(report_des->report_items.next, usb_hid_report_field_t, link);
			list_remove(&field->link);

			free(field);
		}
		
		free(report_des);
	}
	
	return;
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
    const uint8_t *data, size_t size)
{
	link_t *list_item;
	usb_hid_report_field_t *item;

	uint8_t report_id = 0;
	usb_hid_report_description_t *report_des;
	usb_hid_report_type_t type = USB_HID_REPORT_TYPE_INPUT;

	if(report == NULL) {
		return EINVAL;
	}

	if(report->use_report_ids != 0) {
		report_id = data[0];
	}

	report_des = usb_hid_report_find_description(report, report_id, type);

	/* read data */
	list_item = report_des->report_items.next;	   
	while(list_item != &(report_des->report_items)) {

		item = list_get_instance(list_item, usb_hid_report_field_t, link);

		if(!USB_HID_ITEM_FLAG_CONSTANT(item->item_flags)) {
			
			if((USB_HID_ITEM_FLAG_VARIABLE(item->item_flags) == 0) ||
			   ((item->usage_minimum == 0) && (item->usage_maximum == 0))) {

				// variable item
				item->value = usb_hid_translate_data(item, data,0);

				// array item ???
				if(!((item->usage_minimum == 0) && (item->usage_maximum == 0))) {
					item->usage = item->value + item->usage_minimum;
				}
			}
			else {
				// bitmapa
				// TODO: overit jestli vraci hodnoty podle phy min/max 
				item->value = usb_hid_translate_data(item, data, 0);
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
int usb_hid_translate_data(usb_hid_report_field_t *item, const uint8_t *data, size_t j)
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

	if((item->physical_minimum == 0) && (item->physical_maximum == 0)) {
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

	offset = item->offset + (j * item->size);
	
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

	if(!(item->logical_minimum >= 0 && item->logical_maximum >= 0)){
		value = (int32_t)value;
	}
	else {
		value = (uint32_t)value;
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
			if(usb_hid_report_compare_usage_path (path, field->collection_path, flags) == EOK) {
				ret++;
			}
		}
		
		field_it = field_it->next;
	}

	return ret;
	}


/**
 * Appends one item (couple of usage_path and usage) into the usage path
 * structure
 *
 * @param usage_path Usage path structure
 * @param usage_page Usage page constant
 * @param usage Usage constant
 * @return Error code
 */
int usb_hid_report_path_append_item(usb_hid_report_path_t *usage_path, 
                                    int32_t usage_page, int32_t usage)
{	
	usb_hid_report_usage_path_t *item;

	if(!(item=malloc(sizeof(usb_hid_report_usage_path_t)))) {
		return ENOMEM;
	}
	list_initialize(&item->link);

	item->usage = usage;
	item->usage_page = usage_page;
	item->flags = 0;
	
	list_append (&item->link, &usage_path->link);
	usage_path->depth++;
	return EOK;
}

/**
 * Removes last item from the usage path structure
 * @param usage_path 
 * @return void
 */
void usb_hid_report_remove_last_item(usb_hid_report_path_t *usage_path)
{
	usb_hid_report_usage_path_t *item;
	
	if(!list_empty(&usage_path->link)){
		item = list_get_instance(usage_path->link.prev, usb_hid_report_usage_path_t, link);		
		list_remove(usage_path->link.prev);
		usage_path->depth--;
		free(item);
	}
}

/**
 * Nulls last item of the usage path structure.
 *
 * @param usage_path
 * @return void
 */
void usb_hid_report_null_last_item(usb_hid_report_path_t *usage_path)
{
	usb_hid_report_usage_path_t *item;
	
	if(!list_empty(&usage_path->link)){	
		item = list_get_instance(usage_path->link.prev, usb_hid_report_usage_path_t, link);
		memset(item, 0, sizeof(usb_hid_report_usage_path_t));
	}
}

/**
 * Modifies last item of usage path structure by given usage page or usage
 *
 * @param usage_path Opaque usage path structure
 * @param tag Class of currently processed tag (Usage page tag falls into Global
 * class but Usage tag into the Local)
 * @param data Value of the processed tag
 * @return void
 */
void usb_hid_report_set_last_item(usb_hid_report_path_t *usage_path, int32_t tag, int32_t data)
{
	usb_hid_report_usage_path_t *item;
	
	if(!list_empty(&usage_path->link)){	
		item = list_get_instance(usage_path->link.prev, usb_hid_report_usage_path_t, link);

		switch(tag) {
			case USB_HID_TAG_CLASS_GLOBAL:
				item->usage_page = data;
				break;
			case USB_HID_TAG_CLASS_LOCAL:
				item->usage = data;
				break;
		}
	}
	
}


void usb_hid_print_usage_path(usb_hid_report_path_t *path)
{
	usb_log_debug("USAGE_PATH FOR RId(%d):\n", path->report_id);
	usb_log_debug("\tLENGTH: %d\n", path->depth);

	link_t *item = path->link.next;
	usb_hid_report_usage_path_t *path_item;
	while(item != &path->link) {

		path_item = list_get_instance(item, usb_hid_report_usage_path_t, link);
		usb_log_debug("\tUSAGE_PAGE: %X\n", path_item->usage_page);
		usb_log_debug("\tUSAGE: %X\n", path_item->usage);
		usb_log_debug("\tFLAGS: %d\n", path_item->flags);		
		
		item = item->next;
	}
}

/**
 * Compares two usage paths structures
 *
 * @param report_path usage path structure to compare
 * @param path usage patrh structure to compare
 * @param flags Flags determining the mode of comparison
 * @return EOK if both paths are identical, non zero number otherwise
 */
int usb_hid_report_compare_usage_path(usb_hid_report_path_t *report_path, 
                                      usb_hid_report_path_t *path,
                                      int flags)
{
	usb_hid_report_usage_path_t *report_item;
	usb_hid_report_usage_path_t *path_item;

	link_t *report_link;
	link_t *path_link;

	int only_page;

	if(report_path->report_id != path->report_id) {
		return 1;
	}

	if(path->depth == 0){
		return EOK;
	}

	//usb_log_debug("---------- PATH COMPARISON ----------\n\n");
	//usb_hid_print_usage_path(report_path);
	//usb_hid_print_usage_path(path);
	

	if((only_page = flags & USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY) != 0){
		flags -= USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY;
	}
	
	switch(flags){
		/* path must be completly identical */
		case USB_HID_PATH_COMPARE_STRICT:
				if(report_path->depth != path->depth){
					return 1;
				}

				report_link = report_path->link.next;
				path_link = path->link.next;
			
				while((report_link != &report_path->link) && (path_link != &path->link)) {
					report_item = list_get_instance(report_link, usb_hid_report_usage_path_t, link);
					path_item = list_get_instance(path_link, usb_hid_report_usage_path_t, link);		

					if((report_item->usage_page != path_item->usage_page) || 
					   ((only_page == 0) && (report_item->usage != path_item->usage))) {
						   return 1;
					} else {
						report_link = report_link->next;
						path_link = path_link->next;			
					}
			
				}

				if((report_link == &report_path->link) && (path_link == &path->link)) {
					return EOK;
				}
				else {
					return 1;
				}						
			break;

		/* compare with only the end of path*/
		case USB_HID_PATH_COMPARE_END:
				report_link = report_path->link.prev;
				path_link = path->link.prev;

				if(list_empty(&path->link)){
					return EOK;
				}
			
				while((report_link != &report_path->link) && (path_link != &path->link)) {
					report_item = list_get_instance(report_link, usb_hid_report_usage_path_t, link);
					path_item = list_get_instance(path_link, usb_hid_report_usage_path_t, link);		

					if((report_item->usage_page != path_item->usage_page) || 
					   ((only_page == 0) && (report_item->usage != path_item->usage))) {
						   return 1;
					} else {
						report_link = report_link->prev;
						path_link = path_link->prev;			
					}
			
				}

				if(path_link == &path->link) {
					return EOK;
				}
				else {
					return 1;
				}						
			
			break;

		default:
			return EINVAL;
	}
	
	
	
	
}

/**
 * Allocates and initializes new usage path structure.
 *
 * @return Initialized usage path structure
 */
usb_hid_report_path_t *usb_hid_report_path(void)
{
	usb_hid_report_path_t *path;
	path = malloc(sizeof(usb_hid_report_path_t));
	if(path == NULL){
		return NULL;
	}
	else {
		path->depth = 0;
		path->report_id = 0;
		list_initialize(&path->link);
		return path;
	}
}

/**
 * Releases given usage path structure.
 *
 * @param path usage path structure to release
 * @return void
 */
void usb_hid_report_path_free(usb_hid_report_path_t *path)
{
	while(!list_empty(&path->link)){
		usb_hid_report_remove_last_item(path);
	}

	list_remove(&path->link);
	free(path);
}


/**
 * Clone content of given usage path to the new one
 *
 * @param usage_path Usage path structure to clone
 * @return New copy of given usage path structure
 */
usb_hid_report_path_t *usb_hid_report_path_clone(usb_hid_report_path_t *usage_path)
{
	link_t *path_link;
	usb_hid_report_usage_path_t *path_item;
	usb_hid_report_usage_path_t *new_path_item;
	usb_hid_report_path_t *new_usage_path = usb_hid_report_path ();

	if(new_usage_path == NULL){
		return NULL;
	}
	
	if(list_empty(&usage_path->link)){
		return new_usage_path;
	}

	path_link = usage_path->link.next;
	while(path_link != &usage_path->link) {
		path_item = list_get_instance(path_link, usb_hid_report_usage_path_t, link);
		new_path_item = malloc(sizeof(usb_hid_report_usage_path_t));
		if(new_path_item == NULL) {
			return NULL;
		}

		list_initialize (&new_path_item->link);
		new_path_item->usage_page = path_item->usage_page;
		new_path_item->usage = path_item->usage;		
		new_path_item->flags = path_item->flags;		
		
		list_append(&new_path_item->link, &new_usage_path->link);
		new_usage_path->depth++;

		path_link = path_link->next;
	}

	return new_usage_path;
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
		if((report_des->report_id == report_id) && (report_des->type = USB_HID_REPORT_TYPE_OUTPUT)){
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
		// TODO: bacha na porovnani - posledni prvek v ceste uz jsou usage/page z inputu a ne kolekce!!!
		if(usb_hid_report_compare_usage_path (field->collection_path, path, flags) == EOK) {
			ret++;
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

			if((USB_HID_ITEM_FLAG_VARIABLE(report_item->item_flags) == 0) ||
				((report_item->usage_minimum == 0) && (report_item->usage_maximum == 0))) {
					
				// variable item
				value = usb_hid_translate_data_reverse(report_item, report_item->value);
				offset = report_item->offset;
				length = report_item->size;
			}
			else {
				//bitmap
				value += usb_hid_translate_data_reverse(report_item, report_item->value);
				offset = report_item->offset;
				length = report_item->size;
			}

			if((offset/8) == ((offset+length-1)/8)) {
				// je to v jednom bytu
				if(((size_t)(offset/8) >= size) || ((size_t)(offset+length-1)/8) >= size) {
					break; // TODO ErrorCode
				}

				size_t shift = offset%8;

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

	if((USB_HID_ITEM_FLAG_VARIABLE(item->item_flags) == 0)) {

		// variable item
		if((item->physical_minimum == 0) && (item->physical_maximum == 0)) {
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


	return (uint32_t)ret;
}

/**
 * Sets report id in usage path structure
 *
 * @param path Usage path structure
 * @param report_id Report id to set
 * @return Error code
 */
int usb_hid_report_path_set_report_id(usb_hid_report_path_t *path, uint8_t report_id)
{
	if(path == NULL){
		return EINVAL;
	}

	path->report_id = report_id;
	return EOK;
}

/**
 * Clones given report item structure and returns the new one
 *
 * @param item Report item structure to clone
 * @return Clonned item
 */
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


/**
 *
 *
 *
 *
 *
 */
int usb_hid_report_output_set_data(usb_hid_report_t *report, 
                                   usb_hid_report_path_t *path, int flags, 
                                  int *data, size_t data_size)
{
	size_t data_idx = 0;
	if(report == NULL){
		return EINVAL;
	}

	usb_hid_report_description_t *report_des;
	report_des = usb_hid_report_find_description (report, path->report_id, 
	                                              USB_HID_REPORT_TYPE_OUTPUT);
	if(report_des == NULL){
		return EINVAL;
	}

	usb_hid_report_field_t *field;
	link_t *field_it = report_des->report_items.next;	
	while(field_it != &report_des->report_items){

		field = list_get_instance(field_it, usb_hid_report_field_t, link);		
		if(USB_HID_ITEM_FLAG_CONSTANT(field->item_flags) == 0) {
			if(usb_hid_report_compare_usage_path (path, field->collection_path, 
		                                      flags) == EOK) {

				if(data_idx < data_size) {
					field->value = data[data_idx++];
				}
				else {
					field->value = 0;
				}
			}
		}
		
		field_it = field_it->next;
	}

	return EOK;
}

/**
 * @}
 */
