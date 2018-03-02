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
 * HID report descriptor and report data parser implementation.
 */
#include <usb/hid/hidparser.h>
#include <errno.h>
#include <stdio.h>
#include <mem.h>
#include <usb/debug.h>
#include <assert.h>
#include <stdlib.h>


/*
 * Constants defining current parsing mode for correct parsing of the set of
 * local tags (usage) enclosed in delimter tags.
 */
/**
 * Second delimiter tag was read. The set of local items (usage) ended.
 */
#define OUTSIDE_DELIMITER_SET	0

/**
 * First delimiter tag was read. The set of local items (usage) started.
 */
#define START_DELIMITER_SET	1

/**
 * Parser is in the set of local items.
 */
#define INSIDE_DELIMITER_SET	2



/** The new report item flag. Used to determine when the item is completly
 * configured and should be added to the report structure
 */
#define USB_HID_NEW_REPORT_ITEM 1

/** No special action after the report descriptor tag is processed should be
 * done
 */
#define USB_HID_NO_ACTION	2

#define USB_HID_RESET_OFFSET	3

#define USB_HID_INVALID                 -98
/** Unknown tag was founded in report descriptor data*/
#define USB_HID_UNKNOWN_TAG		-99


/**
 * Checks if given collection path is already present in report structure and
 * inserts it if not.
 *
 * @param report Report structure
 * @param cmp_path The collection path
 * @return Pointer to the result collection path in report structure.
 * @retval NULL If some error occurs
 */
usb_hid_report_path_t *usb_hid_report_path_try_insert(usb_hid_report_t *report,
    usb_hid_report_path_t *cmp_path)
{
	link_t *path_it = report->collection_paths.head.next;
	usb_hid_report_path_t *path = NULL;

	if((report == NULL) || (cmp_path == NULL)) {
		return NULL;
	}

	while(path_it != &report->collection_paths.head) {
		path = list_get_instance(path_it, usb_hid_report_path_t,
				cpath_link);

		if(usb_hid_report_compare_usage_path(path, cmp_path,
					USB_HID_PATH_COMPARE_STRICT) == 0){
			break;
		}
		path_it = path_it->next;
	}
	if(path_it == &report->collection_paths.head) {
		path = usb_hid_report_path_clone(cmp_path);
		if(path == NULL) {
			return NULL;
		}
		list_append(&path->cpath_link, &report->collection_paths);
		report->collection_paths_count++;

		return path;
	}
	else {
		return list_get_instance(path_it, usb_hid_report_path_t,
				cpath_link);
	}
}


/**
 * Initialize the report descriptor parser structure
 *
 * @param parser Report descriptor parser structure
 * @return Error code
 * @retval EINVAL If no report structure was given
 * @retval EOK If report structure was successfully initialized
 */
errno_t usb_hid_report_init(usb_hid_report_t *report)
{
	if (report == NULL) {
		return EINVAL;
	}

	memset(report, 0, sizeof(usb_hid_report_t));
	list_initialize(&report->reports);
	list_initialize(&report->collection_paths);

	report->use_report_ids = 0;
    return EOK;
}



/**
 *
 *
 * @param report Report structure in which the new report items should be
 * 		 stored
 * @param report_item Current report descriptor's parsing state table
 * @return Error code
 * @retval EOK If all fields were successfully append to report
 * @retval EINVAL If invalid parameters (NULL) was given
 * @retval ENOMEM If there is no memmory to store new report description
 *
 */
