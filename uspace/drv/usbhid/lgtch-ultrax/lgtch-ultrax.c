/*
 * Copyright (c) 2011 Lubos Slovak, Vojtech Horky
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

/** @addtogroup drvusbhid
 * @{
 */
/**
 * @file
 * USB Logitech UltraX Keyboard sample driver.
 */


#include "lgtch-ultrax.h"
#include "../usbhid.h"
#include "keymap.h"

#include <usb/classes/hidparser.h>
#include <usb/debug.h>
#include <usb/classes/hidut.h>

#include <errno.h>
#include <str_error.h>

#include <ipc/kbd.h>
#include <io/console.h>

#define NAME "lgtch-ultrax"

typedef enum usb_lgtch_flags {
	USB_LGTCH_STATUS_UNINITIALIZED = 0,
	USB_LGTCH_STATUS_INITIALIZED = 1,
	USB_LGTCH_STATUS_TO_DESTROY = -1
} usb_lgtch_flags;

/*----------------------------------------------------------------------------*/
/**
 * Logitech UltraX device type.
 */
typedef struct usb_lgtch_ultrax_t {
	/** Previously pressed keys (not translated to key codes). */
	int32_t *keys_old;
	/** Currently pressed keys (not translated to key codes). */
	int32_t *keys;
	/** Count of stored keys (i.e. number of keys in the report). */
	size_t key_count;
	
	/** IPC phone to the console device (for sending key events). */
	int console_phone;

	/** Information for auto-repeat of keys. */
//	usb_kbd_repeat_t repeat;
	
	/** Mutex for accessing the information about auto-repeat. */
//	fibril_mutex_t *repeat_mtx;

	/** State of the structure (for checking before use). 
	 * 
	 * 0 - not initialized
	 * 1 - initialized
	 * -1 - ready for destroying
	 */
	int initialized;
} usb_lgtch_ultrax_t;


/*----------------------------------------------------------------------------*/
/** 
 * Default handler for IPC methods not handled by DDF.
 *
 * Currently recognizes only one method (IPC_M_CONNECT_TO_ME), in which case it
 * assumes the caller is the console and thus it stores IPC phone to it for 
 * later use by the driver to notify about key events.
 *
 * @param fun Device function handling the call.
 * @param icallid Call id.
 * @param icall Call data.
 */
static void default_connection_handler(ddf_fun_t *fun,
    ipc_callid_t icallid, ipc_call_t *icall)
{
	usb_log_debug(NAME " default_connection_handler()\n");
	
	sysarg_t method = IPC_GET_IMETHOD(*icall);
	
	usb_hid_dev_t *hid_dev = (usb_hid_dev_t *)fun->driver_data;
	
	if (hid_dev == NULL || hid_dev->data == NULL) {
		async_answer_0(icallid, EINVAL);
		return;
	}
	
	assert(hid_dev != NULL);
	assert(hid_dev->data != NULL);
	usb_lgtch_ultrax_t *lgtch_dev = (usb_lgtch_ultrax_t *)hid_dev->data;

	if (method == IPC_M_CONNECT_TO_ME) {
		int callback = IPC_GET_ARG5(*icall);

		if (lgtch_dev->console_phone != -1) {
			async_answer_0(icallid, ELIMIT);
			return;
		}

		lgtch_dev->console_phone = callback;
		usb_log_debug(NAME " Saved phone to console: %d\n", callback);
		async_answer_0(icallid, EOK);
		return;
	}
	
	async_answer_0(icallid, EINVAL);
}

/*----------------------------------------------------------------------------*/

static ddf_dev_ops_t lgtch_ultrax_ops = {
	.default_handler = default_connection_handler
};

/*----------------------------------------------------------------------------*/

//static void usb_lgtch_process_keycodes(const uint8_t *key_codes, size_t count,
//    uint8_t report_id, void *arg);

//static const usb_hid_report_in_callbacks_t usb_lgtch_parser_callbacks = {
//	.keyboard = usb_lgtch_process_keycodes
//};

///*----------------------------------------------------------------------------*/

//static void usb_lgtch_process_keycodes(const uint8_t *key_codes, size_t count,
//    uint8_t report_id, void *arg)
//{
//	// TODO: checks
	
//	usb_log_debug(NAME " Got keys from parser (report id: %u): %s\n", 
//	    report_id, usb_debug_str_buffer(key_codes, count, 0));
//}

