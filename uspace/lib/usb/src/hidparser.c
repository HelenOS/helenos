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

/** */
#define USB_HID_NEW_REPORT_ITEM 1

/** */
#define USB_HID_NO_ACTION		2

/** */
#define USB_HID_UNKNOWN_TAG		-99

/*
 * Private descriptor parser functions
 */
int usb_hid_report_parse_tag(uint8_t tag, uint8_t class, const uint8_t *data, size_t item_size,
                             usb_hid_report_item_t *report_item, usb_hid_report_path_t *usage_path);
int usb_hid_report_parse_main_tag(uint8_t tag, const uint8_t *data, size_t item_size,
                             usb_hid_report_item_t *report_item, usb_hid_report_path_t *usage_path);
int usb_hid_report_parse_global_tag(uint8_t tag, const uint8_t *data, size_t item_size,
                             usb_hid_report_item_t *report_item, usb_hid_report_path_t *usage_path);
int usb_hid_report_parse_local_tag(uint8_t tag, const uint8_t *data, size_t item_size,
                             usb_hid_report_item_t *report_item, usb_hid_report_path_t *usage_path);

void usb_hid_descriptor_print_list(link_t *head);
int usb_hid_report_reset_local_items();
void usb_hid_free_report_list(link_t *head);
usb_hid_report_item_t *usb_hid_report_item_clone(const usb_hid_report_item_t *item);
/*
 * Data translation private functions
 */
int32_t usb_hid_report_tag_data_int32(const uint8_t *data, size_t size);
inline size_t usb_hid_count_item_offset(usb_hid_report_item_t * report_item, size_t offset);
int usb_hid_translate_data(usb_hid_report_item_t *item, const uint8_t *data, size_t j);
int32_t usb_hid_translate_data_reverse(usb_hid_report_item_t *item, int32_t value);
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
int usb_hid_parser_init(usb_hid_report_parser_t *parser)
{
	if(parser == NULL) {
		return EINVAL;
	}

	list_initialize(&(parser->input));
    list_initialize(&(parser->output));
    list_initialize(&(parser->feature));

	list_initialize(&(parser->stack));

	parser->use_report_id = 0;
    return EOK;   
}


/** Parse HID report descriptor.
 *
 * @param parser Opaque HID report parser structure.
 * @param data Data describing the report.
 * @return Error code.
 */
