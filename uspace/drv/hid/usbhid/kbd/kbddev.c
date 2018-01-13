/*
 * Copyright (c) 2011 Lubos Slovak
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
 * USB HID keyboard device structure and API.
 */

#include <errno.h>
#include <str_error.h>
#include <stdio.h>

#include <io/keycode.h>
#include <io/console.h>
#include <abi/ipc/methods.h>
#include <ipc/kbdev.h>
#include <async.h>
#include <fibril.h>
#include <fibril_synch.h>

#include <ddf/log.h>

#include <usb/usb.h>
#include <usb/dev/dp.h>
#include <usb/dev/request.h>
#include <usb/hid/hid.h>
#include <usb/dev/pipes.h>
#include <usb/debug.h>
#include <usb/hid/hidparser.h>
#include <usb/classes/classes.h>
#include <usb/hid/usages/core.h>
#include <usb/hid/request.h>
#include <usb/hid/hidreport.h>
#include <usb/hid/usages/led.h>

#include <usb/dev/driver.h>

#include "kbddev.h"

#include "conv.h"
#include "kbdrepeat.h"

#include "../usbhid.h"

static void default_connection_handler(ddf_fun_t *, ipc_callid_t, ipc_call_t *);
static ddf_dev_ops_t kbdops = { .default_handler = default_connection_handler };


static const unsigned DEFAULT_ACTIVE_MODS = KM_NUM_LOCK;

static const uint8_t ERROR_ROLLOVER = 1;

/** Default idle rate for keyboards. */
static const uint8_t IDLE_RATE = 0;

/** Delay before a pressed key starts auto-repeating. */
static const unsigned int DEFAULT_DELAY_BEFORE_FIRST_REPEAT = 500 * 1000;

/** Delay between two repeats of a pressed key when auto-repeating. */
static const unsigned int DEFAULT_REPEAT_DELAY = 50 * 1000;


/** Keyboard polling endpoint description for boot protocol class. */
const usb_endpoint_description_t usb_hid_kbd_poll_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HID,
	.interface_subclass = USB_HID_SUBCLASS_BOOT,
	.interface_protocol = USB_HID_PROTOCOL_KEYBOARD,
	.flags = 0
};

const char *HID_KBD_FUN_NAME = "keyboard";
const char *HID_KBD_CATEGORY_NAME = "keyboard";

static void usb_kbd_set_led(usb_hid_dev_t *hid_dev, usb_kbd_t *kbd_dev);

static const uint8_t USB_KBD_BOOT_REPORT_DESCRIPTOR[] = {
	0x05, 0x01,  /* Usage Page (Generic Desktop), */
	0x09, 0x06,  /* Usage (Keyboard), */
	0xA1, 0x01,  /* Collection (Application), */
	0x75, 0x01,  /*   Report Size (1), */
	0x95, 0x08,  /*   Report Count (8), */
	0x05, 0x07,  /*   Usage Page (Key Codes); */
	0x19, 0xE0,  /*   Usage Minimum (224), */
	0x29, 0xE7,  /*   Usage Maximum (231), */
	0x15, 0x00,  /*   Logical Minimum (0), */
	0x25, 0x01,  /*   Logical Maximum (1), */
	0x81, 0x02,  /*   Input (Data, Variable, Absolute),  ; Modifier byte */
	0x95, 0x01,  /*   Report Count (1), */
	0x75, 0x08,  /*   Report Size (8), */
	0x81, 0x01,  /*   Input (Constant),                  ; Reserved byte */
	0x95, 0x05,  /*   Report Count (5), */
	0x75, 0x01,  /*   Report Size (1), */
	0x05, 0x08,  /*   Usage Page (Page# for LEDs), */
	0x19, 0x01,  /*   Usage Minimum (1), */
	0x29, 0x05,  /*   Usage Maxmimum (5), */
	0x91, 0x02,  /*   Output (Data, Variable, Absolute),  ; LED report */
	0x95, 0x01,  /*   Report Count (1), */
	0x75, 0x03,  /*   Report Size (3), */
	0x91, 0x01,  /*   Output (Constant),            ; LED report padding */
	0x95, 0x06,  /*   Report Count (6), */
	0x75, 0x08,  /*   Report Size (8), */
	0x15, 0x00,  /*   Logical Minimum (0), */
	0x25, 0xff,  /*   Logical Maximum (255), */
	0x05, 0x07,  /*   Usage Page (Key Codes), */
	0x19, 0x00,  /*   Usage Minimum (0), */
	0x29, 0xff,  /*   Usage Maximum (255), */
	0x81, 0x00,  /*   Input (Data, Array),   ; Key arrays (6 bytes) */
	0xC0         /* End Collection */
};

