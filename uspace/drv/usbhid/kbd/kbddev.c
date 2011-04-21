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
#include <ipc/kbd.h>
#include <async.h>
#include <fibril.h>
#include <fibril_synch.h>

#include <usb/usb.h>
#include <usb/dp.h>
#include <usb/request.h>
#include <usb/classes/hid.h>
#include <usb/pipes.h>
#include <usb/debug.h>
#include <usb/classes/hidparser.h>
#include <usb/classes/classes.h>
#include <usb/classes/hidut.h>
#include <usb/classes/hidreq.h>
#include <usb/classes/hidreport.h>
#include <usb/classes/hid/utled.h>

#include <usb/devdrv.h>

#include "kbddev.h"

#include "layout.h"
#include "conv.h"
#include "kbdrepeat.h"

#include "../usbhid.h"

/*----------------------------------------------------------------------------*/
/** Default modifiers when the keyboard is initialized. */
static const unsigned DEFAULT_ACTIVE_MODS = KM_NUM_LOCK;

///** Boot protocol report size (key part). */
//static const size_t BOOTP_REPORT_SIZE = 6;

///** Boot protocol total report size. */
//static const size_t BOOTP_BUFFER_SIZE = 8;

///** Boot protocol output report size. */
//static const size_t BOOTP_BUFFER_OUT_SIZE = 1;

///** Boot protocol error key code. */
//static const uint8_t BOOTP_ERROR_ROLLOVER = 1;
static const uint8_t ERROR_ROLLOVER = 1;

/** Default idle rate for keyboards. */
static const uint8_t IDLE_RATE = 0;

/** Delay before a pressed key starts auto-repeating. */
static const unsigned int DEFAULT_DELAY_BEFORE_FIRST_REPEAT = 500 * 1000;

/** Delay between two repeats of a pressed key when auto-repeating. */
static const unsigned int DEFAULT_REPEAT_DELAY = 50 * 1000;

/*----------------------------------------------------------------------------*/

/** Keyboard polling endpoint description for boot protocol class. */
usb_endpoint_description_t usb_hid_kbd_poll_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HID,
	.interface_subclass = USB_HID_SUBCLASS_BOOT,
	.interface_protocol = USB_HID_PROTOCOL_KEYBOARD,
	.flags = 0
};

//static usb_endpoint_description_t hid_poll_endpoint_description = {
//	.transfer_type = USB_TRANSFER_INTERRUPT,
//	.direction = USB_DIRECTION_IN,
//	.interface_class = USB_CLASS_HID,
//	.flags = 0
//};

///* Array of endpoints expected on the device, NULL terminated. */
//usb_endpoint_description_t 
//    *usb_kbd_endpoints[USB_KBD_POLL_EP_COUNT + 1] = {
//	&boot_poll_endpoint_description,
//	&hid_poll_endpoint_description,
//	NULL
//};

const char *HID_KBD_FUN_NAME = "keyboard";
const char *HID_KBD_CLASS_NAME = "keyboard";

/*----------------------------------------------------------------------------*/

enum {
	USB_KBD_BOOT_REPORT_DESCRIPTOR_SIZE = 63
};

