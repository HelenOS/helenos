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
#include <usb/hid/hidparser.h>
#include <errno.h>
#include <stdio.h>
#include <malloc.h>
#include <mem.h>
#include <usb/debug.h>
#include <assert.h>


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
	
	list_append (&item->link, &usage_path->head);
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
	
	if(!list_empty(&usage_path->head)){
		item = list_get_instance(usage_path->head.prev, 
		                         usb_hid_report_usage_path_t, link);		
		list_remove(usage_path->head.prev);
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
	
	if(!list_empty(&usage_path->head)){	
		item = list_get_instance(usage_path->head.prev, usb_hid_report_usage_path_t, link);
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
void usb_hid_report_set_last_item(usb_hid_report_path_t *usage_path, 
                                  int32_t tag, int32_t data)
{
	usb_hid_report_usage_path_t *item;
	
	if(!list_empty(&usage_path->head)){	
		item = list_get_instance(usage_path->head.prev, 
		                         usb_hid_report_usage_path_t, link);

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

	link_t *item = path->head.next;
	usb_hid_report_usage_path_t *path_item;
	while(item != &path->head) {

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
 *
 * @param report_path usage path structure to compare with @path 
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

	// Empty path match all others
	if(path->depth == 0){
		return EOK;
	}


	if((only_page = flags & USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY) != 0){
		flags -= USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY;
	}
	
	switch(flags){
		/* path is somewhere in report_path */
		case USB_HID_PATH_COMPARE_ANYWHERE:
			if(path->depth != 1){
				return 1;
			}

			// projit skrz cestu a kdyz nekde sedi tak vratim EOK
			// dojduli az za konec tak nnesedi
			report_link = report_path->head.next;
			path_link = path->head.next;
			path_item = list_get_instance(path_link, usb_hid_report_usage_path_t, link);

			while(report_link != &report_path->head) {
				report_item = list_get_instance(report_link, usb_hid_report_usage_path_t, link);
				if(report_item->usage_page == path_item->usage_page){
					if(only_page == 0){
						if(report_item->usage == path_item->usage) {
							return EOK;
						}
					}
					else {
						return EOK;
					}
				}

				report_link = report_link->next;
			}

			return 1;
			break;
		/* the paths must be identical */
		case USB_HID_PATH_COMPARE_STRICT:
				if(report_path->depth != path->depth){
					return 1;
				}
		
		/* path is prefix of the report_path */
		case USB_HID_PATH_COMPARE_BEGIN:
	
				report_link = report_path->head.next;
				path_link = path->head.next;
			
				while((report_link != &report_path->head) && 
				      (path_link != &path->head)) {
						  
					report_item = list_get_instance(report_link, 
					                                usb_hid_report_usage_path_t, 
					                                link);
						  
					path_item = list_get_instance(path_link, 
					                              usb_hid_report_usage_path_t, 
					                              link);		

					if((report_item->usage_page != path_item->usage_page) || 
					   ((only_page == 0) && 
					    (report_item->usage != path_item->usage))) {
							
						   return 1;
					} else {
						report_link = report_link->next;
						path_link = path_link->next;			
					}
			
				}

				if((((flags & USB_HID_PATH_COMPARE_BEGIN) != 0) && (path_link == &path->head)) || 
				   ((report_link == &report_path->head) && (path_link == &path->head))) {
					return EOK;
				}
				else {
					return 1;
				}						
			break;

		/* path is suffix of report_path */
		case USB_HID_PATH_COMPARE_END:

				report_link = report_path->head.prev;
				path_link = path->head.prev;

				if(list_empty(&path->head)){
					return EOK;
				}
			
				while((report_link != &report_path->head) && 
				      (path_link != &path->head)) {
						  
					report_item = list_get_instance(report_link, 
					                                usb_hid_report_usage_path_t, 
					                                link);
					path_item = list_get_instance(path_link, 
					                              usb_hid_report_usage_path_t, 
					                              link);		

					if((report_item->usage_page != path_item->usage_page) || 
					   ((only_page == 0) && 
					    (report_item->usage != path_item->usage))) {
						   return 1;
					} else {
						report_link = report_link->prev;
						path_link = path_link->prev;			
					}
			
				}

				if(path_link == &path->head) {
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
		list_initialize(&path->head);
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
	while(!list_empty(&path->head)){
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

	new_usage_path->report_id = usage_path->report_id;
	
	if(list_empty(&usage_path->head)){
		return new_usage_path;
	}

	path_link = usage_path->head.next;
	while(path_link != &usage_path->head) {
		path_item = list_get_instance(path_link, usb_hid_report_usage_path_t, 
		                              link);
		new_path_item = malloc(sizeof(usb_hid_report_usage_path_t));
		if(new_path_item == NULL) {
			return NULL;
		}
		
		list_initialize (&new_path_item->link);		
		new_path_item->usage_page = path_item->usage_page;
		new_path_item->usage = path_item->usage;		
		new_path_item->flags = path_item->flags;		
		
		list_append(&new_path_item->link, &new_usage_path->head);
		new_usage_path->depth++;

		path_link = path_link->next;
	}

	return new_usage_path;
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
 * @}
 */