errno_t usb_hid_report_append_fields(usb_hid_report_t *report,
		usb_hid_report_item_t *report_item) {

	usb_hid_report_field_t *field;
	int i;

	uint32_t *usages;
	int usages_used=0;

	if((report == NULL) || (report_item == NULL)) {
		return EINVAL;
	}

	if(report_item->usages_count > 0){
		usages = malloc(sizeof(uint32_t) * report_item->usages_count);
		memcpy(usages, report_item->usages, sizeof(int32_t) *
				report_item->usages_count);
	}
	else {
		usages = NULL;
	}

	usb_hid_report_path_t *path = report_item->usage_path;
	for(i=0; i<report_item->count; i++){

		field = malloc(sizeof(usb_hid_report_field_t));
		if(field == NULL) {
			return ENOMEM;
		}

		memset(field, 0, sizeof(usb_hid_report_field_t));
		link_initialize(&field->ritems_link);

		/* fill the attributes */
		field->logical_minimum = report_item->logical_minimum;
		field->logical_maximum = report_item->logical_maximum;
		field->physical_minimum = report_item->physical_minimum;
		field->physical_maximum = report_item->physical_maximum;

		if(USB_HID_ITEM_FLAG_VARIABLE(report_item->item_flags) == 0){
			/*
			Store usage array. The Correct Usage Page and Usage is
			depending on data in report and will be filled later
			*/
			field->usage = 0;
			field->usage_page = 0; //report_item->usage_page;

			field->usages_count = report_item->usages_count;
			field->usages = usages;
			usages_used = 1;
		}
		else {

			/* Fill the correct Usage and Usage Page */
			int32_t usage;
			if(i < report_item->usages_count) {
				usage = report_item->usages[i];
			}
			else {
				usage = report_item->usages[
					report_item->usages_count- 1];
			}

			if(USB_HID_IS_EXTENDED_USAGE(usage)){
				field->usage = USB_HID_EXTENDED_USAGE(usage);
				field->usage_page =
					USB_HID_EXTENDED_USAGE_PAGE(usage);
			}
			else {
				// should not occur
				field->usage = usage;
				field->usage_page = report_item->usage_page;
			}
		}

		usb_hid_report_set_last_item(path, USB_HID_TAG_CLASS_GLOBAL,
				field->usage_page);
		usb_hid_report_set_last_item(path, USB_HID_TAG_CLASS_LOCAL,
				field->usage);

		field->collection_path =
			usb_hid_report_path_try_insert(report, path);

		field->size = report_item->size;

		field->offset = report_item->offset + (i * report_item->size);

		if(report->use_report_ids != 0) {
			field->offset += 8;
			report->use_report_ids = 1;
		}

		field->item_flags = report_item->item_flags;

		/* find the right report list*/
		usb_hid_report_description_t *report_des;
		report_des = usb_hid_report_find_description(report,
			report_item->id, report_item->type);

		if(report_des == NULL){
			report_des = malloc(
				sizeof(usb_hid_report_description_t));
			if(report_des == NULL) {
				return ENOMEM;
			}

			memset(report_des, 0,
				sizeof(usb_hid_report_description_t));

			report_des->type = report_item->type;
			report_des->report_id = report_item->id;
			if(report_des->report_id != 0) {
				/* set up the bit length by report_id field */
				report_des->bit_length = 8;
			}

			link_initialize (&report_des->reports_link);
			list_initialize (&report_des->report_items);

			list_append(&report_des->reports_link, &report->reports);
			report->report_count++;
		}

		/* append this field to the end of founded report list */
		list_append(&field->ritems_link, &report_des->report_items);

		/* update the sizes */
		report_des->bit_length += field->size;
		report_des->item_length++;

	}

	// free only when not used!!!
	if(usages && usages_used == 0) {
		free(usages);
	}

	return EOK;
}

/**
 * Finds description of report with given report_id and of given type in
 * opaque report structure.
 *
 * @param report Opaque structure containing the parsed report descriptor
 * @param report_id ReportId of report we are searching
 * @param type Type of report we are searching
 * @return Pointer to the particular report description
 * @retval NULL If no description is founded
 */
usb_hid_report_description_t * usb_hid_report_find_description(
		const usb_hid_report_t *report, uint8_t report_id,
		usb_hid_report_type_t type) {

	if(report == NULL) {
		return NULL;
	}

	list_foreach(report->reports, reports_link,
	    usb_hid_report_description_t, report_des) {
		// if report id not set, return the first of the type
		if(((report_des->report_id == report_id) || (report_id == 0)) &&
		   (report_des->type == type)) {
			return report_des;
		}
	}

	return NULL;
}


