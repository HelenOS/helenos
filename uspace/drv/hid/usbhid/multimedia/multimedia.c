/*
 * Copyright (c) 2011 Lubos Slovak
 * Copyright (c) 2011 Vojtech Horky
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
#include <usb/hid/usages/consumer.h>

#include <errno.h>
#include <async.h>
#include <str_error.h>

#include <ipc/kbdev.h>
#include <io/kbd_event.h>

#define NAME  "multimedia-keys"

/**
 * Logitech UltraX device type.
 */
typedef struct usb_multimedia_t {
	/** Previously pressed keys (not translated to key codes). */
	//int32_t *keys_old;
	/** Currently pressed keys (not translated to key codes). */
	//int32_t *keys;
	/** Count of stored keys (i.e. number of keys in the report). */
	//size_t key_count;
	/** IPC session to the console device (for sending key events). */
	async_sess_t *console_sess;
} usb_multimedia_t;

/** Default handler for IPC methods not handled by DDF.
 *
 * Currently recognizes only one method (IPC_M_CONNECT_TO_ME), in which case it
 * assumes the caller is the console and thus it stores IPC session to it for
 * later use by the driver to notify about key events.
 *
 * @param fun   Device function handling the call.
 * @param icall Call data.
 *
 */
static void default_connection_handler(ddf_fun_t *fun, ipc_call_t *icall)
{
	usb_log_debug(NAME " default_connection_handler()");

	usb_multimedia_t *multim_dev = ddf_fun_data_get(fun);

	async_sess_t *sess =
	    async_callback_receive_start(EXCHANGE_SERIALIZE, icall);
	if (sess != NULL) {
		if (multim_dev->console_sess == NULL) {
			multim_dev->console_sess = sess;
			usb_log_debug(NAME " Saved session to console: %p",
			    sess);
			async_answer_0(icall, EOK);
		} else
			async_answer_0(icall, ELIMIT);
	} else
		async_answer_0(icall, EINVAL);
}

static ddf_dev_ops_t multimedia_ops = {
	.default_handler = default_connection_handler
};

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
 * @param multim_dev
 * @param type Type of the event (press / release). Recognized values:
 *             KEY_PRESS, KEY_RELEASE
 * @param key Key code of the key according to HID Usage Tables.
 */
static void usb_multimedia_push_ev(
    usb_multimedia_t *multim_dev, int type, unsigned int key)
{
	assert(multim_dev != NULL);

	const kbd_event_t ev = {
		.type = type,
		.key = key,
		.mods = 0,
		.c = 0,
	};

	usb_log_debug2(NAME " Sending key %d to the console", ev.key);
	if (multim_dev->console_sess == NULL) {
		usb_log_warning(
		    "Connection to console not ready, key discarded.\n");
		return;
	}

	async_exch_t *exch = async_exchange_begin(multim_dev->console_sess);
	if (exch != NULL) {
		async_msg_4(exch, KBDEV_EVENT, ev.type, ev.key, ev.mods, ev.c);
		async_exchange_end(exch);
	} else {
		usb_log_warning("Failed to send multimedia key.");
	}
}

errno_t usb_multimedia_init(struct usb_hid_dev *hid_dev, void **data)
{
	if (hid_dev == NULL || hid_dev->usb_dev == NULL) {
		return EINVAL;
	}

	usb_log_debug(NAME " Initializing HID/multimedia structure...");

	/* Create the exposed function. */
	ddf_fun_t *fun = usb_device_ddf_fun_create(
	    hid_dev->usb_dev, fun_exposed, NAME);
	if (fun == NULL) {
		usb_log_error("Could not create DDF function node.");
		return ENOMEM;
	}

	ddf_fun_set_ops(fun, &multimedia_ops);

	usb_multimedia_t *multim_dev =
	    ddf_fun_data_alloc(fun, sizeof(usb_multimedia_t));
	if (multim_dev == NULL) {
		ddf_fun_destroy(fun);
		return ENOMEM;
	}

	multim_dev->console_sess = NULL;

	//todo Autorepeat?

	errno_t rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		usb_log_error("Could not bind DDF function: %s.",
		    str_error(rc));
		ddf_fun_destroy(fun);
		return rc;
	}

	usb_log_debug(NAME " function created (handle: %" PRIun ").",
	    ddf_fun_get_handle(fun));

	rc = ddf_fun_add_to_category(fun, "keyboard");
	if (rc != EOK) {
		usb_log_error(
		    "Could not add DDF function to category 'keyboard': %s.\n",
		    str_error(rc));
		if (ddf_fun_unbind(fun) != EOK) {
			usb_log_error("Failed to unbind %s, won't destroy.",
			    ddf_fun_get_name(fun));
		} else {
			ddf_fun_destroy(fun);
		}
		return rc;
	}

	/* Save the KBD device structure into the HID device structure. */
	*data = fun;

	usb_log_debug(NAME " HID/multimedia structure initialized.");
	return EOK;
}

void usb_multimedia_deinit(struct usb_hid_dev *hid_dev, void *data)
{
	ddf_fun_t *fun = data;

	usb_multimedia_t *multim_dev = ddf_fun_data_get(fun);

	/* Hangup session to the console */
	if (multim_dev->console_sess)
		async_hangup(multim_dev->console_sess);
	if (ddf_fun_unbind(fun) != EOK) {
		usb_log_error("Failed to unbind %s, won't destroy.",
		    ddf_fun_get_name(fun));
	} else {
		usb_log_debug2("%s unbound.", ddf_fun_get_name(fun));
		/*
		 * This frees multim_dev too as it was stored in
		 * fun->data
		 */
		ddf_fun_destroy(fun);
	}
}

bool usb_multimedia_polling_callback(struct usb_hid_dev *hid_dev, void *data)
{
	// TODO: checks
	ddf_fun_t *fun = data;
	if (hid_dev == NULL) {
		return false;
	}

	usb_multimedia_t *multim_dev = ddf_fun_data_get(fun);

	usb_hid_report_path_t *path = usb_hid_report_path();
	if (path == NULL)
		return true; /* This might be a temporary failure. */

	errno_t ret =
	    usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_CONSUMER, 0);
	if (ret != EOK) {
		usb_hid_report_path_free(path);
		return true; /* This might be a temporary failure. */
	}

	usb_hid_report_path_set_report_id(path, hid_dev->report_id);

	usb_hid_report_field_t *field = usb_hid_report_get_sibling(
	    &hid_dev->report, NULL, path, USB_HID_PATH_COMPARE_END |
	    USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY,
	    USB_HID_REPORT_TYPE_INPUT);

	//FIXME Is this iterating OK if done multiple times?
	//FIXME The parsing is not OK. (what's wrong?)
	while (field != NULL) {
		if (field->value != 0) {
			usb_log_debug(NAME " KEY VALUE(%X) USAGE(%X)",
			    field->value, field->usage);
			const unsigned key =
			    usb_multimedia_map_usage(field->usage);
			const char *key_str =
			    usbhid_multimedia_usage_to_str(field->usage);
			usb_log_info("Pressed key: %s", key_str);
			usb_multimedia_push_ev(multim_dev, KEY_PRESS, key);
		}

		field = usb_hid_report_get_sibling(
		    &hid_dev->report, field, path, USB_HID_PATH_COMPARE_END |
		    USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY,
		    USB_HID_REPORT_TYPE_INPUT);
	}

	usb_hid_report_path_free(path);

	return true;
}
/**
 * @}
 */