static const uint8_t USB_KBD_BOOT_REPORT_DESCRIPTOR[
    USB_KBD_BOOT_REPORT_DESCRIPTOR_SIZE] = {
        0x05, 0x01,  // Usage Page (Generic Desktop),
        0x09, 0x06,  // Usage (Keyboard),
        0xA1, 0x01,  // Collection (Application),
        0x75, 0x01,  //   Report Size (1),
        0x95, 0x08,  //   Report Count (8),       
        0x05, 0x07,  //   Usage Page (Key Codes);
        0x19, 0xE0,  //   Usage Minimum (224),
        0x29, 0xE7,  //   Usage Maximum (231),
        0x15, 0x00,  //   Logical Minimum (0),
        0x25, 0x01,  //   Logical Maximum (1),
        0x81, 0x02,  //   Input (Data, Variable, Absolute),   ; Modifier byte
	0x95, 0x01,  //   Report Count (1),
        0x75, 0x08,  //   Report Size (8),
        0x81, 0x01,  //   Input (Constant),                   ; Reserved byte
        0x95, 0x05,  //   Report Count (5),
        0x75, 0x01,  //   Report Size (1),
        0x05, 0x08,  //   Usage Page (Page# for LEDs),
        0x19, 0x01,  //   Usage Minimum (1),
        0x29, 0x05,  //   Usage Maxmimum (5),
        0x91, 0x02,  //   Output (Data, Variable, Absolute),  ; LED report
        0x95, 0x01,  //   Report Count (1),
        0x75, 0x03,  //   Report Size (3),
        0x91, 0x01,  //   Output (Constant),              ; LED report padding
        0x95, 0x06,  //   Report Count (6),
        0x75, 0x08,  //   Report Size (8),
        0x15, 0x00,  //   Logical Minimum (0),
        0x25, 0xff,  //   Logical Maximum (255),
        0x05, 0x07,  //   Usage Page (Key Codes),
        0x19, 0x00,  //   Usage Minimum (0),
        0x29, 0xff,  //   Usage Maximum (255),
        0x81, 0x00,  //   Input (Data, Array),            ; Key arrays (6 bytes)
        0xC0           // End Collection

};

/*----------------------------------------------------------------------------*/

typedef enum usb_kbd_flags {
	USB_KBD_STATUS_UNINITIALIZED = 0,
	USB_KBD_STATUS_INITIALIZED = 1,
	USB_KBD_STATUS_TO_DESTROY = -1
} usb_kbd_flags;

/*----------------------------------------------------------------------------*/

//static void usb_kbd_process_keycodes(const uint8_t *key_codes, size_t count,
//    uint8_t report_id, void *arg);

//static const usb_hid_report_in_callbacks_t usb_kbd_parser_callbacks = {
//	.keyboard = usb_kbd_process_keycodes
//};

/*----------------------------------------------------------------------------*/
/* Keyboard layouts                                                           */
/*----------------------------------------------------------------------------*/

#define NUM_LAYOUTS 3

/** Keyboard layout map. */
static layout_op_t *layout[NUM_LAYOUTS] = {
	&us_qwerty_op,
	&us_dvorak_op,
	&cz_op
};

static int active_layout = 0;

/*----------------------------------------------------------------------------*/
/* Modifier constants                                                         */
/*----------------------------------------------------------------------------*/
/** Mapping of USB modifier key codes to generic modifier key codes. */
static const keycode_t usbhid_modifiers_keycodes[USB_HID_MOD_COUNT] = {
	KC_LCTRL,         /* USB_HID_MOD_LCTRL */
	KC_LSHIFT,        /* USB_HID_MOD_LSHIFT */
	KC_LALT,          /* USB_HID_MOD_LALT */
	0,                /* USB_HID_MOD_LGUI */
	KC_RCTRL,         /* USB_HID_MOD_RCTRL */
	KC_RSHIFT,        /* USB_HID_MOD_RSHIFT */
	KC_RALT,          /* USB_HID_MOD_RALT */
	0,                /* USB_HID_MOD_RGUI */
};

typedef enum usbhid_lock_code {
	USB_KBD_LOCK_NUM = 0x53,
	USB_KBD_LOCK_CAPS = 0x39,
	USB_KBD_LOCK_SCROLL = 0x47,
	USB_KBD_LOCK_COUNT = 3
} usbhid_lock_code;

static const usbhid_lock_code usbhid_lock_codes[USB_KBD_LOCK_COUNT] = {
	USB_KBD_LOCK_NUM,
	USB_KBD_LOCK_CAPS,
	USB_KBD_LOCK_SCROLL
};

/*----------------------------------------------------------------------------*/
/* IPC method handler                                                         */
/*----------------------------------------------------------------------------*/