typedef enum usb_kbd_flags {
	USB_KBD_STATUS_UNINITIALIZED = 0,
	USB_KBD_STATUS_INITIALIZED = 1,
	USB_KBD_STATUS_TO_DESTROY = -1
} usb_kbd_flags;

/* IPC method handler                                                         */

/**
 * Default handler for IPC methods not handled by DDF.
 *
 * Currently recognizes only two methods (IPC_M_CONNECT_TO_ME and KBDEV_SET_IND)
 * IPC_M_CONNECT_TO_ME assumes the caller is the console and  stores IPC
 * session to it for later use by the driver to notify about key events.
 * KBDEV_SET_IND sets LED keyboard indicators.
 *
 * @param fun Device function handling the call.
 * @param icallid Call id.
 * @param icall Call data.
 */
static void default_connection_handler(ddf_fun_t *fun,
    ipc_callid_t icallid, ipc_call_t *icall)
{
	const sysarg_t method = IPC_GET_IMETHOD(*icall);
	usb_kbd_t *kbd_dev = ddf_fun_data_get(fun);

	switch (method) {
	case KBDEV_SET_IND:
		kbd_dev->mods = IPC_GET_ARG1(*icall);
		usb_kbd_set_led(kbd_dev->hid_dev, kbd_dev);
		async_answer_0(icallid, EOK);
		break;
	/* This might be ugly but async_callback_receive_start makes no
	 * difference for incorrect call and malloc failure. */
	case IPC_M_CONNECT_TO_ME: {
		async_sess_t *sess =
		    async_callback_receive_start(EXCHANGE_SERIALIZE, icall);
		/* Probably ENOMEM error, try again. */
		if (sess == NULL) {
			usb_log_warning(
			    "Failed to create start console session.\n");
			async_answer_0(icallid, EAGAIN);
			break;
		}
		if (kbd_dev->client_sess == NULL) {
			kbd_dev->client_sess = sess;
			usb_log_debug("%s: OK\n", __FUNCTION__);
			async_answer_0(icallid, EOK);
		} else {
			usb_log_error("%s: console session already set\n",
			   __FUNCTION__);
			async_answer_0(icallid, ELIMIT);
		}
		break;
	}
	default:
			usb_log_error("%s: Unknown method: %d.\n",
			    __FUNCTION__, (int) method);
			async_answer_0(icallid, EINVAL);
			break;
	}

}

/* Key processing functions                                                   */

/**
 * Handles turning of LED lights on and off.
 *
 * As with most other keyboards, the LED indicators in USB keyboards are
 * driven by software. When state of some modifier changes, the input server
 * will call us and tell us to update the LED state and what the new state
 * should be.
 *
 * This functions sets the LED lights according to current settings of modifiers
 * kept in the keyboard device structure.
 *
 * @param kbd_dev Keyboard device structure.
 */
