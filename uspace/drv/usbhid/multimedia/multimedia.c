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
 * USB Keyboard multimedia keys subdriver.
 */


#include "multimedia.h"
#include "../usbhid.h"
#include "keymap.h"

#include <usb/hid/hidparser.h>
#include <usb/debug.h>
#include <usb/hid/usages/core.h>

#include <errno.h>
#include <str_error.h>

#include <ipc/kbd.h>
#include <io/console.h>

#define NAME "multimedia-keys"

/*----------------------------------------------------------------------------*/
/**
 * Logitech UltraX device type.
 */
typedef struct usb_multimedia_t {
	/** Previously pressed keys (not translated to key codes). */
	int32_t *keys_old;
	/** Currently pressed keys (not translated to key codes). */
	int32_t *keys;
	/** Count of stored keys (i.e. number of keys in the report). */
	size_t key_count;	
	/** IPC phone to the console device (for sending key events). */
	int console_phone;
} usb_multimedia_t;


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
	
	usb_multimedia_t *multim_dev = (usb_multimedia_t *)fun->driver_data;
	//usb_hid_dev_t *hid_dev = (usb_hid_dev_t *)fun->driver_data;
	
	if (multim_dev == NULL) {
		async_answer_0(icallid, EINVAL);
		return;
	}

	if (method == IPC_M_CONNECT_TO_ME) {
		int callback = IPC_GET_ARG5(*icall);

		if (multim_dev->console_phone != -1) {
			async_answer_0(icallid, ELIMIT);
			return;
		}

		multim_dev->console_phone = callback;
		usb_log_debug(NAME " Saved phone to console: %d\n", callback);
		async_answer_0(icallid, EOK);
		return;
	}
	
	async_answer_0(icallid, EINVAL);
}

/*----------------------------------------------------------------------------*/

static ddf_dev_ops_t multimedia_ops = {
	.default_handler = default_connection_handler
};

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
static void usb_multimedia_push_ev(usb_hid_dev_t *hid_dev, 
    usb_multimedia_t *multim_dev, int type, unsigned int key)
{
	assert(hid_dev != NULL);
	assert(multim_dev != NULL);
	
//	usb_multimedia_t *multim_dev = (usb_multimedia_t *)hid_dev->data;
	
	console_event_t ev;
	
	ev.type = type;
	ev.key = key;
	ev.mods = 0;
	ev.c = 0;

	usb_log_debug2(NAME " Sending key %d to the console\n", ev.key);
	if (multim_dev->console_phone < 0) {
		usb_log_warning(
		    "Connection to console not ready, key discarded.\n");
		return;
	}
	
	async_msg_4(multim_dev->console_phone, KBD_EVENT, ev.type, ev.key, 
	    ev.mods, ev.c);
}

/*----------------------------------------------------------------------------*/

static void usb_multimedia_free(usb_multimedia_t **multim_dev)
{
	if (multim_dev == NULL || *multim_dev == NULL) {
		return;
	}
	
	// hangup phone to the console
	async_hangup((*multim_dev)->console_phone);
	
	// free all buffers
	if ((*multim_dev)->keys != NULL) {
		free((*multim_dev)->keys);
	}
	if ((*multim_dev)->keys_old != NULL) {
		free((*multim_dev)->keys_old);
	}

	free(*multim_dev);
	*multim_dev = NULL;
}

/*----------------------------------------------------------------------------*/

static int usb_multimedia_create_function(usb_hid_dev_t *hid_dev, 
    usb_multimedia_t *multim_dev)
{
	/* Create the function exposed under /dev/devices. */
	ddf_fun_t *fun = ddf_fun_create(hid_dev->usb_dev->ddf_dev, fun_exposed, 
	    NAME);
	if (fun == NULL) {
		usb_log_error("Could not create DDF function node.\n");
		return ENOMEM;
	}
	
	fun->ops = &multimedia_ops;
	fun->driver_data = multim_dev;   // TODO: maybe change to hid_dev->data
	
	int rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		usb_log_error("Could not bind DDF function: %s.\n",
		    str_error(rc));
		// TODO: Can / should I destroy the DDF function?
		ddf_fun_destroy(fun);
		return rc;
	}
	
	usb_log_debug("%s function created. Handle: %d\n", NAME, fun->handle);
	
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