static void default_connection_handler(ddf_fun_t *, ipc_callid_t, ipc_call_t *);
//ddf_dev_ops_t keyboard_ops = {
//	.default_handler = default_connection_handler
//};

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
	sysarg_t method = IPC_GET_IMETHOD(*icall);
	
	usb_hid_dev_t *hid_dev = (usb_hid_dev_t *)fun->driver_data;
	
	if (hid_dev == NULL || hid_dev->data == NULL) {
		async_answer_0(icallid, EINVAL);
		return;
	}
	
	assert(hid_dev != NULL);
	assert(hid_dev->data != NULL);
	usb_kbd_t *kbd_dev = (usb_kbd_t *)hid_dev->data;

	if (method == IPC_M_CONNECT_TO_ME) {
		int callback = IPC_GET_ARG5(*icall);

		if (kbd_dev->console_phone != -1) {
			async_answer_0(icallid, ELIMIT);
			return;
		}

		kbd_dev->console_phone = callback;
		async_answer_0(icallid, EOK);
		return;
	}
	
	async_answer_0(icallid, EINVAL);
}

/*----------------------------------------------------------------------------*/
/* Key processing functions                                                   */
/*----------------------------------------------------------------------------*/
/**
 * Handles turning of LED lights on and off.
 *
 * In case of USB keyboards, the LEDs are handled in the driver, not in the 
 * device. When there should be a change (lock key was pressed), the driver
 * uses a Set_Report request sent to the device to set the state of the LEDs.
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
	    hid_dev->report, NULL, kbd_dev->led_path, 
	    USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY | USB_HID_PATH_COMPARE_END,
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
		
		field = usb_hid_report_get_sibling(hid_dev->report, field,
		    kbd_dev->led_path, USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY 
		    | USB_HID_PATH_COMPARE_END, USB_HID_REPORT_TYPE_OUTPUT);
	}
	
	// TODO: what about the Report ID?
	int rc = usb_hid_report_output_translate(hid_dev->report, 0,
	    kbd_dev->output_buffer, kbd_dev->output_size);
	
	if (rc != EOK) {
		usb_log_warning("Error translating LED output to output report"
		    ".\n");
		return;
	}
	
	usb_log_debug("Output report buffer: %s\n", 
	    usb_debug_str_buffer(kbd_dev->output_buffer, kbd_dev->output_size, 
	        0));
	
	usbhid_req_set_report(&hid_dev->usb_dev->ctrl_pipe, 
	    hid_dev->usb_dev->interface_no, USB_HID_REPORT_TYPE_OUTPUT, 
	    kbd_dev->output_buffer, kbd_dev->output_size);
}

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
 * @param kbd_dev Keyboard device structure.
 * @param type Type of the event (press / release). Recognized values: 
 *             KEY_PRESS, KEY_RELEASE
 * @param key Key code of the key according to HID Usage Tables.
 */