static void usb_kbd_set_led(usb_hid_dev_t *hid_dev, usb_kbd_t *kbd_dev)
{
	if (kbd_dev->output_size == 0) {
		return;
	}

	/* Reset the LED data. */
	memset(kbd_dev->led_data, 0, kbd_dev->led_output_size * sizeof(int32_t));
	usb_log_debug("Creating output report:\n");

	usb_hid_report_field_t *field = usb_hid_report_get_sibling(
	    &hid_dev->report, NULL, kbd_dev->led_path,
	    USB_HID_PATH_COMPARE_END | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY,
	    USB_HID_REPORT_TYPE_OUTPUT);

	while (field != NULL) {

		if ((field->usage == USB_HID_LED_NUM_LOCK)
		    && (kbd_dev->mods & KM_NUM_LOCK)){
			field->value = 1;
		}

		if ((field->usage == USB_HID_LED_CAPS_LOCK)
		    && (kbd_dev->mods & KM_CAPS_LOCK)){
			field->value = 1;
		}

		if ((field->usage == USB_HID_LED_SCROLL_LOCK)
		    && (kbd_dev->mods & KM_SCROLL_LOCK)){
			field->value = 1;
		}

		field = usb_hid_report_get_sibling(
		    &hid_dev->report, field, kbd_dev->led_path,
		USB_HID_PATH_COMPARE_END | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY,
		    USB_HID_REPORT_TYPE_OUTPUT);
	}

	// TODO: what about the Report ID?
	errno_t rc = usb_hid_report_output_translate(&hid_dev->report, 0,
	    kbd_dev->output_buffer, kbd_dev->output_size);

	if (rc != EOK) {
		usb_log_warning("Could not translate LED output to output"
		    "report.\n");
		return;
	}

	usb_log_debug("Output report buffer: %s\n",
	    usb_debug_str_buffer(kbd_dev->output_buffer, kbd_dev->output_size,
	        0));

	rc = usbhid_req_set_report(
	    usb_device_get_default_pipe(hid_dev->usb_dev),
	    usb_device_get_iface_number(hid_dev->usb_dev),
	    USB_HID_REPORT_TYPE_OUTPUT,
	    kbd_dev->output_buffer, kbd_dev->output_size);
	if (rc != EOK) {
		usb_log_warning("Failed to set kbd indicators.\n");
	}
}

/** Send key event.
 *
 * @param kbd_dev Keyboard device structure.
 * @param type Type of the event (press / release). Recognized values:
 *             KEY_PRESS, KEY_RELEASE
 * @param key Key code
 */
void usb_kbd_push_ev(usb_kbd_t *kbd_dev, int type, unsigned key)
{
	usb_log_debug2("Sending kbdev event %d/%d to the console\n", type, key);
	if (kbd_dev->client_sess == NULL) {
		usb_log_warning(
		    "Connection to console not ready, key discarded.\n");
		return;
	}

	async_exch_t *exch = async_exchange_begin(kbd_dev->client_sess);
	if (exch != NULL) {
		async_msg_2(exch, KBDEV_EVENT, type, key);
		async_exchange_end(exch);
	} else {
		usb_log_warning("Failed to send key to console.\n");
	}
}

static inline int usb_kbd_is_lock(unsigned int key_code)
{
	return (key_code == KC_NUM_LOCK
	    || key_code == KC_SCROLL_LOCK
	    || key_code == KC_CAPS_LOCK);
}

static size_t find_in_array_int32(int32_t val, int32_t *arr, size_t arr_size)
{
	for (size_t i = 0; i < arr_size; i++) {
		if (arr[i] == val) {
			return i;
		}
	}

	return (size_t) -1;
}

/**
 * Checks if some keys were pressed or released and generates key events.
 *
 * An event is created only when key is pressed or released. Besides handling
 * the events (usb_kbd_push_ev()), the auto-repeat fibril is notified about
 * key presses and releases (see usb_kbd_repeat_start() and 
 * usb_kbd_repeat_stop()).
 *
 * @param kbd_dev Keyboard device structure.
 * @param key_codes Parsed keyboard report - codes of currently pressed keys 
 *                  according to HID Usage Tables.
 * @param count Number of key codes in report (size of the report).
 *
 * @sa usb_kbd_push_ev(), usb_kbd_repeat_start(), usb_kbd_repeat_stop()
 */
