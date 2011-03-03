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
#include <fibril.h>

#include <io/keycode.h>
#include <ipc/kbd.h>
#include <async.h>

#include <usb/usb.h>
#include <usb/classes/hid.h>
#include <usb/pipes.h>
#include <usb/debug.h>
#include <usb/classes/hidparser.h>
#include <usb/classes/classes.h>

#include "kbddev.h"
#include "hiddev.h"
#include "hidreq.h"
#include "layout.h"
#include "conv.h"

/*----------------------------------------------------------------------------*/

static const unsigned DEFAULT_ACTIVE_MODS = KM_NUM_LOCK;
static const size_t BOOTP_REPORT_SIZE = 6;
static const size_t BOOTP_BUFFER_SIZE = 8;
static const size_t BOOTP_BUFFER_OUT_SIZE = 1;

/** Keyboard polling endpoint description for boot protocol class. */
static usb_endpoint_description_t poll_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HID,
	.interface_subclass = USB_HID_SUBCLASS_BOOT,
	.interface_protocol = USB_HID_PROTOCOL_KEYBOARD,
	.flags = 0
};

/*----------------------------------------------------------------------------*/
/* Keyboard layouts                                                           */
/*----------------------------------------------------------------------------*/

#define NUM_LAYOUTS 3

static layout_op_t *layout[NUM_LAYOUTS] = {
	&us_qwerty_op,
	&us_dvorak_op,
	&cz_op
};

static int active_layout = 0;

/*----------------------------------------------------------------------------*/
/* Modifier constants                                                         */
/*----------------------------------------------------------------------------*/

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

/*----------------------------------------------------------------------------*/
/* IPC method handler                                                         */
/*----------------------------------------------------------------------------*/

static void default_connection_handler(ddf_fun_t *, ipc_callid_t, ipc_call_t *);
static ddf_dev_ops_t keyboard_ops = {
	.default_handler = default_connection_handler
};

/** Default handler for IPC methods not handled by DDF.
 *
 * @param dev Device handling the call.
 * @param icallid Call id.
 * @param icall Call data.
 */