/*----------------------------------------------------------------------------*/
/**
 * Processes key events.
 *
 * @note This function was copied from AT keyboard driver and modified to suit
 *       USB keyboard.
 *
 * @note Lock keys are not sent to the console, as they are completely handled
 *       in the driver. It may, however, be required later that the driver
 *       sends also these keys to application (otherwise it cannot use those
 *       keys at all).
 * 
 * @param hid_dev 
 * @param lgtch_dev 
 * @param type Type of the event (press / release). Recognized values: 
 *             KEY_PRESS, KEY_RELEASE
 * @param key Key code of the key according to HID Usage Tables.
 */
static void usb_lgtch_push_ev(usb_hid_dev_t *hid_dev, int type, 
    unsigned int key)
{
	assert(hid_dev != NULL);
	assert(hid_dev->data != NULL);
	
	usb_lgtch_ultrax_t *lgtch_dev = (usb_lgtch_ultrax_t *)hid_dev->data;
	
	console_event_t ev;
	
	ev.type = type;
	ev.key = key;
	ev.mods = 0;

	ev.c = 0;

	usb_log_debug2(NAME " Sending key %d to the console\n", ev.key);
	if (lgtch_dev->console_phone < 0) {
		usb_log_warning(
		    "Connection to console not ready, key discarded.\n");
		return;
	}
	
	async_msg_4(lgtch_dev->console_phone, KBD_EVENT, ev.type, ev.key, 
	    ev.mods, ev.c);
}

/*----------------------------------------------------------------------------*/

static void usb_lgtch_free(usb_lgtch_ultrax_t **lgtch_dev)
{
	if (lgtch_dev == NULL || *lgtch_dev == NULL) {
		return;
	}
	
	// hangup phone to the console
	async_hangup((*lgtch_dev)->console_phone);
	
//	if ((*lgtch_dev)->repeat_mtx != NULL) {
//		/* TODO: replace by some check and wait */
//		assert(!fibril_mutex_is_locked((*lgtch_dev)->repeat_mtx));
//		free((*lgtch_dev)->repeat_mtx);
//	}
	
	// free all buffers
	if ((*lgtch_dev)->keys != NULL) {
		free((*lgtch_dev)->keys);
	}
	if ((*lgtch_dev)->keys_old != NULL) {
		free((*lgtch_dev)->keys_old);
	}

	free(*lgtch_dev);
	*lgtch_dev = NULL;
}

/*----------------------------------------------------------------------------*/

static int usb_lgtch_create_function(usb_hid_dev_t *hid_dev)
{
	/* Create the function exposed under /dev/devices. */
	ddf_fun_t *fun = ddf_fun_create(hid_dev->usb_dev->ddf_dev, fun_exposed, 
	    NAME);
	if (fun == NULL) {
		usb_log_error("Could not create DDF function node.\n");
		return ENOMEM;
	}
	
	/*
	 * Store the initialized HID device and HID ops
	 * to the DDF function.
	 */
	fun->ops = &lgtch_ultrax_ops;
	fun->driver_data = hid_dev;   // TODO: maybe change to hid_dev->data
	
	int rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		usb_log_error("Could not bind DDF function: %s.\n",
		    str_error(rc));
		// TODO: Can / should I destroy the DDF function?
		ddf_fun_destroy(fun);
		return rc;
	}
	
	rc = ddf_fun_add_to_class(fun, "keyboard");
	if (rc != EOK) {
		usb_log_error(
		    "Could not add DDF function to class 'keyboard': %s.\n",
		    str_error(rc));
		// TODO: Can / should I destroy the DDF function?
		ddf_fun_destroy(fun);
		return rc;
	}
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