int usb_multimedia_init(struct usb_hid_dev *hid_dev, void **data)
{
	if (hid_dev == NULL || hid_dev->usb_dev == NULL) {
		return EINVAL; /*! @todo Other return code? */
	}
	
	usb_log_debug(NAME " Initializing HID/multimedia structure...\n");
	
	usb_multimedia_t *multim_dev = (usb_multimedia_t *)malloc(
	    sizeof(usb_multimedia_t));
	if (multim_dev == NULL) {
		return ENOMEM;
	}
	
	multim_dev->console_phone = -1;
	
	usb_hid_report_path_t *path = usb_hid_report_path();
	usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_CONSUMER, 0);
	
	usb_hid_report_path_set_report_id(path, 1);
	
	multim_dev->key_count = usb_hid_report_size(
	    hid_dev->report, 0, USB_HID_REPORT_TYPE_INPUT);

	usb_hid_report_path_free(path);
	
	usb_log_debug(NAME " Size of the input report: %zu\n", 
	    multim_dev->key_count);
	
	multim_dev->keys = (int32_t *)calloc(multim_dev->key_count, 
	    sizeof(int32_t));
	
	if (multim_dev->keys == NULL) {
		usb_log_fatal("No memory!\n");
		free(multim_dev);
		return ENOMEM;
	}
	
	multim_dev->keys_old = 
		(int32_t *)calloc(multim_dev->key_count, sizeof(int32_t));
	
	if (multim_dev->keys_old == NULL) {
		usb_log_fatal("No memory!\n");
		free(multim_dev->keys);
		free(multim_dev);
		return ENOMEM;
	}
	
	/*! @todo Autorepeat */
	
	// save the KBD device structure into the HID device structure
	*data = multim_dev;
	
	usb_log_debug(NAME " HID/multimedia device structure initialized.\n");
	
	int rc = usb_multimedia_create_function(hid_dev, multim_dev);
	if (rc != EOK) {
		usb_multimedia_free(&multim_dev);
		return rc;
	}
	
	usb_log_debug(NAME " HID/multimedia structure initialized.\n");
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

void usb_multimedia_deinit(struct usb_hid_dev *hid_dev, void *data)
{
	if (hid_dev == NULL) {
		return;
	}
	
	if (data != NULL) {
		usb_multimedia_t *multim_dev = (usb_multimedia_t *)data;
		usb_multimedia_free(&multim_dev);
	}
}

/*----------------------------------------------------------------------------*/

bool usb_multimedia_polling_callback(struct usb_hid_dev *hid_dev, void *data, 
    uint8_t *buffer, size_t buffer_size)
{
	// TODO: checks
	
	usb_log_debug(NAME " usb_lgtch_polling_callback(%p, %p, %zu)\n",
	    hid_dev, buffer, buffer_size);
	
	if (data == NULL) {
		return EINVAL;	// TODO: other error code?
	}
	
	usb_multimedia_t *multim_dev = (usb_multimedia_t *)data;

	usb_log_debug(NAME " Calling usb_hid_parse_report() with "
	    "buffer %s\n", usb_debug_str_buffer(buffer, buffer_size, 0));
	
	usb_hid_report_path_t *path = usb_hid_report_path();
	usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_CONSUMER, 0);

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
			usb_log_debug(NAME " KEY VALUE(%X) USAGE(%X)\n", 
			    field->value, field->usage);
			unsigned int key = 
			    usb_multimedia_map_usage(field->usage);
			const char *key_str = 
			    usb_multimedia_usage_to_str(field->usage);
			usb_log_info("Pressed key: %s\n", key_str);
			usb_multimedia_push_ev(hid_dev, multim_dev, KEY_PRESS, 
			                       key);
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