void default_connection_handler(ddf_fun_t *fun,
    ipc_callid_t icallid, ipc_call_t *icall)
{
	sysarg_t method = IPC_GET_IMETHOD(*icall);
	
	usbhid_kbd_t *kbd_dev = (usbhid_kbd_t *)fun->driver_data;
	assert(kbd_dev != NULL);

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

static void usbhid_kbd_set_led(usbhid_kbd_t *kbd_dev) 
{
	uint8_t buffer[BOOTP_BUFFER_OUT_SIZE];
	int rc= 0;
	unsigned i;
	
	memset(buffer, 0, BOOTP_BUFFER_OUT_SIZE);
	uint8_t leds = 0;

	if (kbd_dev->mods & KM_NUM_LOCK) {
		leds |= USB_HID_LED_NUM_LOCK;
	}
	
	if (kbd_dev->mods & KM_CAPS_LOCK) {
		leds |= USB_HID_LED_CAPS_LOCK;
	}
	
	if (kbd_dev->mods & KM_SCROLL_LOCK) {
		leds |= USB_HID_LED_SCROLL_LOCK;
	}

	// TODO: COMPOSE and KANA
	
	usb_log_debug("Creating output report.\n");
	usb_log_debug("Leds: 0x%x\n", leds);
	if ((rc = usb_hid_boot_keyboard_output_report(
	    leds, buffer, BOOTP_BUFFER_OUT_SIZE)) != EOK) {
		usb_log_warning("Error composing output report to the keyboard:"
		    "%s.\n", str_error(rc));
		return;
	}
	
	// TODO: REFACTOR!!!
	
	usb_log_debug("Output report buffer: ");
	for (i = 0; i < BOOTP_BUFFER_OUT_SIZE; ++i) {
		usb_log_debug("0x%x ", buffer[i]);
	}
	usb_log_debug("\n");
	
	uint16_t value = 0;
	value |= (USB_HID_REPORT_TYPE_OUTPUT << 8);

	assert(kbd_dev->hid_dev != NULL);
	assert(kbd_dev->hid_dev->initialized);
	usbhid_req_set_report(kbd_dev->hid_dev, value, buffer, 
	    BOOTP_BUFFER_OUT_SIZE);
}

/*----------------------------------------------------------------------------*/

static void usbhid_kbd_push_ev(usbhid_kbd_t *kbd_dev, int type, 
    unsigned int key)
{
	console_event_t ev;
	unsigned mod_mask;

	// TODO: replace by our own parsing?? or are the key codes identical??
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
		usb_log_debug2("\n\nChanging mods and lock keys\n");
		usb_log_debug2("\nmods before: 0x%x\n", kbd_dev->mods);
		usb_log_debug2("\nLock keys before:0x%x\n\n", 
		    kbd_dev->lock_keys);
		
		if (type == KEY_PRESS) {
			usb_log_debug2("\nKey pressed.\n");
			/*
			 * Only change lock state on transition from released
			 * to pressed. This prevents autorepeat from messing
			 * up the lock state.
			 */
			kbd_dev->mods = 
			    kbd_dev->mods ^ (mod_mask & ~kbd_dev->lock_keys);
			kbd_dev->lock_keys = kbd_dev->lock_keys | mod_mask;

			/* Update keyboard lock indicator lights. */
 			usbhid_kbd_set_led(kbd_dev);
		} else {
			usb_log_debug2("\nKey released.\n");
			kbd_dev->lock_keys = kbd_dev->lock_keys & ~mod_mask;
		}
	}

	usb_log_debug2("\n\nmods after: 0x%x\n", kbd_dev->mods);
	usb_log_debug2("\nLock keys after: 0x%x\n\n", kbd_dev->lock_keys);
	
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
	
	if (ev.mods & KM_NUM_LOCK) {
		usb_log_debug("\n\nNum Lock turned on.\n\n");
	}

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

static void usbhid_kbd_check_modifier_changes(usbhid_kbd_t *kbd_dev,
    uint8_t modifiers)
{
	/*
	 * TODO: why the USB keyboard has NUM_, SCROLL_ and CAPS_LOCK
	 *       both as modifiers and as keys with their own scancodes???
	 *
	 * modifiers should be sent as normal keys to usbhid_parse_scancode()!!
	 * so maybe it would be better if I received it from report parser in 
	 * that way
	 */
	
	int i;
	for (i = 0; i < USB_HID_MOD_COUNT; ++i) {
		if ((modifiers & usb_hid_modifiers_consts[i]) &&
		    !(kbd_dev->modifiers & usb_hid_modifiers_consts[i])) {
			// modifier pressed
			if (usbhid_modifiers_keycodes[i] != 0) {
				usbhid_kbd_push_ev(kbd_dev, KEY_PRESS, 
				    usbhid_modifiers_keycodes[i]);
			}
		} else if (!(modifiers & usb_hid_modifiers_consts[i]) &&
		    (kbd_dev->modifiers & usb_hid_modifiers_consts[i])) {
			// modifier released
			if (usbhid_modifiers_keycodes[i] != 0) {
				usbhid_kbd_push_ev(kbd_dev, KEY_RELEASE, 
				    usbhid_modifiers_keycodes[i]);
			}
		}	// no change
	}
	
	kbd_dev->modifiers = modifiers;
}

/*----------------------------------------------------------------------------*/

static void usbhid_kbd_check_key_changes(usbhid_kbd_t *kbd_dev, 
    const uint8_t *key_codes)
{
	// TODO: phantom state!!
	
	unsigned int key;
	unsigned int i, j;
	
	// TODO: quite dummy right now, think of better implementation
	
	/*
	 * 1) Key releases
	 */
	for (j = 0; j < kbd_dev->keycode_count; ++j) {
		// try to find the old key in the new key list
		i = 0;
		while (i < kbd_dev->keycode_count
		    && key_codes[i] != kbd_dev->keycodes[j]) {
			++i;
		}
		
		if (i == kbd_dev->keycode_count) {
			// not found, i.e. the key was released
			key = usbhid_parse_scancode(kbd_dev->keycodes[j]);
			usbhid_kbd_push_ev(kbd_dev, KEY_RELEASE, key);
			usb_log_debug2("\nKey released: %d\n", key);
		} else {
			// found, nothing happens
		}
	}
	
	/*
	 * 1) Key presses
	 */
	for (i = 0; i < kbd_dev->keycode_count; ++i) {
		// try to find the new key in the old key list
		j = 0;
		while (j < kbd_dev->keycode_count 
		    && kbd_dev->keycodes[j] != key_codes[i]) { 
			++j;
		}
		
		if (j == kbd_dev->keycode_count) {
			// not found, i.e. new key pressed
			key = usbhid_parse_scancode(key_codes[i]);
			usb_log_debug2("\nKey pressed: %d (keycode: %d)\n", key,
			    key_codes[i]);
			usbhid_kbd_push_ev(kbd_dev, KEY_PRESS, key);
		} else {
			// found, nothing happens
		}
	}
	
	memcpy(kbd_dev->keycodes, key_codes, kbd_dev->keycode_count);
	
	usb_log_debug2("\nNew stored keycodes: ");
	for (i = 0; i < kbd_dev->keycode_count; ++i) {
		usb_log_debug2("%d ", kbd_dev->keycodes[i]);
	}
}

/*----------------------------------------------------------------------------*/
/* Callbacks for parser                                                       */
/*----------------------------------------------------------------------------*/

static void usbhid_kbd_process_keycodes(const uint8_t *key_codes, size_t count,
    uint8_t modifiers, void *arg)
{
	if (arg == NULL) {
		usb_log_warning("Missing argument in callback "
		    "usbhid_process_keycodes().\n");
		return;
	}

	usb_log_debug2("Got keys from parser: ");
	unsigned i;
	for (i = 0; i < count; ++i) {
		usb_log_debug2("%d ", key_codes[i]);
	}
	usb_log_debug2("\n");
	
	usbhid_kbd_t *kbd_dev = (usbhid_kbd_t *)arg;
	assert(kbd_dev != NULL);
	
	if (count != kbd_dev->keycode_count) {
		usb_log_warning("Number of received keycodes (%d) differs from"
		    " expected number (%d).\n", count, kbd_dev->keycode_count);
		return;
	}
	
	usbhid_kbd_check_modifier_changes(kbd_dev, modifiers);
	usbhid_kbd_check_key_changes(kbd_dev, key_codes);
}

/*----------------------------------------------------------------------------*/
/* General kbd functions                                                      */
/*----------------------------------------------------------------------------*/

static void usbhid_kbd_process_data(usbhid_kbd_t *kbd_dev,
                                    uint8_t *buffer, size_t actual_size)
{
	usb_hid_report_in_callbacks_t *callbacks =
	    (usb_hid_report_in_callbacks_t *)malloc(
	        sizeof(usb_hid_report_in_callbacks_t));
	
	callbacks->keyboard = usbhid_kbd_process_keycodes;

	//usb_hid_parse_report(kbd_dev->parser, buffer, actual_size, callbacks, 
	//    NULL);
	/*usb_log_debug2("Calling usb_hid_boot_keyboard_input_report() with size"
	    " %zu\n", actual_size);*/
	//dump_buffer("bufffer: ", buffer, actual_size);
	
	int rc = usb_hid_boot_keyboard_input_report(buffer, actual_size,
	    callbacks, kbd_dev);
	
	if (rc != EOK) {
		usb_log_warning("Error in usb_hid_boot_keyboard_input_report():"
		    "%s\n", str_error(rc));
	}
}

/*----------------------------------------------------------------------------*/
/* HID/KBD structure manipulation                                             */
/*----------------------------------------------------------------------------*/

static usbhid_kbd_t *usbhid_kbd_new(void)
{
	usbhid_kbd_t *kbd_dev = 
	    (usbhid_kbd_t *)malloc(sizeof(usbhid_kbd_t));

	if (kbd_dev == NULL) {
		usb_log_fatal("No memory!\n");
		return NULL;
	}
	
	memset(kbd_dev, 0, sizeof(usbhid_kbd_t));
	
	kbd_dev->hid_dev = usbhid_dev_new();
	if (kbd_dev->hid_dev == NULL) {
		usb_log_fatal("Could not create HID device structure.\n");
		return NULL;
	}
	
	kbd_dev->console_phone = -1;
	kbd_dev->initialized = 0;
	
	return kbd_dev;
}

/*----------------------------------------------------------------------------*/

static void usbhid_kbd_free(usbhid_kbd_t **kbd_dev)
{
	if (kbd_dev == NULL || *kbd_dev == NULL) {
		return;
	}
	
	// hangup phone to the console
	async_hangup((*kbd_dev)->console_phone);
	
	if ((*kbd_dev)->hid_dev != NULL) {
		usbhid_dev_free(&(*kbd_dev)->hid_dev);
		assert((*kbd_dev)->hid_dev == NULL);
	}
	
	free(*kbd_dev);
	*kbd_dev = NULL;
}

/*----------------------------------------------------------------------------*/

static int usbhid_kbd_init(usbhid_kbd_t *kbd_dev, ddf_dev_t *dev)
{
	int rc;
	
	usb_log_info("Initializing HID/KBD structure...\n");
	
	if (kbd_dev == NULL) {
		usb_log_error("Failed to init keyboard structure: no structure"
		    " given.\n");
		return EINVAL;
	}
	
	if (dev == NULL) {
		usb_log_error("Failed to init keyboard structure: no device"
		    " given.\n");
		return EINVAL;
	}
	
	if (kbd_dev->initialized) {
		usb_log_warning("Keyboard structure already initialized.\n");
		return EINVAL;
	}
	
	rc = usbhid_dev_init(kbd_dev->hid_dev, dev, &poll_endpoint_description);
	
	if (rc != EOK) {
		usb_log_error("Failed to initialize HID device structure: %s\n",
		   str_error(rc));
		return rc;
	}
	
	assert(kbd_dev->hid_dev->initialized);
	
	// save the size of the report (boot protocol report by default)
	kbd_dev->keycode_count = BOOTP_REPORT_SIZE;
	kbd_dev->keycodes = (uint8_t *)calloc(
	    kbd_dev->keycode_count, sizeof(uint8_t));
	
	if (kbd_dev->keycodes == NULL) {
		usb_log_fatal("No memory!\n");
		return rc;
	}
	
	kbd_dev->modifiers = 0;
	kbd_dev->mods = DEFAULT_ACTIVE_MODS;
	kbd_dev->lock_keys = 0;
	
	/*
	 * Set boot protocol.
	 * Set LEDs according to initial setup.
	 */
	assert(kbd_dev->hid_dev != NULL);
	assert(kbd_dev->hid_dev->initialized);
	usbhid_req_set_protocol(kbd_dev->hid_dev, USB_HID_PROTOCOL_BOOT);
	
	usbhid_kbd_set_led(kbd_dev);
	
	kbd_dev->initialized = 1;
	usb_log_info("HID/KBD device structure initialized.\n");
	
	return EOK;
}

/*----------------------------------------------------------------------------*/
/* HID/KBD polling                                                            */
/*----------------------------------------------------------------------------*/

static void usbhid_kbd_poll(usbhid_kbd_t *kbd_dev)
{
	int rc, sess_rc;
	uint8_t buffer[BOOTP_BUFFER_SIZE];
	size_t actual_size;
	
	usb_log_info("Polling keyboard...\n");
	
	if (!kbd_dev->initialized) {
		usb_log_error("HID/KBD device not initialized!\n");
		return;
	}
	
	assert(kbd_dev->hid_dev != NULL);
	assert(kbd_dev->hid_dev->initialized);

	while (true) {
		async_usleep(1000 * 10);

		sess_rc = usb_endpoint_pipe_start_session(
		    &kbd_dev->hid_dev->poll_pipe);
		if (sess_rc != EOK) {
			usb_log_warning("Failed to start a session: %s.\n",
			    str_error(sess_rc));
			continue;
		}

		rc = usb_endpoint_pipe_read(&kbd_dev->hid_dev->poll_pipe, 
		    buffer, BOOTP_BUFFER_SIZE, &actual_size);
		
		sess_rc = usb_endpoint_pipe_end_session(
		    &kbd_dev->hid_dev->poll_pipe);

		if (rc != EOK) {
			usb_log_warning("Error polling the keyboard: %s.\n",
			    str_error(rc));
			continue;
		}

		if (sess_rc != EOK) {
			usb_log_warning("Error closing session: %s.\n",
			    str_error(sess_rc));
			continue;
		}

		/*
		 * If the keyboard answered with NAK, it returned no data.
		 * This implies that no change happened since last query.
		 */
		if (actual_size == 0) {
			usb_log_debug("Keyboard returned NAK\n");
			continue;
		}

		/*
		 * TODO: Process pressed keys.
		 */
		usb_log_debug("Calling usbhid_kbd_process_data()\n");
		usbhid_kbd_process_data(kbd_dev, buffer, actual_size);
	}

	// not reached
	assert(0);
}

/*----------------------------------------------------------------------------*/

static int usbhid_kbd_fibril(void *arg)
{
	if (arg == NULL) {
		usb_log_error("No device!\n");
		return EINVAL;
	}
	
	usbhid_kbd_t *kbd_dev = (usbhid_kbd_t *)arg;

	usbhid_kbd_poll(kbd_dev);
	
	// at the end, properly destroy the KBD structure
	usbhid_kbd_free(&kbd_dev);
	assert(kbd_dev == NULL);

	return EOK;
}

/*----------------------------------------------------------------------------*/
/* API functions                                                              */
/*----------------------------------------------------------------------------*/

int usbhid_kbd_try_add_device(ddf_dev_t *dev)
{
	/*
	 * Create default function.
	 */
	ddf_fun_t *kbd_fun = ddf_fun_create(dev, fun_exposed, "keyboard");
	if (kbd_fun == NULL) {
		usb_log_error("Could not create DDF function node.\n");
		return ENOMEM;
	}
	
	/* 
	 * Initialize device (get and process descriptors, get address, etc.)
	 */
	usb_log_info("Initializing USB/HID KBD device...\n");
	
	usbhid_kbd_t *kbd_dev = usbhid_kbd_new();
	if (kbd_dev == NULL) {
		usb_log_error("Error while creating USB/HID KBD device "
		    "structure.\n");
		ddf_fun_destroy(kbd_fun);
		return EINVAL;  // TODO: some other code??
	}
	
	int rc = usbhid_kbd_init(kbd_dev, dev);
	
	if (rc != EOK) {
		usb_log_error("Failed to initialize USB/HID KBD device.\n");
		ddf_fun_destroy(kbd_fun);
		usbhid_kbd_free(&kbd_dev);
		return rc;
	}	
	
	usb_log_info("USB/HID KBD device structure initialized.\n");
	
	/*
	 * Store the initialized keyboard device and keyboard ops
	 * to the DDF function.
	 */
	kbd_fun->driver_data = kbd_dev;
	kbd_fun->ops = &keyboard_ops;

	rc = ddf_fun_bind(kbd_fun);
	if (rc != EOK) {
		usb_log_error("Could not bind DDF function.\n");
		// TODO: Can / should I destroy the DDF function?
		ddf_fun_destroy(kbd_fun);
		usbhid_kbd_free(&kbd_dev);
		return rc;
	}
	
	rc = ddf_fun_add_to_class(kbd_fun, "keyboard");
	if (rc != EOK) {
		usb_log_error("Could not add DDF function to class 'keyboard'"
		    "\n");
		// TODO: Can / should I destroy the DDF function?
		ddf_fun_destroy(kbd_fun);
		usbhid_kbd_free(&kbd_dev);
		return rc;
	}
	
	/*
	 * Create new fibril for handling this keyboard
	 */
	fid_t fid = fibril_create(usbhid_kbd_fibril, kbd_dev);
	if (fid == 0) {
		usb_log_error("Failed to start fibril for KBD device\n");
		return ENOMEM;
	}
	fibril_add_ready(fid);

	(void)keyboard_ops;

	/*
	 * Hurrah, device is initialized.
	 */
	return EOK;
}

/**
 * @}
 */