static void usb_kbd_check_key_changes(usb_hid_dev_t *hid_dev,
    usb_kbd_t *kbd_dev)
{

	/*
	 * First of all, check if the kbd have reported phantom state.
	 *
	 * As there is no way to distinguish keys from modifiers, we do not have
	 * a way to check that 'all keys report Error Rollover'. We thus check
	 * if there is at least one such error and in such case we ignore the
	 * whole input report.
	 */
	size_t i = find_in_array_int32(ERROR_ROLLOVER, kbd_dev->keys,
	    kbd_dev->key_count);
	if (i != (size_t) -1) {
		usb_log_error("Detected phantom state.\n");
		return;
	}

	/*
	 * Key releases
	 */
	for (i = 0; i < kbd_dev->key_count; i++) {
		const int32_t old_key = kbd_dev->keys_old[i];
		/* Find the old key among currently pressed keys. */
		const size_t pos = find_in_array_int32(old_key, kbd_dev->keys,
		    kbd_dev->key_count);
		/* If the key was not found, we need to signal release. */
		if (pos == (size_t) -1) {
			const unsigned key = usbhid_parse_scancode(old_key);
			if (!usb_kbd_is_lock(key)) {
				usb_kbd_repeat_stop(kbd_dev, key);
			}
			usb_kbd_push_ev(kbd_dev, KEY_RELEASE, key);
			usb_log_debug2("Key released: %u "
			    "(USB code %" PRIu32 ")\n", key, old_key);
		}
	}

	/*
	 * Key presses
	 */
	for (i = 0; i < kbd_dev->key_count; ++i) {
		const int32_t new_key = kbd_dev->keys[i];
		/* Find the new key among already pressed keys. */
		const size_t pos = find_in_array_int32(new_key,
		    kbd_dev->keys_old, kbd_dev->key_count);
		/* If the key was not found, we need to signal press. */
		if (pos == (size_t) -1) {
			unsigned key = usbhid_parse_scancode(kbd_dev->keys[i]);
			if (!usb_kbd_is_lock(key)) {
				usb_kbd_repeat_start(kbd_dev, key);
			}
			usb_kbd_push_ev(kbd_dev, KEY_PRESS, key);
			usb_log_debug2("Key pressed: %u "
			    "(USB code %" PRIu32 ")\n", key, new_key);
		}
	}

	memcpy(kbd_dev->keys_old, kbd_dev->keys, kbd_dev->key_count * 4);

	// TODO Get rid of this
	char key_buffer[512];
	ddf_dump_buffer(key_buffer, 512,
	    kbd_dev->keys_old, 4, kbd_dev->key_count, 0);
	usb_log_debug2("Stored keys %s.\n", key_buffer);
}

/* General kbd functions                                                      */

/**
 * Processes data received from the device in form of report.
 *
 * This function uses the HID report parser to translate the data received from
 * the device into generic USB HID key codes and into generic modifiers bitmap.
 * The parser then calls the given callback (usb_kbd_process_keycodes()).
 *
 * @note Currently, only the boot protocol is supported.
 *
 * @param kbd_dev Keyboard device structure (must be initialized).
 * @param buffer Data from the keyboard (i.e. the report).
 * @param actual_size Size of the data from keyboard (report size) in bytes.
 *
 * @sa usb_kbd_process_keycodes(), usb_hid_boot_keyboard_input_report(),
 *     usb_hid_parse_report().
 */
static void usb_kbd_process_data(usb_hid_dev_t *hid_dev, usb_kbd_t *kbd_dev)
{
	assert(hid_dev != NULL);
	assert(kbd_dev != NULL);

	usb_hid_report_path_t *path = usb_hid_report_path();
	if (path == NULL) {
		usb_log_error("Failed to create hid/kbd report path.\n");
		return;
	}

	errno_t ret =
	   usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_KEYBOARD, 0);
	if (ret != EOK) {
		usb_log_error("Failed to append to hid/kbd report path.\n");
		return;
	}

	usb_hid_report_path_set_report_id(path, hid_dev->report_id);

	/* Fill in the currently pressed keys. */
	usb_hid_report_field_t *field = usb_hid_report_get_sibling(
	    &hid_dev->report, NULL, path,
	    USB_HID_PATH_COMPARE_END | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY,
	    USB_HID_REPORT_TYPE_INPUT);
	unsigned i = 0;

	while (field != NULL) {
		usb_log_debug2("FIELD (%p) - VALUE(%d) USAGE(%u)\n",
		    field, field->value, field->usage);

		assert(i < kbd_dev->key_count);

		/* Save the key usage. */
		if (field->value != 0) {
			kbd_dev->keys[i] = field->usage;
		}
		else {
			kbd_dev->keys[i] = 0;
		}
		usb_log_debug2("Saved %u. key usage %d\n", i, kbd_dev->keys[i]);

		++i;
		field = usb_hid_report_get_sibling(
		    &hid_dev->report, field, path, USB_HID_PATH_COMPARE_END
		        | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY,
		    USB_HID_REPORT_TYPE_INPUT);
	}

	usb_hid_report_path_free(path);

	usb_kbd_check_key_changes(hid_dev, kbd_dev);
}