void usb_kbd_push_ev(usb_hid_dev_t *hid_dev, usb_kbd_t *kbd_dev, int type, 
    unsigned int key)
{
	console_event_t ev;
	unsigned mod_mask;

	/*
	 * These parts are copy-pasted from the AT keyboard driver.
	 *
	 * They definitely require some refactoring, but will keep it for later
	 * when the console and keyboard system is changed in HelenOS.
	 */
	switch (key) {
	case KC_LCTRL: mod_mask = KM_LCTRL; break;
	case KC_RCTRL: mod_mask = KM_RCTRL; break;
	case KC_LSHIFT: mod_mask = KM_LSHIFT; break;
	case KC_RSHIFT: mod_mask = KM_RSHIFT; break;
	case KC_LALT: mod_mask = KM_LALT; break;
	case KC_RALT: mod_mask = KM_RALT; break;
	default: mod_mask = 0; break;
	}

	if (mod_mask != 0) {
		if (type == KEY_PRESS)
			kbd_dev->mods = kbd_dev->mods | mod_mask;
		else
			kbd_dev->mods = kbd_dev->mods & ~mod_mask;
	}

	switch (key) {
	case KC_CAPS_LOCK: mod_mask = KM_CAPS_LOCK; break;
	case KC_NUM_LOCK: mod_mask = KM_NUM_LOCK; break;
	case KC_SCROLL_LOCK: mod_mask = KM_SCROLL_LOCK; break;
	default: mod_mask = 0; break;
	}

	if (mod_mask != 0) {
		if (type == KEY_PRESS) {
			/*
			 * Only change lock state on transition from released
			 * to pressed. This prevents autorepeat from messing
			 * up the lock state.
			 */
			unsigned int locks_old = kbd_dev->lock_keys;
			
			kbd_dev->mods = 
			    kbd_dev->mods ^ (mod_mask & ~kbd_dev->lock_keys);
			kbd_dev->lock_keys = kbd_dev->lock_keys | mod_mask;

			/* Update keyboard lock indicator lights. */
			if (kbd_dev->lock_keys != locks_old 
			    && hid_dev != NULL) { // ugly hack
				usb_kbd_set_led(hid_dev, kbd_dev);
			}
		} else {
			kbd_dev->lock_keys = kbd_dev->lock_keys & ~mod_mask;
		}
	}

	if (key == KC_CAPS_LOCK || key == KC_NUM_LOCK || key == KC_SCROLL_LOCK) {
		// do not send anything to the console, this is our business
		return;
	}
	
	if (type == KEY_PRESS && (kbd_dev->mods & KM_LCTRL) && key == KC_F1) {
		active_layout = 0;
		layout[active_layout]->reset();
		return;
	}

	if (type == KEY_PRESS && (kbd_dev->mods & KM_LCTRL) && key == KC_F2) {
		active_layout = 1;
		layout[active_layout]->reset();
		return;
	}

	if (type == KEY_PRESS && (kbd_dev->mods & KM_LCTRL) && key == KC_F3) {
		active_layout = 2;
		layout[active_layout]->reset();
		return;
	}
	
	ev.type = type;
	ev.key = key;
	ev.mods = kbd_dev->mods;

	ev.c = layout[active_layout]->parse_ev(&ev);

	usb_log_debug2("Sending key %d to the console\n", ev.key);
	if (kbd_dev->console_phone < 0) {
		usb_log_warning(
		    "Connection to console not ready, key discarded.\n");
		return;
	}
	
	async_msg_4(kbd_dev->console_phone, KBD_EVENT, ev.type, ev.key, 
	    ev.mods, ev.c);
}

/*----------------------------------------------------------------------------*/

static inline int usb_kbd_is_lock(unsigned int key_code) 
{
	return (key_code == KC_NUM_LOCK
	    || key_code == KC_SCROLL_LOCK
	    || key_code == KC_CAPS_LOCK);
}