int usb_lgtch_init(struct usb_hid_dev *hid_dev)
{
	if (hid_dev == NULL || hid_dev->usb_dev == NULL) {
		return EINVAL; /*! @todo Other return code? */
	}
	
	usb_log_debug(NAME " Initializing HID/lgtch_ultrax structure...\n");
	
	usb_lgtch_ultrax_t *lgtch_dev = (usb_lgtch_ultrax_t *)malloc(
	    sizeof(usb_lgtch_ultrax_t));
	if (lgtch_dev == NULL) {
		return ENOMEM;
	}
	
	lgtch_dev->console_phone = -1;
	
	usb_hid_report_path_t *path = usb_hid_report_path();
	usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_CONSUMER, 0);
	
	usb_hid_report_path_set_report_id(path, 1);
	
	lgtch_dev->key_count = usb_hid_report_size(
	    hid_dev->report, 0, USB_HID_REPORT_TYPE_INPUT);
	usb_hid_report_path_free(path);
	
	usb_log_debug(NAME " Size of the input report: %zu\n", 
	    lgtch_dev->key_count);
	
	lgtch_dev->keys = (int32_t *)calloc(lgtch_dev->key_count, 
	    sizeof(int32_t));
	
	if (lgtch_dev->keys == NULL) {
		usb_log_fatal("No memory!\n");
		free(lgtch_dev);
		return ENOMEM;
	}
	
	lgtch_dev->keys_old = 
		(int32_t *)calloc(lgtch_dev->key_count, sizeof(int32_t));
	
	if (lgtch_dev->keys_old == NULL) {
		usb_log_fatal("No memory!\n");
		free(lgtch_dev->keys);
		free(lgtch_dev);
		return ENOMEM;
	}
	
	/*! @todo Autorepeat */
	
	// save the KBD device structure into the HID device structure
	hid_dev->data = lgtch_dev;
	
	lgtch_dev->initialized = USB_LGTCH_STATUS_INITIALIZED;
	usb_log_debug(NAME " HID/lgtch_ultrax device structure initialized.\n");
	
	int rc = usb_lgtch_create_function(hid_dev);
	if (rc != EOK) {
		usb_lgtch_free(&lgtch_dev);
		return rc;
	}
	
	usb_log_debug(NAME " HID/lgtch_ultrax structure initialized.\n");
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

void usb_lgtch_deinit(struct usb_hid_dev *hid_dev)
{
	if (hid_dev == NULL) {
		return;
	}
	
	if (hid_dev->data != NULL) {
		usb_lgtch_ultrax_t *lgtch_dev = 
		    (usb_lgtch_ultrax_t *)hid_dev->data;
//		if (usb_kbd_is_initialized(kbd_dev)) {
//			usb_kbd_mark_unusable(kbd_dev);
//		} else {
			usb_lgtch_free(&lgtch_dev);
			hid_dev->data = NULL;
//		}
	}
}

/*----------------------------------------------------------------------------*/

bool usb_lgtch_polling_callback(struct usb_hid_dev *hid_dev, 
    uint8_t *buffer, size_t buffer_size)
{
	// TODO: checks
	
	usb_log_debug(NAME " usb_lgtch_polling_callback(%p, %p, %zu)\n",
	    hid_dev, buffer, buffer_size);

	usb_log_debug(NAME " Calling usb_hid_parse_report() with "
	    "buffer %s\n", usb_debug_str_buffer(buffer, buffer_size, 0));
	
	usb_hid_report_path_t *path = usb_hid_report_path();
	usb_hid_report_path_append_item(path, 0xc, 0);

	uint8_t report_id;
	
	int rc = usb_hid_parse_report(hid_dev->report, buffer, buffer_size, 
	    &report_id);
	
	if (rc != EOK) {
		usb_log_warning(NAME "Error in usb_hid_parse_report(): %s\n", 
		    str_error(rc));
		return true;
	}
	
	usb_hid_report_path_set_report_id(path, report_id);

	usb_hid_report_field_t *field = usb_hid_report_get_sibling(
	    hid_dev->report, NULL, path, USB_HID_PATH_COMPARE_END 
	    | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY, 
	    USB_HID_REPORT_TYPE_INPUT);
	
//	unsigned int key;
	
	/*! @todo Is this iterating OK if done multiple times? 
	 *  @todo The parsing is not OK
	 */
	while (field != NULL) {
		if(field->value != 0) {
			usb_log_debug(NAME " KEY VALUE(%X) USAGE(%X)\n", field->value, 
			    field->usage);
		
			//key = usb_lgtch_map_usage(field->usage);
			usb_lgtch_push_ev(hid_dev, KEY_PRESS, field->usage);
		}
		
		field = usb_hid_report_get_sibling(
		    hid_dev->report, field, path, USB_HID_PATH_COMPARE_END
		    | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY, 
		    USB_HID_REPORT_TYPE_INPUT);
	}	

	usb_hid_report_path_free(path);
	
	return true;
}

/**
 * @}
 */