/* HID/KBD structure manipulation                                             */

static errno_t kbd_dev_init(usb_kbd_t *kbd_dev, usb_hid_dev_t *hid_dev)
{
	assert(kbd_dev);
	assert(hid_dev);

	/* Default values */
	fibril_mutex_initialize(&kbd_dev->repeat_mtx);
	kbd_dev->initialized = USB_KBD_STATUS_UNINITIALIZED;

	/* Store link to HID device */
	kbd_dev->hid_dev = hid_dev;

	/* Modifiers and locks */
	kbd_dev->mods = DEFAULT_ACTIVE_MODS;

	/* Autorepeat */
	kbd_dev->repeat.delay_before = DEFAULT_DELAY_BEFORE_FIRST_REPEAT;
	kbd_dev->repeat.delay_between = DEFAULT_REPEAT_DELAY;

	// TODO: make more general
	usb_hid_report_path_t *path = usb_hid_report_path();
	if (path == NULL) {
		usb_log_error("Failed to create kbd report path.\n");
		usb_kbd_destroy(kbd_dev);
		return ENOMEM;
	}

	errno_t ret =
	    usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_KEYBOARD, 0);
	if (ret != EOK) {
		usb_log_error("Failed to append item to kbd report path.\n");
		usb_hid_report_path_free(path);
		usb_kbd_destroy(kbd_dev);
		return ret;
	}

	usb_hid_report_path_set_report_id(path, 0);

	kbd_dev->key_count =
	    usb_hid_report_size(&hid_dev->report, 0, USB_HID_REPORT_TYPE_INPUT);

	usb_hid_report_path_free(path);

	usb_log_debug("Size of the input report: %zu\n", kbd_dev->key_count);

	kbd_dev->keys = calloc(kbd_dev->key_count, sizeof(int32_t));
	if (kbd_dev->keys == NULL) {
		usb_log_error("Failed to allocate key buffer.\n");
		usb_kbd_destroy(kbd_dev);
		return ENOMEM;
	}

	kbd_dev->keys_old = calloc(kbd_dev->key_count, sizeof(int32_t));
	if (kbd_dev->keys_old == NULL) {
		usb_log_error("Failed to allocate old_key buffer.\n");
		usb_kbd_destroy(kbd_dev);
		return ENOMEM;
	}

	/* Output report */
	kbd_dev->output_size = 0;
	kbd_dev->output_buffer = usb_hid_report_output(&hid_dev->report,
	    &kbd_dev->output_size, 0);
	if (kbd_dev->output_buffer == NULL) {
		usb_log_error("Error creating output report buffer.\n");
		usb_kbd_destroy(kbd_dev);
		return ENOMEM;
	}

	usb_log_debug("Output buffer size: %zu\n", kbd_dev->output_size);

	kbd_dev->led_path = usb_hid_report_path();
	if (kbd_dev->led_path == NULL) {
		usb_log_error("Failed to create kbd led report path.\n");
		usb_kbd_destroy(kbd_dev);
		return ENOMEM;
	}

	ret = usb_hid_report_path_append_item(
	    kbd_dev->led_path, USB_HIDUT_PAGE_LED, 0);
	if (ret != EOK) {
		usb_log_error("Failed to append to kbd/led report path.\n");
		usb_kbd_destroy(kbd_dev);
		return ret;
	}

	kbd_dev->led_output_size = usb_hid_report_size(
	    &hid_dev->report, 0, USB_HID_REPORT_TYPE_OUTPUT);

	usb_log_debug("Output report size (in items): %zu\n",
	    kbd_dev->led_output_size);

	kbd_dev->led_data = calloc(kbd_dev->led_output_size, sizeof(int32_t));
	if (kbd_dev->led_data == NULL) {
		usb_log_error("Error creating buffer for LED output report.\n");
		usb_kbd_destroy(kbd_dev);
		return ENOMEM;
	}

	/* Set LEDs according to initial setup.
	 * Set Idle rate */
	usb_kbd_set_led(hid_dev, kbd_dev);

	usbhid_req_set_idle(usb_device_get_default_pipe(hid_dev->usb_dev),
	    usb_device_get_iface_number(hid_dev->usb_dev), IDLE_RATE);


	kbd_dev->initialized = USB_KBD_STATUS_INITIALIZED;
	usb_log_debug("HID/KBD device structure initialized.\n");

	return EOK;
}