/*----------------------------------------------------------------------------*/
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
    usb_kbd_t *kbd_dev/*, const uint8_t *key_codes, size_t count*/)
{
	unsigned int key;
	unsigned int i, j;
	
	/*
	 * First of all, check if the kbd have reported phantom state.
	 *
	 * As there is no way to distinguish keys from modifiers, we do not have
	 * a way to check that 'all keys report Error Rollover'. We thus check
	 * if there is at least one such error and in such case we ignore the
	 * whole input report.
	 */
	i = 0;
	while (i < kbd_dev->key_count && kbd_dev->keys[i] != ERROR_ROLLOVER) {
		++i;
	}
	if (i != kbd_dev->key_count) {
		usb_log_debug("Phantom state occured.\n");
		// phantom state, do nothing
		return;
	}
	
	/*
	 * 1) Key releases
	 */
	for (j = 0; j < kbd_dev->key_count; ++j) {
		// try to find the old key in the new key list
		i = 0;
		while (i < kbd_dev->key_count
		    && kbd_dev->keys[i] != kbd_dev->keys_old[j]) {
			++i;
		}
		
		if (i == kbd_dev->key_count) {
			// not found, i.e. the key was released
			key = usbhid_parse_scancode(kbd_dev->keys_old[j]);
			if (!usb_kbd_is_lock(key)) {
				usb_kbd_repeat_stop(kbd_dev, key);
			}
			usb_kbd_push_ev(hid_dev, kbd_dev, KEY_RELEASE, key);
			usb_log_debug2("Key released: %d\n", key);
		} else {
			// found, nothing happens
		}
	}
	
	/*
	 * 1) Key presses
	 */
	for (i = 0; i < kbd_dev->key_count; ++i) {
		// try to find the new key in the old key list
		j = 0;
		while (j < kbd_dev->key_count 
		    && kbd_dev->keys_old[j] != kbd_dev->keys[i]) { 
			++j;
		}
		
		if (j == kbd_dev->key_count) {
			// not found, i.e. new key pressed
			key = usbhid_parse_scancode(kbd_dev->keys[i]);
			usb_log_debug2("Key pressed: %d (keycode: %d)\n", key,
			    kbd_dev->keys[i]);
			usb_kbd_push_ev(hid_dev, kbd_dev, KEY_PRESS, key);
			if (!usb_kbd_is_lock(key)) {
				usb_kbd_repeat_start(kbd_dev, key);
			}
		} else {
			// found, nothing happens
		}
	}
	
//	usb_log_debug("Old keys: ");
//	for (i = 0; i < kbd_dev->key_count; ++i) {
//		usb_log_debug("%d ", kbd_dev->keys_old[i]);
//	}
//	usb_log_debug("\n");
	
	
//	usb_log_debug("New keys: ");
//	for (i = 0; i < kbd_dev->key_count; ++i) {
//		usb_log_debug("%d ", kbd_dev->keys[i]);
//	}
//	usb_log_debug("\n");
	
	memcpy(kbd_dev->keys_old, kbd_dev->keys, kbd_dev->key_count * 4);
	
	usb_log_debug2("New stored keys: ");
	for (i = 0; i < kbd_dev->key_count; ++i) {
		usb_log_debug2("%d ", kbd_dev->keys_old[i]);
	}
	usb_log_debug2("\n");
}

/*----------------------------------------------------------------------------*/
/* Callbacks for parser                                                       */
/*----------------------------------------------------------------------------*/
/**
 * Callback function for the HID report parser.
 *
 * This function is called by the HID report parser with the parsed report.
 * The parsed report is used to check if any events occured (key was pressed or
 * released, modifier was pressed or released).
 *
 * @param key_codes Parsed keyboard report - codes of currently pressed keys 
 *                  according to HID Usage Tables.
 * @param count Number of key codes in report (size of the report).
 * @param report_id
 * @param arg User-specified argument. Expects pointer to the keyboard device
 *            structure representing the keyboard.
 *
 * @sa usb_kbd_check_key_changes(), usb_kbd_check_modifier_changes()
 */
//static void usb_kbd_process_keycodes(const uint8_t *key_codes, size_t count,
//    uint8_t report_id, void *arg)
//{
//	if (arg == NULL) {
//		usb_log_warning("Missing argument in callback "
//		    "usbhid_process_keycodes().\n");
//		return;
//	}
	
//	usb_hid_dev_t *hid_dev = (usb_hid_dev_t *)arg;
	
//	if (hid_dev->data == NULL) {
//		usb_log_warning("Missing KBD device structure in callback.\n");
//		return;
//	}
	
//	usb_kbd_t *kbd_dev = (usb_kbd_t *)hid_dev->data;

//	usb_log_debug("Got keys from parser (report id: %u): %s\n", 
//	    report_id, usb_debug_str_buffer(key_codes, count, 0));
	
//	if (count != kbd_dev->key_count) {
//		usb_log_warning("Number of received keycodes (%zu) differs from"
//		    " expected (%zu).\n", count, kbd_dev->key_count);
//		return;
//	}
	
//	///usb_kbd_check_modifier_changes(kbd_dev, key_codes, count);
//	usb_kbd_check_key_changes(hid_dev, kbd_dev, key_codes, count);
//}