int usb_hid_parse_report_descriptor(usb_hid_report_parser_t *parser, 
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
	usb_hid_report_path_t *tmp_usage_path;

	size_t offset_input=0;
	size_t offset_output=0;
	size_t offset_feature=0;
	

	/* parser structure initialization*/
	if(usb_hid_parser_init(parser) != EOK) {
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
				return EINVAL; // TODO ERROR CODE
			}
			
			tag = USB_HID_ITEM_TAG(data[i]);
			item_size = USB_HID_ITEM_SIZE(data[i]);
			class = USB_HID_ITEM_TAG_CLASS(data[i]);

			usb_log_debug2(
				"i(%u) data(%X) value(%X): TAG %u, class %u, size %u - ", i, 
			    data[i], usb_hid_report_tag_data_int32(data+i+1,item_size), 
			    tag, class, item_size);
			
			ret = usb_hid_report_parse_tag(tag,class,data+i+1,
			                               item_size,report_item, usage_path);
			usb_log_debug2("ret: %u\n", ret);
			switch(ret){
				case USB_HID_NEW_REPORT_ITEM:
					// store report item to report and create the new one
					usb_log_debug("\nNEW REPORT ITEM: %X",ret);

					// store current usage path
					report_item->usage_path = usage_path;
					
					// clone path to the new one
					tmp_usage_path = usb_hid_report_path_clone(usage_path);

					// swap
					usage_path = tmp_usage_path;
					tmp_usage_path = NULL;

					usb_hid_report_path_set_report_id(report_item->usage_path, report_item->id);	
					if(report_item->id != 0){
						parser->use_report_id = 1;
					}
					
					switch(tag) {
						case USB_HID_REPORT_TAG_INPUT:
							report_item->offset = offset_input;
							offset_input += report_item->count * report_item->size;
							usb_log_debug(" - INPUT\n");
							list_append(&(report_item->link), &(parser->input));
							break;
						case USB_HID_REPORT_TAG_OUTPUT:
							report_item->offset = offset_output;
							offset_output += report_item->count * report_item->size;
							usb_log_debug(" - OUTPUT\n");
								list_append(&(report_item->link), &(parser->output));

							break;
						case USB_HID_REPORT_TAG_FEATURE:
							report_item->offset = offset_feature;
							offset_feature += report_item->count * report_item->size;
							usb_log_debug(" - FEATURE\n");
								list_append(&(report_item->link), &(parser->feature));
							break;
						default:
						    usb_log_debug("\tjump over - tag %X\n", tag);
						    break;
					}

					/* clone current state table to the new item */
					if(!(new_report_item = malloc(sizeof(usb_hid_report_item_t)))) {
						return ENOMEM;
					}					
					memcpy(new_report_item,report_item, sizeof(usb_hid_report_item_t));
					link_initialize(&(new_report_item->link));
					
					/* reset local items */
					new_report_item->usage_minimum = 0;
					new_report_item->usage_maximum = 0;
					new_report_item->designator_index = 0;
					new_report_item->designator_minimum = 0;
					new_report_item->designator_maximum = 0;
					new_report_item->string_index = 0;
					new_report_item->string_minimum = 0;
					new_report_item->string_maximum = 0;

					/* reset usage from current usage path */
					usb_hid_report_usage_path_t *path = list_get_instance(&usage_path->link, usb_hid_report_usage_path_t, link);
					path->usage = 0;
					
					report_item = new_report_item;
										
					break;
				case USB_HID_REPORT_TAG_PUSH:
					// push current state to stack
					new_report_item = usb_hid_report_item_clone(report_item);
					list_prepend (&parser->stack, &new_report_item->link);
					
					break;
				case USB_HID_REPORT_TAG_POP:
					// restore current state from stack
					if(list_empty (&parser->stack)) {
						return EINVAL;
					}
					
					report_item = list_get_instance(&parser->stack, usb_hid_report_item_t, link);
					list_remove (parser->stack.next);
					
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
 * Parse input report.
 *
 * @param data Data for report
 * @param size Size of report
 * @param callbacks Callbacks for report actions
 * @param arg Custom arguments
 *
 * @return Error code
 */
int usb_hid_boot_keyboard_input_report(const uint8_t *data, size_t size,
	const usb_hid_report_in_callbacks_t *callbacks, void *arg)
{
	int i;
	usb_hid_report_item_t item;

	/* fill item due to the boot protocol report descriptor */
	// modifier keys are in the first byte
	uint8_t modifiers = data[0];

	item.offset = 2; /* second byte is reserved */
	item.size = 8;
	item.count = 6;
	item.usage_minimum = 0;
	item.usage_maximum = 255;
	item.logical_minimum = 0;
	item.logical_maximum = 255;

	if (size != 8) {
		return -1; //ERANGE;
	}

	uint8_t keys[6];
	for (i = 0; i < item.count; i++) {
		keys[i] = data[i + item.offset];
	}

	callbacks->keyboard(keys, 6, modifiers, arg);
	return EOK;
}

/**
 * Makes output report for keyboard boot protocol
 *
 * @param leds
 * @param output Output report data buffer
 * @param size Size of the output buffer
 * @return Error code
 */
int usb_hid_boot_keyboard_output_report(uint8_t leds, uint8_t *data, size_t size)
{
	if(size != 1){
		return -1;
	}

	/* used only first five bits, others are only padding*/
	*data = leds;
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
			usb_hid_report_path_append_item(usage_path, 0, 0);
						
			return USB_HID_NO_ACTION;
			break;
			
		case USB_HID_REPORT_TAG_END_COLLECTION:
			// TODO
			// znici posledni uroven ve vsech usage paths
			// otazka jestli nema nicit dve, respektive novou posledni vynulovat?
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
			// zmeni to jenom v poslednim poli aktualni usage path
			usb_hid_report_set_last_item(usage_path, USB_HID_TAG_CLASS_GLOBAL,
				usb_hid_report_tag_data_int32(data,item_size));
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
			break;
		case USB_HID_REPORT_TAG_PUSH:
		case USB_HID_REPORT_TAG_POP:
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
			usb_hid_report_set_last_item(usage_path, USB_HID_TAG_CLASS_LOCAL,
				usb_hid_report_tag_data_int32(data,item_size));
			break;
		case USB_HID_REPORT_TAG_USAGE_MINIMUM:
			report_item->usage_minimum = usb_hid_report_tag_data_int32(data,item_size);
			break;
		case USB_HID_REPORT_TAG_USAGE_MAXIMUM:
			report_item->usage_maximum = usb_hid_report_tag_data_int32(data,item_size);
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
			report_item->delimiter = usb_hid_report_tag_data_int32(data,item_size);
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
	usb_hid_report_item_t *report_item;
	usb_hid_report_usage_path_t *path_item;
	link_t *path;
	link_t *item;
	
	if(head == NULL || list_empty(head)) {
	    usb_log_debug("\tempty\n");
	    return;
	}
        
	for(item = head->next; item != head; item = item->next) {
                
		report_item = list_get_instance(item, usb_hid_report_item_t, link);

		usb_log_debug("\tOFFSET: %X\n", report_item->offset);
		usb_log_debug("\tCOUNT: %X\n", report_item->count);
		usb_log_debug("\tSIZE: %X\n", report_item->size);
		usb_log_debug("\tCONSTANT/VAR: %X\n", USB_HID_ITEM_FLAG_CONSTANT(report_item->item_flags));
		usb_log_debug("\tVARIABLE/ARRAY: %X\n", USB_HID_ITEM_FLAG_VARIABLE(report_item->item_flags));
		usb_log_debug("\tUSAGE PATH:\n");

		path = report_item->usage_path->link.next;
		while(path != &report_item->usage_path->link)	{
			path_item = list_get_instance(path, usb_hid_report_usage_path_t, link);
			usb_log_debug("\t\tUSAGE PAGE: %X, USAGE: %X\n", path_item->usage_page, path_item->usage);
			path = path->next;
		}
				
		usb_log_debug("\tLOGMIN: %X\n", report_item->logical_minimum);
		usb_log_debug("\tLOGMAX: %X\n", report_item->logical_maximum);		
		usb_log_debug("\tPHYMIN: %X\n", report_item->physical_minimum);		
		usb_log_debug("\tPHYMAX: %X\n", report_item->physical_maximum);				
		usb_log_debug("\tUSAGEMIN: %X\n", report_item->usage_minimum);
		usb_log_debug("\tUSAGEMAX: %X\n", report_item->usage_maximum);
		
		usb_log_debug("\n");		

	}


}
/**
 * Prints content of given report descriptor in human readable format.
 *
 * @param parser Parsed descriptor to print
 * @return void
 */
void usb_hid_descriptor_print(usb_hid_report_parser_t *parser)
{
	if(parser == NULL) {
		return;
	}
	
	usb_log_debug("INPUT:\n");
	usb_hid_descriptor_print_list(&parser->input);
	
	usb_log_debug("OUTPUT: \n");
	usb_hid_descriptor_print_list(&parser->output);
	
	usb_log_debug("FEATURE:\n");	
	usb_hid_descriptor_print_list(&parser->feature);

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
void usb_hid_free_report_parser(usb_hid_report_parser_t *parser)
{
	if(parser == NULL){
		return;
	}

	parser->use_report_id = 0;

	usb_hid_free_report_list(&parser->input);
	usb_hid_free_report_list(&parser->output);
	usb_hid_free_report_list(&parser->feature);

	return;
}

/** Parse and act upon a HID report.
 *
 * @see usb_hid_parse_report_descriptor
 *
 * @param parser Opaque HID report parser structure.
 * @param data Data for the report.
 * @param callbacks Callbacks for report actions.
 * @param arg Custom argument (passed through to the callbacks).
 * @return Error code.
 */ 
int usb_hid_parse_report(const usb_hid_report_parser_t *parser,  
    const uint8_t *data, size_t size,
    usb_hid_report_path_t *path, int flags,
    const usb_hid_report_in_callbacks_t *callbacks, void *arg)
{
	link_t *list_item;
	usb_hid_report_item_t *item;
	uint8_t *keys;
	uint8_t item_value;
	size_t key_count=0;
	size_t i=0;
	size_t j=0;
	uint8_t report_id = 0;

	if(parser == NULL) {
		return EINVAL;
	}
	
	/* get the size of result array */
	key_count = usb_hid_report_input_length(parser, path, flags);

	if(!(keys = malloc(sizeof(uint8_t) * key_count))){
		return ENOMEM;
	}

	if(parser->use_report_id != 0) {
		report_id = data[0];
		usb_hid_report_path_set_report_id(path, report_id);
	}

	/* read data */
	list_item = parser->input.next;	   
	while(list_item != &(parser->input)) {

		item = list_get_instance(list_item, usb_hid_report_item_t, link);

		if(!USB_HID_ITEM_FLAG_CONSTANT(item->item_flags) && 
		   (usb_hid_report_compare_usage_path(item->usage_path, path, flags) == EOK)) {
			for(j=0; j<(size_t)(item->count); j++) {
				if((USB_HID_ITEM_FLAG_VARIABLE(item->item_flags) == 0) ||
				   ((item->usage_minimum == 0) && (item->usage_maximum == 0))) {
					// variable item
					keys[i++] = usb_hid_translate_data(item, data,j);
				}
				else {
					// bitmapa
					if((item_value = usb_hid_translate_data(item, data, j)) != 0) {
						keys[i++] = (item->count - 1 - j) + item->usage_minimum;
					}
					else {
						keys[i++] = 0;
					}
				}
			}
		}
		list_item = list_item->next;
	}

	callbacks->keyboard(keys, key_count, report_id, arg);
	   
	free(keys);	
	return EOK;
	
}

/**
 * Translate data from the report as specified in report descriptor
 *
 * @param item Report descriptor item with definition of translation
 * @param data Data to translate
 * @param j Index of processed field in report descriptor item
 * @return Translated data
 */
int usb_hid_translate_data(usb_hid_report_item_t *item, const uint8_t *data, size_t j)
{
	int resolution;
	int offset;
	int part_size;
	
	int32_t value;
	int32_t mask;
	const uint8_t *foo;

	// now only common numbers llowed
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
	if(item->id != 0) {
		offset += 8;
		usb_log_debug("MOVED OFFSET BY 1Byte, REPORT_ID(%d)\n", item->id);
	}
	
	// FIXME
	if((offset/8) != ((offset+item->size)/8)) {
		usb_log_debug2("offset %d\n", offset);
		
		part_size = ((offset+item->size)%8);
		usb_log_debug2("part size %d\n",part_size);

		// the higher one
		foo = data+(offset/8);
		mask =  ((1 << (item->size-part_size))-1);
		value = (*foo & mask) << part_size;

		usb_log_debug2("hfoo %x\n", *foo);
		usb_log_debug2("hmaska %x\n",  mask);
		usb_log_debug2("hval %d\n", value);		

		// the lower one
		foo = data+((offset+item->size)/8);
		mask =  ((1 << part_size)-1) << (8-part_size);
		value += ((*foo & mask) >> (8-part_size));

		usb_log_debug2("lfoo %x\n", *foo);
		usb_log_debug2("lmaska %x\n",  mask);
		usb_log_debug2("lval %d\n", ((*foo & mask) >> (8-(item->size-part_size))));		
		usb_log_debug2("val %d\n", value);
		
		
	}
	else {		
		foo = data+(offset/8);
		mask =  ((1 << item->size)-1) << (8-((offset%8)+item->size));
		value = (*foo & mask) >> (8-((offset%8)+item->size));

		usb_log_debug2("offset %d\n", offset);
	
		usb_log_debug2("foo %x\n", *foo);
		usb_log_debug2("maska %x\n",  mask);
		usb_log_debug2("val %d\n", value);				
	}

	usb_log_debug2("---\n\n"); 

	return (int)(((value - item->logical_minimum) / resolution) + item->physical_minimum);
	
}

/**
 *
 *
 * @param parser
 * @param path
 * @param flags
 * @return
 */
size_t usb_hid_report_input_length(const usb_hid_report_parser_t *parser,
	usb_hid_report_path_t *path, int flags)
{	
	size_t ret = 0;
	link_t *item;
	usb_hid_report_item_t *report_item;

	if(parser == NULL) {
		return 0;
	}
	
	item = parser->input.next;
	while(&parser->input != item) {
		report_item = list_get_instance(item, usb_hid_report_item_t, link);
		if(!USB_HID_ITEM_FLAG_CONSTANT(report_item->item_flags) &&
		   (usb_hid_report_compare_usage_path(report_item->usage_path, path, flags) == EOK)) {
			ret += report_item->count;
		}

		item = item->next;
	} 

	return ret;
}


/**
 * 
 * @param usage_path
 * @param usage_page
 * @param usage
 * @return
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
	
	usb_log_debug("Appending usage %d, usage page %d\n", usage, usage_page);
	
	list_append (&usage_path->link, &item->link);
	usage_path->depth++;
	return EOK;
}

/**
 *
 * @param usage_path
 * @return
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
 *
 * @param usage_path
 * @return
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
 *
 * @param usage_path
 * @param tag
 * @param data
 * @return
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

/**
 * 
 *
 * @param report_path
 * @param path
 * @param flags
 * @return
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
 *
 * @return
 */
usb_hid_report_path_t *usb_hid_report_path(void)
{
	usb_hid_report_path_t *path;
	path = malloc(sizeof(usb_hid_report_path_t));
	if(!path){
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
 *
 * @param path
 * @return void
 */
void usb_hid_report_path_free(usb_hid_report_path_t *path)
{
	while(!list_empty(&path->link)){
		usb_hid_report_remove_last_item(path);
	}
}


/**
 * Clone content of given usage path to the new one
 *
 * @param usage_path
 * @return
 */
usb_hid_report_path_t *usb_hid_report_path_clone(usb_hid_report_path_t *usage_path)
{
	usb_hid_report_usage_path_t *path_item;
	link_t *path_link;
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
		usb_hid_report_path_append_item (new_usage_path, path_item->usage_page, path_item->usage);

		path_link = path_link->next;
	}

	return new_usage_path;
}


/*** OUTPUT API **/

/** Allocates output report buffer
 *
 * @param parser
 * @param size
 * @return
 */
uint8_t *usb_hid_report_output(usb_hid_report_parser_t *parser, size_t *size)
{
	if(parser == NULL) {
		*size = 0;
		return NULL;
	}
	
	// read the last output report item
	usb_hid_report_item_t *last;
	link_t *link;

	link = parser->output.prev;
	if(link != &parser->output) {
		last = list_get_instance(link, usb_hid_report_item_t, link);
		*size = (last->offset + (last->size * last->count)) / 8;

		uint8_t *buffer = malloc(sizeof(uint8_t) * (*size));
		memset(buffer, 0, sizeof(uint8_t) * (*size));
		usb_log_debug("output buffer: %s\n", usb_debug_str_buffer(buffer, *size, 0));

		return buffer;
	}
	else {
		*size = 0;		
		return NULL;
	}
}


/** Frees output report buffer
 *
 * @param output Output report buffer
 * @return
 */
void usb_hid_report_output_free(uint8_t *output)

{
	if(output != NULL) {
		free (output);
	}
}

/** Returns size of output for given usage path 
 *
 * @param parser
 * @param path
 * @param flags
 * @return
 */
size_t usb_hid_report_output_size(usb_hid_report_parser_t *parser,
                                  usb_hid_report_path_t *path, int flags)
{
	size_t ret = 0;
	link_t *item;
	usb_hid_report_item_t *report_item;

	if(parser == NULL) {
		return 0;
	}

	item = parser->output.next;
	while(&parser->output != item) {
		report_item = list_get_instance(item, usb_hid_report_item_t, link);
		if(!USB_HID_ITEM_FLAG_CONSTANT(report_item->item_flags) &&
		   (usb_hid_report_compare_usage_path(report_item->usage_path, path, flags) == EOK)) {
			ret += report_item->count;
		}

		item = item->next;
	} 

	return ret;
	
}

/** Updates the output report buffer by translated given data 
 *
 * @param parser
 * @param path
 * @param flags
 * @param buffer
 * @param size
 * @param data
 * @param data_size
 * @return
 */
int usb_hid_report_output_translate(usb_hid_report_parser_t *parser,
                                    usb_hid_report_path_t *path, int flags,
                                    uint8_t *buffer, size_t size,
                                    int32_t *data, size_t data_size)
{
	usb_hid_report_item_t *report_item;
	link_t *item;	
	size_t idx=0;
	int i=0;
	int32_t value=0;
	int offset;
	int length;
	int32_t tmp_value;
	size_t offset_prefix = 0;
	
	if(parser == NULL) {
		return EINVAL;
	}

	if(parser->use_report_id != 0) {
		buffer[0] = path->report_id;
		offset_prefix = 8;
	}

	usb_log_debug("OUTPUT BUFFER: %s\n", usb_debug_str_buffer(buffer,size, 0));
	usb_log_debug("OUTPUT DATA[0]: %d, DATA[1]: %d, DATA[2]: %d\n", data[0], data[1], data[2]);

	item = parser->output.next;	
	while(item != &parser->output) {
		report_item = list_get_instance(item, usb_hid_report_item_t, link);

		for(i=0; i<report_item->count; i++) {

			if(idx >= data_size) {
				break;
			}

			if((USB_HID_ITEM_FLAG_VARIABLE(report_item->item_flags) == 0) ||
				((report_item->usage_minimum == 0) && (report_item->usage_maximum == 0))) {
					
//				// variable item
				value = usb_hid_translate_data_reverse(report_item, data[idx++]);
				offset = report_item->offset + (i * report_item->size) + offset_prefix;
				length = report_item->size;
			}
			else {
				//bitmap
				value += usb_hid_translate_data_reverse(report_item, data[idx++]);
				offset = report_item->offset + offset_prefix;
				length = report_item->size * report_item->count;
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
				// je to ve dvou!! FIXME: melo by to umet delsi jak 2

				// konec prvniho -- dolni x bitu
				tmp_value = value;
				tmp_value = tmp_value & ((1 << (8-(offset%8)))-1);				
				tmp_value = tmp_value << (offset%8);

				uint8_t mask = 0;
				mask = ~(((1 << (8-(offset%8)))-1) << (offset%8));
				buffer[offset/8] = (buffer[offset/8] & mask) | tmp_value;

				// a ted druhej -- hornich length-x bitu
				value = value >> (8 - (offset % 8));
				value = value & ((1 << (length - (8 - (offset % 8)))) - 1);
				
				mask = ((1 << (length - (8 - (offset % 8)))) - 1);
				buffer[(offset+length-1)/8] = (buffer[(offset+length-1)/8] & mask) | value;
			}

		}

		item = item->next;
	}

	usb_log_debug("OUTPUT BUFFER: %s\n", usb_debug_str_buffer(buffer,size, 0));

	return EOK;
}

/**
 *
 * @param item
 * @param value
 * @return
 */
int32_t usb_hid_translate_data_reverse(usb_hid_report_item_t *item, int value)
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


	return ret;
}


int usb_hid_report_path_set_report_id(usb_hid_report_path_t *path, uint8_t report_id)
{
	if(path == NULL){
		return EINVAL;
	}

	path->report_id = report_id;
	return EOK;
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

/**
 * @}
 */