/* API functions                                                              */

/**
 * Initialization of the USB/HID keyboard structure.
 *
 * This functions initializes required structures from the device's descriptors.
 *
 * During initialization, the keyboard is switched into boot protocol, the idle
 * rate is set to 0 (infinity), resulting in the keyboard only reporting event
 * when a key is pressed or released. Finally, the LED lights are turned on
 * according to the default setup of lock keys.
 *
 * @note By default, the keyboards is initialized with Num Lock turned on and
 *       other locks turned off.
 *
 * @param kbd_dev Keyboard device structure to be initialized.
 * @param dev DDF device structure of the keyboard.
 *
 * @retval EOK if successful.
 * @retval EINVAL if some parameter is not given.
 * @return Other value inherited from function usbhid_dev_init().
 */
errno_t usb_kbd_init(usb_hid_dev_t *hid_dev, void **data)
{
	usb_log_debug("Initializing HID/KBD structure...\n");

	if (hid_dev == NULL) {
		usb_log_error(
		    "Failed to init keyboard structure: no structure given.\n");
		return EINVAL;
	}

	/* Create the exposed function. */
	usb_log_debug("Creating DDF function %s...\n", HID_KBD_FUN_NAME);
	ddf_fun_t *fun = usb_device_ddf_fun_create(hid_dev->usb_dev,
	    fun_exposed, HID_KBD_FUN_NAME);
	if (fun == NULL) {
		usb_log_error("Could not create DDF function node.\n");
		return ENOMEM;
	}

	usb_kbd_t *kbd_dev = ddf_fun_data_alloc(fun, sizeof(usb_kbd_t));
	if (kbd_dev == NULL) {
		usb_log_error("Failed to allocate KBD device structure.\n");
		ddf_fun_destroy(fun);
		return ENOMEM;
	}

	errno_t ret = kbd_dev_init(kbd_dev, hid_dev);
	if (ret != EOK) {
		usb_log_error("Failed to initialize KBD device  structure.\n");
		ddf_fun_destroy(fun);
		return ret;
	}

	/* Store the initialized HID device and HID ops
	 * to the DDF function. */
	ddf_fun_set_ops(fun, &kbdops);

	ret = ddf_fun_bind(fun);
	if (ret != EOK) {
		usb_log_error("Could not bind DDF function: %s.\n",
		    str_error(ret));
		usb_kbd_destroy(kbd_dev);
		ddf_fun_destroy(fun);
		return ret;
	}

	usb_log_debug("%s function created. Handle: %" PRIun "\n",
	    HID_KBD_FUN_NAME, ddf_fun_get_handle(fun));

	usb_log_debug("Adding DDF function to category %s...\n",
	    HID_KBD_CATEGORY_NAME);
	ret = ddf_fun_add_to_category(fun, HID_KBD_CATEGORY_NAME);
	if (ret != EOK) {
		usb_log_error(
		    "Could not add DDF function to category %s: %s.\n",
		    HID_KBD_CATEGORY_NAME, str_error(ret));
		usb_kbd_destroy(kbd_dev);
		if (ddf_fun_unbind(fun) == EOK) {
			ddf_fun_destroy(fun);
		} else {
			usb_log_error(
			    "Failed to unbind `%s', will not destroy.\n",
			    ddf_fun_get_name(fun));
		}
		return ret;
	}

	/* Create new fibril for auto-repeat. */
	fid_t fid = fibril_create(usb_kbd_repeat_fibril, kbd_dev);
	if (fid == 0) {
		usb_log_error("Failed to start fibril for KBD auto-repeat");
		usb_kbd_destroy(kbd_dev);
		return ENOMEM;
	}
	fibril_add_ready(fid);
	kbd_dev->fun = fun;
	/* Save the KBD device structure into the HID device structure. */
	*data = kbd_dev;

	return EOK;
}