/*----------------------------------------------------------------------------*/
/* General kbd functions                                                      */
/*----------------------------------------------------------------------------*/
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
static void usb_kbd_process_data(usb_hid_dev_t *hid_dev,
                                 uint8_t *buffer, size_t actual_size)
{
	assert(hid_dev->report != NULL);
	assert(hid_dev != NULL);
	assert(hid_dev->data != NULL);
	
	usb_kbd_t *kbd_dev = (usb_kbd_t *)hid_dev->data;

	usb_log_debug("Calling usb_hid_parse_report() with "
	    "buffer %s\n", usb_debug_str_buffer(buffer, actual_size, 0));
	
//	int rc = usb_hid_boot_keyboard_input_report(buffer, actual_size,
//	    callbacks, kbd_dev);
	usb_hid_report_path_t *path = usb_hid_report_path();
	usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_KEYBOARD, 0);
	//usb_hid_report_path_set_report_id(path, 0);

	uint8_t report_id;
	int rc = usb_hid_parse_report(hid_dev->report, buffer, actual_size, 
	    &report_id);
	usb_hid_report_path_set_report_id (path, report_id);
	
	usb_hid_report_field_t *field = usb_hid_report_get_sibling(
	    hid_dev->report, NULL, path, USB_HID_PATH_COMPARE_END 
	    | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY, USB_HID_REPORT_TYPE_INPUT);

	usb_hid_report_path_free(path);
	
	if (rc != EOK) {
		usb_log_warning("Error in usb_hid_parse_report():"
		    "%s\n", str_error(rc));
	}
	
	// fill in the currently pressed keys
	
	field = usb_hid_report_get_sibling(hid_dev->report, NULL, path, 
	    USB_HID_PATH_COMPARE_END | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY, 
	    USB_HID_REPORT_TYPE_INPUT);
	unsigned i = 0;
	
	// TODO: remove this hack - skipping first field
	field = usb_hid_report_get_sibling(hid_dev->report, field, path, 
	    USB_HID_PATH_COMPARE_END 
	    | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY, 
	    USB_HID_REPORT_TYPE_INPUT);
	
	while (field != NULL) {
		usb_log_debug2("FIELD (%p) - VALUE(%d) USAGE(%u)\n", 
		    field, field->value, field->usage);
		
		assert(i < kbd_dev->key_count);
//		if (i == kbd_dev->key_count) {
//			break;
//		}
		
		// save the key usage
		kbd_dev->keys[i] = field->usage;
		usb_log_debug2("Saved %u. key usage %d\n", i, kbd_dev->keys[i]);
		
		++i;
		field = usb_hid_report_get_sibling(hid_dev->report, field, path, 
		    USB_HID_PATH_COMPARE_END 
		    | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY, 
		    USB_HID_REPORT_TYPE_INPUT);
	}
	
	usb_hid_report_path_free(path);
	
	usb_kbd_check_key_changes(hid_dev, kbd_dev);
}

/*----------------------------------------------------------------------------*/
/* HID/KBD structure manipulation                                             */
/*----------------------------------------------------------------------------*/

static void usb_kbd_mark_unusable(usb_kbd_t *kbd_dev)
{
	kbd_dev->initialized = USB_KBD_STATUS_TO_DESTROY;
}

/*----------------------------------------------------------------------------*/

/**
 * Creates a new USB/HID keyboard structure.
 *
 * The structure returned by this function is not initialized. Use 
 * usb_kbd_init() to initialize it prior to polling.
 *
 * @return New uninitialized structure for representing a USB/HID keyboard or
 *         NULL if not successful (memory error).
 */
static usb_kbd_t *usb_kbd_new(void)
{
	usb_kbd_t *kbd_dev = 
	    (usb_kbd_t *)calloc(1, sizeof(usb_kbd_t));

	if (kbd_dev == NULL) {
		usb_log_fatal("No memory!\n");
		return NULL;
	}
	
	kbd_dev->console_phone = -1;
	kbd_dev->initialized = USB_KBD_STATUS_UNINITIALIZED;
	
	return kbd_dev;
}