/** Parse HID report descriptor.
 *
 * @param parser Opaque HID report parser structure.
 * @param data Data describing the report.
 * @return Error code.
 * @retval ENOMEM If no more memmory is available
 * @retval EINVAL If invalid data are founded
 * @retval EOK If report descriptor is successfully parsed
 */
errno_t usb_hid_parse_report_descriptor(usb_hid_report_t *report,
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

	link_t *item_link;

	list_t stack;
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
	link_initialize(&(report_item->link));

	/* usage path context initialization */
	if(!(usage_path=usb_hid_report_path())){
		return ENOMEM;
	}
	usb_hid_report_path_append_item(usage_path, 0, 0);

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
				/* store report item to report and create the
				 * new one store current collection path
				 */
				report_item->usage_path = usage_path;

				usb_hid_report_path_set_report_id(
				     report_item->usage_path, report_item->id);

				if(report_item->id != 0){
					report->use_report_ids = 1;
				}

				switch(tag) {
				case USB_HID_REPORT_TAG_INPUT:
					report_item->type =
					    USB_HID_REPORT_TYPE_INPUT;

					report_item->offset = offset_input;
					offset_input += report_item->count *
					    report_item->size;
					break;

				case USB_HID_REPORT_TAG_OUTPUT:
					report_item->type =
					    USB_HID_REPORT_TYPE_OUTPUT;

					report_item->offset = offset_output;
					offset_output += report_item->count *
					    report_item->size;
					break;

				case USB_HID_REPORT_TAG_FEATURE:
					report_item->type =
					    USB_HID_REPORT_TYPE_FEATURE;

					report_item->offset = offset_feature;
					offset_feature += report_item->count *
						report_item->size;
					break;

				default:
					usb_log_debug2(
					    "\tjump over - tag %X\n", tag);
				    	break;
				}

				/*
				 * append new fields to the report structure
				 */
				usb_hid_report_append_fields(report,
				    report_item);

				/* reset local items */
				usb_hid_report_reset_local_items (report_item);
				break;

			case USB_HID_RESET_OFFSET:
				offset_input = 0;
				offset_output = 0;
				offset_feature = 0;
				usb_hid_report_path_set_report_id (usage_path,
				    report_item->id);
				break;

			case USB_HID_REPORT_TAG_PUSH:
				// push current state to stack
				new_report_item = usb_hid_report_item_clone(
				    report_item);

				usb_hid_report_path_t *tmp_path =
				    usb_hid_report_path_clone(usage_path);

				new_report_item->usage_path = tmp_path;

				list_prepend (&new_report_item->link, &stack);
				break;
			case USB_HID_REPORT_TAG_POP:
				// restore current state from stack
				item_link = list_first(&stack);
				if (item_link == NULL) {
					return EINVAL;
				}
				free(report_item);

				report_item = list_get_instance(item_link,
				    usb_hid_report_item_t, link);

				usb_hid_report_usage_path_t *tmp_usage_path;
				tmp_usage_path = list_get_instance(
				    report_item->usage_path->cpath_link.prev,
				    usb_hid_report_usage_path_t, rpath_items_link);

				usb_hid_report_set_last_item(usage_path,
				    USB_HID_TAG_CLASS_GLOBAL, tmp_usage_path->usage_page);

				usb_hid_report_set_last_item(usage_path,
				    USB_HID_TAG_CLASS_LOCAL, tmp_usage_path->usage);

				usb_hid_report_path_free(report_item->usage_path);
				list_remove (item_link);

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
int usb_hid_report_parse_tag(uint8_t tag, uint8_t class, const uint8_t *data,
	size_t item_size, usb_hid_report_item_t *report_item,
	usb_hid_report_path_t *usage_path) {

	int ret;

	switch(class){
	case USB_HID_TAG_CLASS_MAIN:

		if((ret=usb_hid_report_parse_main_tag(tag, data, item_size,
			report_item, usage_path)) == 0) {

			return USB_HID_NEW_REPORT_ITEM;
		}
		else {
			return ret;
		}
		break;

	case USB_HID_TAG_CLASS_GLOBAL:
		return usb_hid_report_parse_global_tag(tag, data, item_size,
			report_item, usage_path);
		break;

	case USB_HID_TAG_CLASS_LOCAL:
		return usb_hid_report_parse_local_tag(tag, data, item_size,
			report_item, usage_path);
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
 * @return 0 or USB_HID_ code
 */

int usb_hid_report_parse_main_tag(uint8_t tag, const uint8_t *data,
	size_t item_size, usb_hid_report_item_t *report_item,
	usb_hid_report_path_t *usage_path)
{
	usb_hid_report_usage_path_t *path_item;

	switch(tag)
	{
	case USB_HID_REPORT_TAG_INPUT:
	case USB_HID_REPORT_TAG_OUTPUT:
	case USB_HID_REPORT_TAG_FEATURE:
		report_item->item_flags = *data;
		return 0;
		break;

	case USB_HID_REPORT_TAG_COLLECTION:

		/* store collection atributes */
		path_item = list_get_instance(list_first(&usage_path->items),
			usb_hid_report_usage_path_t, rpath_items_link);
		path_item->flags = *data;

		/* set last item */
		usb_hid_report_set_last_item(usage_path,
			USB_HID_TAG_CLASS_GLOBAL,
			USB_HID_EXTENDED_USAGE_PAGE(report_item->usages[
				report_item->usages_count-1]));

		usb_hid_report_set_last_item(usage_path,
			USB_HID_TAG_CLASS_LOCAL,
			USB_HID_EXTENDED_USAGE(report_item->usages[
				report_item->usages_count-1]));

		/* append the new one which will be set by common usage/usage
		 * page */
		usb_hid_report_path_append_item(usage_path,
			report_item->usage_page,
			report_item->usages[report_item->usages_count-1]);

		usb_hid_report_reset_local_items (report_item);
		return USB_HID_NO_ACTION;
		break;

	case USB_HID_REPORT_TAG_END_COLLECTION:
		usb_hid_report_remove_last_item(usage_path);
		return USB_HID_NO_ACTION;
		break;

	default:
		return USB_HID_NO_ACTION;
	}

	return 0;
}

/**
 * Parse global tags of report descriptor
 *
 * @param Tag identifier
 * @param Data buffer
 * @param Length of data buffer
 * @param Current state table
 * @return 0 or USB_HID_ code
 */
int usb_hid_report_parse_global_tag(uint8_t tag, const uint8_t *data,
	size_t item_size, usb_hid_report_item_t *report_item,
	usb_hid_report_path_t *usage_path) {

	switch(tag)
	{
	case USB_HID_REPORT_TAG_USAGE_PAGE:
		report_item->usage_page =
			usb_hid_report_tag_data_uint32(data, item_size);
		break;

	case USB_HID_REPORT_TAG_LOGICAL_MINIMUM:
		report_item->logical_minimum = USB_HID_UINT32_TO_INT32(
			usb_hid_report_tag_data_uint32(data,item_size),
		       	item_size * 8);
		break;

	case USB_HID_REPORT_TAG_LOGICAL_MAXIMUM:
		report_item->logical_maximum = USB_HID_UINT32_TO_INT32(
			usb_hid_report_tag_data_uint32(data,item_size),
			item_size * 8);
		break;

	case USB_HID_REPORT_TAG_PHYSICAL_MINIMUM:
		report_item->physical_minimum = USB_HID_UINT32_TO_INT32(
			usb_hid_report_tag_data_uint32(data,item_size),
			item_size * 8);
		break;

	case USB_HID_REPORT_TAG_PHYSICAL_MAXIMUM:
		report_item->physical_maximum = USB_HID_UINT32_TO_INT32(
			usb_hid_report_tag_data_uint32(data,item_size),
			item_size * 8);
		break;

	case USB_HID_REPORT_TAG_UNIT_EXPONENT:
		report_item->unit_exponent = usb_hid_report_tag_data_uint32(
			data,item_size);
		break;

	case USB_HID_REPORT_TAG_UNIT:
		report_item->unit = usb_hid_report_tag_data_uint32(
			data,item_size);
		break;

	case USB_HID_REPORT_TAG_REPORT_SIZE:
		report_item->size = usb_hid_report_tag_data_uint32(
			data,item_size);
		break;

	case USB_HID_REPORT_TAG_REPORT_COUNT:
		report_item->count = usb_hid_report_tag_data_uint32(
			data,item_size);
		break;

	case USB_HID_REPORT_TAG_REPORT_ID:
		report_item->id = usb_hid_report_tag_data_uint32(data,
			item_size);
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

	return 0;
}

/**
 * Parse local tags of report descriptor
 *
 * @param Tag identifier
 * @param Data buffer
 * @param Length of data buffer
 * @param Current state table
 * @return 0 or USB_HID_ code
 */
int usb_hid_report_parse_local_tag(uint8_t tag, const uint8_t *data,
    size_t item_size, usb_hid_report_item_t *report_item,
    usb_hid_report_path_t *usage_path)
{
	int32_t extended_usage;

	switch (tag) {
	case USB_HID_REPORT_TAG_USAGE:
		switch (report_item->in_delimiter) {
		case INSIDE_DELIMITER_SET:
			/*
			 * Nothing to do.
			 * We catch only the first one
			 */
			break;

		case START_DELIMITER_SET:
			report_item->in_delimiter = INSIDE_DELIMITER_SET;
			/* Fallthrough */
		case OUTSIDE_DELIMITER_SET:
			extended_usage = ((report_item->usage_page) << 16);
			extended_usage +=
			    usb_hid_report_tag_data_uint32(data, item_size);

			report_item->usages[report_item->usages_count] =
			    extended_usage;

			report_item->usages_count++;
			break;
		}
		break;

	case USB_HID_REPORT_TAG_USAGE_MINIMUM:
		if (item_size == 3) {
			/* Usage extended usages */
			report_item->extended_usage_page =
			    USB_HID_EXTENDED_USAGE_PAGE(
			    usb_hid_report_tag_data_uint32(data, item_size));


			report_item->usage_minimum =
			    USB_HID_EXTENDED_USAGE(
			    usb_hid_report_tag_data_uint32(data, item_size));
		} else {
			report_item->usage_minimum =
			    usb_hid_report_tag_data_uint32(data, item_size);
		}
		break;

	case USB_HID_REPORT_TAG_USAGE_MAXIMUM:
		if (item_size == 3) {
			if (report_item->extended_usage_page !=
			    USB_HID_EXTENDED_USAGE_PAGE(
			    usb_hid_report_tag_data_uint32(data, item_size))) {
				return USB_HID_INVALID;
			}

			/* Usage extended usages */
			report_item->extended_usage_page =
			    USB_HID_EXTENDED_USAGE_PAGE(
			    usb_hid_report_tag_data_uint32(data,item_size));

			report_item->usage_maximum =
			    USB_HID_EXTENDED_USAGE(
			    usb_hid_report_tag_data_uint32(data,item_size));
		} else {
			report_item->usage_maximum =
			    usb_hid_report_tag_data_uint32(data,item_size);
		}

		/* Put the records into the usages array */
		for (int32_t i = report_item->usage_minimum;
		    i <= report_item->usage_maximum; i++) {

			if (report_item->extended_usage_page) {
				report_item->usages[report_item->usages_count++] =
				    (report_item->extended_usage_page << 16) + i;
			} else {
				report_item->usages[report_item->usages_count++] =
				    (report_item->usage_page << 16) + i;
			}
		}
		report_item->extended_usage_page = 0;

		break;

	case USB_HID_REPORT_TAG_DESIGNATOR_INDEX:
		report_item->designator_index =
		    usb_hid_report_tag_data_uint32(data, item_size);
		break;

	case USB_HID_REPORT_TAG_DESIGNATOR_MINIMUM:
		report_item->designator_minimum =
		    usb_hid_report_tag_data_uint32(data, item_size);
		break;

	case USB_HID_REPORT_TAG_DESIGNATOR_MAXIMUM:
		report_item->designator_maximum =
		    usb_hid_report_tag_data_uint32(data, item_size);
		break;

	case USB_HID_REPORT_TAG_STRING_INDEX:
		report_item->string_index =
		    usb_hid_report_tag_data_uint32(data, item_size);
		break;

	case USB_HID_REPORT_TAG_STRING_MINIMUM:
		report_item->string_minimum =
		    usb_hid_report_tag_data_uint32(data, item_size);
		break;

	case USB_HID_REPORT_TAG_STRING_MAXIMUM:
		report_item->string_maximum =
		    usb_hid_report_tag_data_uint32(data, item_size);
		break;

	case USB_HID_REPORT_TAG_DELIMITER:
		report_item->in_delimiter =
		    usb_hid_report_tag_data_uint32(data, item_size);
		break;

	default:
		return USB_HID_NO_ACTION;
	}

	return 0;
}

/**
 * Converts raw data to uint32 (thats the maximum length of short item data)
 *
 * @param Data buffer
 * @param Size of buffer
 * @return Converted int32 number
 */
uint32_t usb_hid_report_tag_data_uint32(const uint8_t *data, size_t size)
{
	unsigned int i;
	uint32_t result;

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
void usb_hid_descriptor_print_list(list_t *list)
{
	if(list == NULL || list_empty(list)) {
	    usb_log_debug("\tempty");
	    return;
	}

        list_foreach(*list, ritems_link, usb_hid_report_field_t,
    	    report_item) {
		usb_log_debug("\t\tOFFSET: %u", report_item->offset);
		usb_log_debug("\t\tSIZE: %zu", report_item->size);
		usb_log_debug("\t\tLOGMIN: %d",
			report_item->logical_minimum);
		usb_log_debug("\t\tLOGMAX: %d",
			report_item->logical_maximum);
		usb_log_debug("\t\tPHYMIN: %d",
			report_item->physical_minimum);
		usb_log_debug("\t\tPHYMAX: %d",
			report_item->physical_maximum);
		usb_log_debug("\t\ttUSAGEMIN: %X",
			report_item->usage_minimum);
		usb_log_debug("\t\tUSAGEMAX: %X",
			       report_item->usage_maximum);
		usb_log_debug("\t\tUSAGES COUNT: %zu",
			report_item->usages_count);

		usb_log_debug("\t\tVALUE: %X", report_item->value);
		usb_log_debug("\t\ttUSAGE: %X", report_item->usage);
		usb_log_debug("\t\tUSAGE PAGE: %X", report_item->usage_page);

		usb_hid_print_usage_path(report_item->collection_path);
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
	if (report == NULL)
		return;

	list_foreach(report->reports, reports_link,
	    usb_hid_report_description_t, report_des) {
		usb_log_debug("Report ID: %d", report_des->report_id);
		usb_log_debug("\tType: %d", report_des->type);
		usb_log_debug("\tLength: %zu", report_des->bit_length);
		usb_log_debug("\tB Size: %zu",
			usb_hid_report_byte_size(report,
				report_des->report_id,
				report_des->type));
		usb_log_debug("\tItems: %zu", report_des->item_length);

		usb_hid_descriptor_print_list(&report_des->report_items);
	}
}



/** Frees the HID report descriptor parser structure
 *
 * @param parser Opaque HID report parser structure
 * @return void
 */
void usb_hid_report_deinit(usb_hid_report_t *report)
{
	if(report == NULL){
		return;
	}

	// free collection paths
	link_t *path_link;
	usb_hid_report_path_t *path;
	while(!list_empty(&report->collection_paths)) {
		path_link = list_first(&report->collection_paths);
		path = list_get_instance(path_link,
		    usb_hid_report_path_t, cpath_link);

		list_remove(path_link);
		usb_hid_report_path_free(path);
	}

	// free report items
	usb_hid_report_description_t *report_des;
	usb_hid_report_field_t *field;
	while(!list_empty(&report->reports)) {
		report_des = list_get_instance(list_first(&report->reports),
				usb_hid_report_description_t, reports_link);

		list_remove(&report_des->reports_link);

		while(!list_empty(&report_des->report_items)) {
			field = list_get_instance(
			    list_first(&report_des->report_items),
			    usb_hid_report_field_t, ritems_link);

			list_remove(&field->ritems_link);

			free(field);
		}

		free(report_des);
	}

	return;
}


/**
 * @}
 */