bool usb_kbd_polling_callback(usb_hid_dev_t *hid_dev, void *data)
{
	if (hid_dev == NULL || data == NULL) {
		/* This means something serious */
		return false;
	}

	usb_kbd_t *kbd_dev = data;
	// TODO: add return value from this function
	usb_kbd_process_data(hid_dev, kbd_dev);

	return true;
}

int usb_kbd_is_initialized(const usb_kbd_t *kbd_dev)
{
	return (kbd_dev->initialized == USB_KBD_STATUS_INITIALIZED);
}

int usb_kbd_is_ready_to_destroy(const usb_kbd_t *kbd_dev)
{
	return (kbd_dev->initialized == USB_KBD_STATUS_TO_DESTROY);
}

/**
 * Properly destroys the USB/HID keyboard structure.
 *
 * @param kbd_dev Pointer to the structure to be destroyed.
 */
void usb_kbd_destroy(usb_kbd_t *kbd_dev)
{
	if (kbd_dev == NULL) {
		return;
	}

	/* Hangup session to the console. */
	if (kbd_dev->client_sess)
		async_hangup(kbd_dev->client_sess);

	//assert(!fibril_mutex_is_locked((*kbd_dev)->repeat_mtx));
	// FIXME - the fibril_mutex_is_locked may not cause
	// fibril scheduling
	while (fibril_mutex_is_locked(&kbd_dev->repeat_mtx)) {}

	/* Free all buffers. */
	free(kbd_dev->keys);
	free(kbd_dev->keys_old);
	free(kbd_dev->led_data);

	usb_hid_report_path_free(kbd_dev->led_path);
	usb_hid_report_output_free(kbd_dev->output_buffer);

	if (kbd_dev->fun) {
		if (ddf_fun_unbind(kbd_dev->fun) != EOK) {
			usb_log_warning("Failed to unbind %s.\n",
			    ddf_fun_get_name(kbd_dev->fun));
		} else {
			usb_log_debug2("%s unbound.\n",
			    ddf_fun_get_name(kbd_dev->fun));
			ddf_fun_destroy(kbd_dev->fun);
		}
	}
}

void usb_kbd_deinit(usb_hid_dev_t *hid_dev, void *data)
{
	if (data != NULL) {
		usb_kbd_t *kbd_dev = data;
		if (usb_kbd_is_initialized(kbd_dev)) {
			kbd_dev->initialized = USB_KBD_STATUS_TO_DESTROY;
			/* Wait for autorepeat */
			async_usleep(CHECK_DELAY);
		}
		usb_kbd_destroy(kbd_dev);
	}
}

errno_t usb_kbd_set_boot_protocol(usb_hid_dev_t *hid_dev)
{
	assert(hid_dev);
	errno_t rc = usb_hid_parse_report_descriptor(
	    &hid_dev->report, USB_KBD_BOOT_REPORT_DESCRIPTOR,
	    sizeof(USB_KBD_BOOT_REPORT_DESCRIPTOR));

	if (rc != EOK) {
		usb_log_error("Failed to parse boot report descriptor: %s\n",
		    str_error(rc));
		return rc;
	}

	rc = usbhid_req_set_protocol(
	    usb_device_get_default_pipe(hid_dev->usb_dev),
	    usb_device_get_iface_number(hid_dev->usb_dev),
	    USB_HID_PROTOCOL_BOOT);

	if (rc != EOK) {
		usb_log_warning("Failed to set boot protocol to the device: "
		    "%s\n", str_error(rc));
		return rc;
	}

	return EOK;
}
/**
 * @}
 */