/*----------------------------------------------------------------------------*/
/* API functions                                                              */
/*----------------------------------------------------------------------------*/
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
int usb_kbd_init(usb_hid_dev_t *hid_dev)
{
	usb_log_debug("Initializing HID/KBD structure...\n");
	
	if (hid_dev == NULL) {
		usb_log_error("Failed to init keyboard structure: no structure"
		    " given.\n");
		return EINVAL;
	}
	
	usb_kbd_t *kbd_dev = usb_kbd_new();
	if (kbd_dev == NULL) {
		usb_log_error("Error while creating USB/HID KBD device "
		    "structure.\n");
		return ENOMEM;  // TODO: some other code??
	}
	
	/*
	 * TODO: make more general
	 */
	usb_hid_report_path_t *path = usb_hid_report_path();
	usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_KEYBOARD, 0);
	
	usb_hid_report_path_set_report_id(path, 0);
	
	kbd_dev->key_count = usb_hid_report_input_length(
	    hid_dev->report, path, 
	    USB_HID_PATH_COMPARE_END | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY);
	usb_hid_report_path_free(path);
	
	usb_log_debug("Size of the input report: %zu\n", kbd_dev->key_count);
	
	kbd_dev->keys = (int32_t *)calloc(kbd_dev->key_count, sizeof(int32_t));
	
	if (kbd_dev->keys == NULL) {
		usb_log_fatal("No memory!\n");
		free(kbd_dev);
		return ENOMEM;
	}
	
	kbd_dev->keys_old = 
		(int32_t *)calloc(kbd_dev->key_count, sizeof(int32_t));
	
	if (kbd_dev->keys_old == NULL) {
		usb_log_fatal("No memory!\n");
		free(kbd_dev->keys);
		free(kbd_dev);
		return ENOMEM;
	}
	
	/*
	 * Output report
	 */
	kbd_dev->output_size = 0;
	kbd_dev->output_buffer = usb_hid_report_output(hid_dev->report, 
	    &kbd_dev->output_size, 0);
	if (kbd_dev->output_buffer == NULL) {
		usb_log_warning("Error creating output report buffer.\n");
		free(kbd_dev->keys);
		return ENOMEM;  /* TODO: other error code */
	}
	
	usb_log_debug("Output buffer size: %zu\n", kbd_dev->output_size);
	
	kbd_dev->led_path = usb_hid_report_path();
	usb_hid_report_path_append_item(
	    kbd_dev->led_path, USB_HIDUT_PAGE_LED, 0);
	
	kbd_dev->led_output_size = usb_hid_report_output_size(hid_dev->report, 
	    kbd_dev->led_path, 
	    USB_HID_PATH_COMPARE_END | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY);
	
	usb_log_debug("Output report size (in items): %zu\n", 
	    kbd_dev->led_output_size);
	
	kbd_dev->led_data = (int32_t *)calloc(
	    kbd_dev->led_output_size, sizeof(int32_t));
	
	if (kbd_dev->led_data == NULL) {
		usb_log_warning("Error creating buffer for LED output report."
		    "\n");
		free(kbd_dev->keys);
		usb_hid_report_output_free(kbd_dev->output_buffer);
		free(kbd_dev);
		return ENOMEM;
	}
	
	/*
	 * Modifiers and locks
	 */	
	kbd_dev->modifiers = 0;
	kbd_dev->mods = DEFAULT_ACTIVE_MODS;
	kbd_dev->lock_keys = 0;
	
	/*
	 * Autorepeat
	 */	
	kbd_dev->repeat.key_new = 0;
	kbd_dev->repeat.key_repeated = 0;
	kbd_dev->repeat.delay_before = DEFAULT_DELAY_BEFORE_FIRST_REPEAT;
	kbd_dev->repeat.delay_between = DEFAULT_REPEAT_DELAY;
	
	kbd_dev->repeat_mtx = (fibril_mutex_t *)(
	    malloc(sizeof(fibril_mutex_t)));
	if (kbd_dev->repeat_mtx == NULL) {
		usb_log_fatal("No memory!\n");
		free(kbd_dev->keys);
		usb_hid_report_output_free(kbd_dev->output_buffer);
		free(kbd_dev);
		return ENOMEM;
	}
	
	fibril_mutex_initialize(kbd_dev->repeat_mtx);
	
	// save the KBD device structure into the HID device structure
	hid_dev->data = kbd_dev;
	
	// set handler for incoming calls
	hid_dev->ops.default_handler = default_connection_handler;
	
	/*
	 * Set LEDs according to initial setup.
	 * Set Idle rate
	 */
	usb_kbd_set_led(hid_dev, kbd_dev);
	
	usbhid_req_set_idle(&hid_dev->usb_dev->ctrl_pipe, 
	    hid_dev->usb_dev->interface_no, IDLE_RATE);
	
	/*
	 * Create new fibril for auto-repeat
	 */
	fid_t fid = fibril_create(usb_kbd_repeat_fibril, kbd_dev);
	if (fid == 0) {
		usb_log_error("Failed to start fibril for KBD auto-repeat");
		return ENOMEM;
	}
	fibril_add_ready(fid);
	
	kbd_dev->initialized = USB_KBD_STATUS_INITIALIZED;
	usb_log_debug("HID/KBD device structure initialized.\n");
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

