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
/** @addtogroup usbhub
 * @{
 */
/** @file
 * @brief usblist implementation
 */
#include <sys/types.h>

#include "usbhub_private.h"


usb_general_list_t * usb_lst_create(void) {
	usb_general_list_t* result = usb_new(usb_general_list_t);
	usb_lst_init(result);
	return result;
}

void usb_lst_init(usb_general_list_t * lst) {
	lst->prev = lst;
	lst->next = lst;
	lst->data = NULL;
}

void usb_lst_prepend(usb_general_list_t* item, void* data) {
	usb_general_list_t* appended = usb_new(usb_general_list_t);
	appended->data = data;
	appended->next = item;
	appended->prev = item->prev;
	item->prev->next = appended;
	item->prev = appended;
}

void usb_lst_append(usb_general_list_t* item, void* data) {
	usb_general_list_t* appended = usb_new(usb_general_list_t);
	appended->data = data;
	appended->next = item->next;
	appended->prev = item;
	item->next->prev = appended;
	item->next = appended;
}

void usb_lst_remove(usb_general_list_t* item) {
	item->next->prev = item->prev;
	item->prev->next = item->next;
}



/**
 * @}
 */


