/*
 * Copyright (c) 2010 Matus Dekanek
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

/** @addtogroup usb
 * @{
 */
/** @file
 * @brief Hub driver.
 */

#ifndef USBHUB_PRIVATE_H
#define	USBHUB_PRIVATE_H

#include "usbhub.h"
#include <adt/list.h>
#include <bool.h>
#include <driver.h>
#include <usb/usb.h>
#include <usb/devreq.h>

//************
//
// convenience define for malloc
//
//************
#define usb_new(type) (type*)malloc(sizeof(type))


//************
//
// My private list implementation; I did not like the original helenos list
//
// This one does not depend on the structure of stored data
//
//************

/** general list structure */


typedef struct usb_general_list{
	void * data;
	struct usb_general_list * prev, * next;
} usb_general_list_t;

/** create head of usb general list */
usb_general_list_t * usb_lst_create(void);

/** initialize head of usb general list */
void usb_lst_init(usb_general_list_t * lst);


/** is the list empty? */
static inline bool usb_lst_empty(usb_general_list_t * lst){
	return lst?(lst->next==lst):true;
}

/** append data behind item */
void usb_lst_append(usb_general_list_t * lst, void * data);

/** prepend data beore item */
void usb_lst_prepend(usb_general_list_t * lst, void * data);

/** remove list item from list */
void usb_lst_remove(usb_general_list_t * item);

/** get data o specified type from list item */
#define usb_lst_get_data(item, type)  (type *) (item->data)

/** get usb_hub_info_t data from list item */
static inline usb_hub_info_t * usb_hub_lst_get_data(usb_general_list_t * item) {
	return usb_lst_get_data(item,usb_hub_info_t);
}

/**
 * @brief create hub structure instance
 *
 * @param device
 * @return
 */
usb_hub_info_t * usb_create_hub_info(device_t * device);

/** list of hubs maanged by this driver */
extern usb_general_list_t usb_hub_list;


/**
 * @brief perform complete control read transaction
 *
 * manages all three steps of transaction: setup, read and finalize
 * @param phone
 * @param target
 * @param request request for data
 * @param rcvd_buffer received data
 * @param rcvd_size
 * @param actual_size actual size of received data
 * @return error code
 */
int usb_drv_sync_control_read(
    int phone, usb_target_t target,
    usb_device_request_setup_packet_t * request,
    void * rcvd_buffer, size_t rcvd_size, size_t * actual_size
);

/**
 * @brief perform complete control write transaction
 *
 * maanges all three steps of transaction: setup, write and finalize
 * @param phone
 * @param target
 * @param request request to send data
 * @param sent_buffer
 * @param sent_size
 * @return error code
 */
int usb_drv_sync_control_write(
    int phone, usb_target_t target,
    usb_device_request_setup_packet_t * request,
    void * sent_buffer, size_t sent_size
);


/**
 * set the device requsst to be a set address request
 * @param request
 * @param addr
 */
static inline void usb_hub_set_set_address_request(
usb_device_request_setup_packet_t * request, uint16_t addr
){
	request->index = 0;
	request->request_type = 0;/// \TODO this is not very nice sollution
	request->request = USB_DEVREQ_SET_ADDRESS;
	request->value = addr;
	request->length = 0;
}


#endif	/* USBHUB_PRIVATE_H */

/**
 * @}
 */