bool usb_kbd_polling_callback(usb_hid_dev_t *hid_dev, uint8_t *buffer,
     size_t buffer_size)
{
	if (hid_dev == NULL || buffer == NULL) {
		// do not continue polling (???)
		return false;
	}
	
	// TODO: add return value from this function
	usb_kbd_process_data(hid_dev, buffer, buffer_size);
	
	return true;
}

/*----------------------------------------------------------------------------*/

int usb_kbd_is_initialized(const usb_kbd_t *kbd_dev)
{
	return (kbd_dev->initialized == USB_KBD_STATUS_INITIALIZED);
}

/*----------------------------------------------------------------------------*/

int usb_kbd_is_ready_to_destroy(const usb_kbd_t *kbd_dev)
{
	return (kbd_dev->initialized == USB_KBD_STATUS_TO_DESTROY);
}

/*----------------------------------------------------------------------------*/
/**
 * Properly destroys the USB/HID keyboard structure.
 *
 * @param kbd_dev Pointer to the structure to be destroyed.
 */
void usb_kbd_free(usb_kbd_t **kbd_dev)
{
	if (kbd_dev == NULL || *kbd_dev == NULL) {
		return;
	}
	
	// hangup phone to the console
	async_hangup((*kbd_dev)->console_phone);
	
	if ((*kbd_dev)->repeat_mtx != NULL) {
		/* TODO: replace by some check and wait */
		assert(!fibril_mutex_is_locked((*kbd_dev)->repeat_mtx));
		free((*kbd_dev)->repeat_mtx);
	}
	
	// free the output buffer
	usb_hid_report_output_free((*kbd_dev)->output_buffer);

	free(*kbd_dev);
	*kbd_dev = NULL;
}

/*----------------------------------------------------------------------------*/

void usb_kbd_deinit(usb_hid_dev_t *hid_dev)
{
	if (hid_dev == NULL) {
		return;
	}
	
	if (hid_dev->data != NULL) {
		usb_kbd_t *kbd_dev = (usb_kbd_t *)hid_dev->data;
		if (usb_kbd_is_initialized(kbd_dev)) {
			usb_kbd_mark_unusable(kbd_dev);
		} else {
			usb_kbd_free(&kbd_dev);
		}
	}
}

/*----------------------------------------------------------------------------*/

int usb_kbd_set_boot_protocol(usb_hid_dev_t *hid_dev)
{
	int rc = usb_hid_parse_report_descriptor(hid_dev->report, 
	    USB_KBD_BOOT_REPORT_DESCRIPTOR, 
	    USB_KBD_BOOT_REPORT_DESCRIPTOR_SIZE);
	
	if (rc != EOK) {
		usb_log_error("Failed to parse boot report descriptor: %s\n",
		    str_error(rc));
		return rc;
	}
	
	rc = usbhid_req_set_protocol(&hid_dev->usb_dev->ctrl_pipe, 
	    hid_dev->usb_dev->interface_no, USB_HID_PROTOCOL_BOOT);
	
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
