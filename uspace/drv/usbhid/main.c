/*
 * Copyright (c) 2010 Vojtech Horky
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
 * Main routines of USB HID driver.
 */

#include <ddf/driver.h>
#include <ipc/driver.h>
#include <ipc/kbd.h>
#include <io/keycode.h>
#include <io/console.h>
#include <errno.h>
#include <str_error.h>
#include <fibril.h>
#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/classes/hid.h>
#include <usb/classes/hidparser.h>
#include <usb/request.h>
#include <usb/descriptor.h>
#include <io/console.h>
#include <stdint.h>
#include <usb/dp.h>
#include "hid.h"
//#include "descparser.h"
//#include "descdump.h"
#include "conv.h"
#include "layout.h"

#define BUFFER_SIZE 8
#define BUFFER_OUT_SIZE 1
#define NAME "usbhid"

//#define GUESSED_POLL_ENDPOINT 1
#define BOOTP_REPORT_SIZE 6

static unsigned DEFAULT_ACTIVE_MODS = KM_NUM_LOCK;

/** Keyboard polling endpoint description for boot protocol class. */
static usb_endpoint_description_t poll_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HID,
	.interface_subclass = USB_HID_SUBCLASS_BOOT,
	.interface_protocol = USB_HID_PROTOCOL_KEYBOARD,
	.flags = 0
};

static void default_connection_handler(ddf_fun_t *, ipc_callid_t, ipc_call_t *);
static ddf_dev_ops_t keyboard_ops = {
	.default_handler = default_connection_handler
};

static int console_callback_phone = -1;

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

	if (method == IPC_M_CONNECT_TO_ME) {
		int callback = IPC_GET_ARG5(*icall);

		if (console_callback_phone != -1) {
			async_answer_0(icallid, ELIMIT);
			return;
		}

		console_callback_phone = callback;
		async_answer_0(icallid, EOK);
		return;
	}

	async_answer_0(icallid, EINVAL);
}

#if 0
static void send_key(int key, int type, wchar_t c) {
	async_msg_4(console_callback_phone, KBD_EVENT, type, key,
	    KM_NUM_LOCK, c);
}
#endif

/*
 * TODO: Move somewhere else
 */
/*
#define BYTES_PER_LINE 12

static void dump_buffer(const char *msg, const uint8_t *buffer, size_t length)
{uint8_t buffer[BUFFER_SIZE];
	printf("%s\n", msg);
	
	size_t i;
	for (i = 0; i < length; i++) {
		printf("  0x%02X", buffer[i]);
		if (((i > 0) && (((i+1) % BYTES_PER_LINE) == 0))
		    || (i + 1 == length)) {
			printf("\n");
		}
	}
}
*/
/*
 * Copy-paste from srv/hid/kbd/generic/kbd.c
 */

/** Currently active modifiers (locks is probably better word).
 *
 * TODO: put to device?
 */
//static unsigned mods = KM_NUM_LOCK;

/** Currently pressed lock keys. We track these to tackle autorepeat.  
 *
 * TODO: put to device? 
 */
//static unsigned lock_keys;

#define NUM_LAYOUTS 3

static layout_op_t *layout[NUM_LAYOUTS] = {
	&us_qwerty_op,
	&us_dvorak_op,
	&cz_op
};

static int active_layout = 0;

static void usbkbd_req_set_report(usb_hid_dev_kbd_t *kbd_dev, uint16_t iface,
    usb_hid_report_type_t type, uint8_t *buffer, size_t buf_size)
{
	int rc, sess_rc;
	
	sess_rc = usb_endpoint_pipe_start_session(&kbd_dev->ctrl_pipe);
	if (sess_rc != EOK) {
		usb_log_warning("Failed to start a session: %s.\n",
		    str_error(sess_rc));
		return;
	}

	usb_log_debug("Sending Set_Report request to the device.\n");
	
	rc = usb_control_request_set(&kbd_dev->ctrl_pipe, 
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE, 
	    USB_HIDREQ_SET_REPORT, type, iface, buffer, buf_size);

	sess_rc = usb_endpoint_pipe_end_session(&kbd_dev->ctrl_pipe);

	if (rc != EOK) {
		usb_log_warning("Error sending output report to the keyboard: "
		    "%s.\n", str_error(rc));
		return;
	}

	if (sess_rc != EOK) {
		usb_log_warning("Error closing session: %s.\n",
		    str_error(sess_rc));
		return;
	}
}

static void usbkbd_req_set_protocol(usb_hid_dev_kbd_t *kbd_dev,
    usb_hid_protocol_t protocol)
{
	int rc, sess_rc;
	
	sess_rc = usb_endpoint_pipe_start_session(&kbd_dev->ctrl_pipe);
	if (sess_rc != EOK) {
		usb_log_warning("Failed to start a session: %s.\n",
		    str_error(sess_rc));
		return;
	}

	usb_log_debug("Sending Set_Protocol request to the device ("
	    "protocol: %d, iface: %d).\n", protocol, kbd_dev->iface);
	
	rc = usb_control_request_set(&kbd_dev->ctrl_pipe, 
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE, 
	    USB_HIDREQ_SET_PROTOCOL, protocol, kbd_dev->iface, NULL, 0);

	sess_rc = usb_endpoint_pipe_end_session(&kbd_dev->ctrl_pipe);

	if (rc != EOK) {
		usb_log_warning("Error sending output report to the keyboard: "
		    "%s.\n", str_error(rc));
		return;
	}

	if (sess_rc != EOK) {
		usb_log_warning("Error closing session: %s.\n",
		    str_error(sess_rc));
		return;
	}
}

static void usbkbd_set_led(usb_hid_dev_kbd_t *kbd_dev) 
{
	uint8_t buffer[BUFFER_OUT_SIZE];
	int rc= 0, i;
	
	memset(buffer, 0, BUFFER_OUT_SIZE);
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
	    leds, buffer, BUFFER_OUT_SIZE)) != EOK) {
		usb_log_warning("Error composing output report to the keyboard:"
		    "%s.\n", str_error(rc));
		return;
	}
	
	usb_log_debug("Output report buffer: ");
	for (i = 0; i < BUFFER_OUT_SIZE; ++i) {
		usb_log_debug("0x%x ", buffer[i]);
	}
	usb_log_debug("\n");
	
	uint16_t value = 0;
	value |= (USB_HID_REPORT_TYPE_OUTPUT << 8);

	usbkbd_req_set_report(kbd_dev, kbd_dev->iface, value, buffer, 
	    BUFFER_OUT_SIZE);
}

static void kbd_push_ev(int type, unsigned int key, usb_hid_dev_kbd_t *kbd_dev)
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
	case KC_CAPS_LOCK: mod_mask = KM_CAPS_LOCK; usb_log_debug2("\n\nPushing CAPS LOCK! (mask: %u)\n\n", mod_mask); break;
	case KC_NUM_LOCK: mod_mask = KM_NUM_LOCK; usb_log_debug2("\n\nPushing NUM LOCK! (mask: %u)\n\n", mod_mask); break;
	case KC_SCROLL_LOCK: mod_mask = KM_SCROLL_LOCK; usb_log_debug2("\n\nPushing SCROLL LOCK! (mask: %u)\n\n", mod_mask); break;
	default: mod_mask = 0; break;
	}

	if (mod_mask != 0) {
		usb_log_debug2("\n\nChanging mods and lock keys\n");
		usb_log_debug2("\nmods before: 0x%x\n", kbd_dev->mods);
		usb_log_debug2("\nLock keys before:0x%x\n\n", kbd_dev->lock_keys);
		
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
 			usbkbd_set_led(kbd_dev);
		} else {
			usb_log_debug2("\nKey released.\n");
			kbd_dev->lock_keys = kbd_dev->lock_keys & ~mod_mask;
		}
	}

	usb_log_debug2("\n\nmods after: 0x%x\n", kbd_dev->mods);
	usb_log_debug2("\nLock keys after: 0x%x\n\n", kbd_dev->lock_keys);
	
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
	assert(console_callback_phone != -1);
	async_msg_4(console_callback_phone, KBD_EVENT, ev.type, ev.key, 
	    ev.mods, ev.c);
}
/*
 * End of copy-paste
 */

	/*
	 * TODO:
	 * 1) key press / key release - how does the keyboard notify about 
	 *    release?
	 * 2) layouts (use the already defined), not important now
	 * 3) 
	 */

static const keycode_t usb_hid_modifiers_keycodes[USB_HID_MOD_COUNT] = {
	KC_LCTRL,         /* USB_HID_MOD_LCTRL */
	KC_LSHIFT,        /* USB_HID_MOD_LSHIFT */
	KC_LALT,          /* USB_HID_MOD_LALT */
	0,                /* USB_HID_MOD_LGUI */
	KC_RCTRL,         /* USB_HID_MOD_RCTRL */
	KC_RSHIFT,        /* USB_HID_MOD_RSHIFT */
	KC_RALT,          /* USB_HID_MOD_RALT */
	0,                /* USB_HID_MOD_RGUI */
};

static void usbkbd_check_modifier_changes(usb_hid_dev_kbd_t *kbd_dev,
    uint8_t modifiers)
{
	/*
	 * TODO: why the USB keyboard has NUM_, SCROLL_ and CAPS_LOCK
	 *       both as modifiers and as keys with their own scancodes???
	 *
	 * modifiers should be sent as normal keys to usbkbd_parse_scancode()!!
	 * so maybe it would be better if I received it from report parser in 
	 * that way
	 */
	
	int i;
	for (i = 0; i < USB_HID_MOD_COUNT; ++i) {
		if ((modifiers & usb_hid_modifiers_consts[i]) &&
		    !(kbd_dev->modifiers & usb_hid_modifiers_consts[i])) {
			// modifier pressed
			if (usb_hid_modifiers_keycodes[i] != 0) {
				kbd_push_ev(KEY_PRESS, 
				    usb_hid_modifiers_keycodes[i], kbd_dev);
			}
		} else if (!(modifiers & usb_hid_modifiers_consts[i]) &&
		    (kbd_dev->modifiers & usb_hid_modifiers_consts[i])) {
			// modifier released
			if (usb_hid_modifiers_keycodes[i] != 0) {
				kbd_push_ev(KEY_RELEASE, 
				    usb_hid_modifiers_keycodes[i], kbd_dev);
			}
		}	// no change
	}
	
	kbd_dev->modifiers = modifiers;
}

static void usbkbd_check_key_changes(usb_hid_dev_kbd_t *kbd_dev, 
    const uint8_t *key_codes)
{
	// TODO: phantom state!!
	
	unsigned int key;
	unsigned int i, j;
	
	// TODO: quite dummy right now, think of better implementation
	
	// key releases
	for (j = 0; j < kbd_dev->keycode_count; ++j) {
		// try to find the old key in the new key list
		i = 0;
		while (i < kbd_dev->keycode_count
		    && key_codes[i] != kbd_dev->keycodes[j]) {
			++i;
		}
		
		if (i == kbd_dev->keycode_count) {
			// not found, i.e. the key was released
			key = usbkbd_parse_scancode(kbd_dev->keycodes[j]);
			kbd_push_ev(KEY_RELEASE, key, kbd_dev);
			usb_log_debug2("\nKey released: %d\n", key);
		} else {
			// found, nothing happens
		}
	}
	
	// key presses
	for (i = 0; i < kbd_dev->keycode_count; ++i) {
		// try to find the new key in the old key list
		j = 0;
		while (j < kbd_dev->keycode_count 
		    && kbd_dev->keycodes[j] != key_codes[i]) { 
			++j;
		}
		
		if (j == kbd_dev->keycode_count) {
			// not found, i.e. new key pressed
			key = usbkbd_parse_scancode(key_codes[i]);
			usb_log_debug2("\nKey pressed: %d (keycode: %d)\n", key,
			    key_codes[i]);
			kbd_push_ev(KEY_PRESS, key, kbd_dev);
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

/*
 * Callbacks for parser
 */
static void usbkbd_process_keycodes(const uint8_t *key_codes, size_t count,
    uint8_t modifiers, void *arg)
{
	if (arg == NULL) {
		usb_log_warning("Missing argument in callback "
		    "usbkbd_process_keycodes().\n");
		return;
	}

	usb_log_debug2("Got keys from parser: ");
	unsigned i;
	for (i = 0; i < count; ++i) {
		usb_log_debug2("%d ", key_codes[i]);
	}
	usb_log_debug2("\n");
	
	usb_hid_dev_kbd_t *kbd_dev = (usb_hid_dev_kbd_t *)arg;
	
	if (count != kbd_dev->keycode_count) {
		usb_log_warning("Number of received keycodes (%d) differs from"
		    " expected number (%d).\n", count, kbd_dev->keycode_count);
		return;
	}
	
	usbkbd_check_modifier_changes(kbd_dev, modifiers);
	usbkbd_check_key_changes(kbd_dev, key_codes);
}

/*
 * Kbd functions
 */
//static int usbkbd_get_report_descriptor(usb_hid_dev_kbd_t *kbd_dev)
//{
//	// iterate over all configurations and interfaces
//	// TODO: more configurations!!
//	unsigned i;
//	for (i = 0; i < kbd_dev->conf->config_descriptor.interface_count; ++i) {
//		// TODO: endianness
//		uint16_t length =  kbd_dev->conf->interfaces[i].hid_desc.
//		    report_desc_info.length;
//		size_t actual_size = 0;

//		// allocate space for the report descriptor
//		kbd_dev->conf->interfaces[i].report_desc = 
//		    (uint8_t *)malloc(length);
		
//		// get the descriptor from the device
//		int rc = usb_request_get_descriptor(&kbd_dev->ctrl_pipe,
//		    USB_REQUEST_TYPE_CLASS, USB_DESCTYPE_HID_REPORT,
//		    i, 0,
//		    kbd_dev->conf->interfaces[i].report_desc, length,
//		    &actual_size);

//		if (rc != EOK) {
//			return rc;
//		}

//		assert(actual_size == length);

//		//dump_hid_class_descriptor(0, USB_DESCTYPE_HID_REPORT, 
//		//    kbd_dev->conf->interfaces[i].report_desc, length);
//	}

//	return EOK;
//}

static int usbkbd_get_report_descriptor(usb_hid_dev_kbd_t *kbd_dev, 
    uint8_t *config_desc, size_t config_desc_size, uint8_t *iface_desc)
{
	assert(kbd_dev != NULL);
	assert(config_desc != NULL);
	assert(config_desc_size != 0);
	assert(iface_desc != NULL);
	
	usb_dp_parser_t parser =  {
		.nesting = usb_dp_standard_descriptor_nesting
	};
	
	usb_dp_parser_data_t parser_data = {
		.data = config_desc,
		.size = config_desc_size,
		.arg = NULL
	};
	
	/*
	 * First nested descriptor of interface descriptor.
	 */
	uint8_t *d = 
	    usb_dp_get_nested_descriptor(&parser, &parser_data, iface_desc);
	
	/*
	 * Search through siblings until the HID descriptor is found.
	 */
	while (d != NULL && *(d + 1) != USB_DESCTYPE_HID) {
		d = usb_dp_get_sibling_descriptor(&parser, &parser_data, 
		    iface_desc, d);
	}
	
	if (d == NULL) {
		usb_log_fatal("No HID descriptor found!\n");
		return ENOENT;
	}
	
	if (*d != sizeof(usb_standard_hid_descriptor_t)) {
		usb_log_fatal("HID descriptor hass wrong size (%u, expected %u"
		    ")\n", *d, sizeof(usb_standard_hid_descriptor_t));
		return EINVAL;
	}
	
	usb_standard_hid_descriptor_t *hid_desc = 
	    (usb_standard_hid_descriptor_t *)d;
	
	uint16_t length =  hid_desc->report_desc_info.length;
	size_t actual_size = 0;

	/*
	 * Allocate space for the report descriptor.
	 */
	kbd_dev->report_desc = (uint8_t *)malloc(length);
	if (kbd_dev->report_desc == NULL) {
		usb_log_fatal("Failed to allocate space for Report descriptor."
		    "\n");
		return ENOMEM;
	}
	
	usb_log_debug("Getting Report descriptor, expected size: %u\n", length);
	
	/*
	 * Get the descriptor from the device.
	 */
	int rc = usb_request_get_descriptor(&kbd_dev->ctrl_pipe,
	    USB_REQUEST_TYPE_STANDARD, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_DESCTYPE_HID_REPORT, 0,
	    kbd_dev->iface, kbd_dev->report_desc, length, &actual_size);

	if (rc != EOK) {
		return rc;
	}

	if (actual_size != length) {
		free(kbd_dev->report_desc);
		kbd_dev->report_desc = NULL;
		usb_log_fatal("Report descriptor has wrong size (%u, expected "
		    "%u)\n", actual_size, length);
		return EINVAL;
	}
	
	usb_log_debug("Done.\n");
	
	return EOK;
}

static int usbkbd_process_descriptors(usb_hid_dev_kbd_t *kbd_dev)
{
	// get the first configuration descriptor (TODO: parse also other!)
	usb_standard_configuration_descriptor_t config_desc;
	
	int rc;
	rc = usb_request_get_bare_configuration_descriptor(&kbd_dev->ctrl_pipe,
	    0, &config_desc);
	
	if (rc != EOK) {
		return rc;
	}
	
	// prepare space for all underlying descriptors
	uint8_t *descriptors = (uint8_t *)malloc(config_desc.total_length);
	if (descriptors == NULL) {
		return ENOMEM;
	}
	
	size_t transferred = 0;
	// get full configuration descriptor
	rc = usb_request_get_full_configuration_descriptor(&kbd_dev->ctrl_pipe,
	    0, descriptors,
	    config_desc.total_length, &transferred);
	
	if (rc != EOK) {
		return rc;
	}
	if (transferred != config_desc.total_length) {
		return ELIMIT;
	}
	
	/*
	 * Initialize the interrupt in endpoint.
	 */
	usb_endpoint_mapping_t endpoint_mapping[1] = {
		{
			.pipe = &kbd_dev->poll_pipe,
			.description = &poll_endpoint_description,
			.interface_no =
			    usb_device_get_assigned_interface(kbd_dev->device)
		}
	};
	rc = usb_endpoint_pipe_initialize_from_configuration(
	    endpoint_mapping, 1,
	    descriptors, config_desc.total_length,
	    &kbd_dev->wire);
	
	if (rc != EOK) {
		usb_log_error("Failed to initialize poll pipe: %s.\n",
		    str_error(rc));
		free(descriptors);
		return rc;
	}
	
	if (!endpoint_mapping[0].present) {
		usb_log_warning("Not accepting device, " \
		    "not boot-protocol keyboard.\n");
		free(descriptors);
		return EREFUSED;
	}
	
	usb_log_debug("Accepted device. Saving interface, and getting Report"
	    " descriptor.\n");
	
	/*
	 * Save assigned interface number.
	 */
	if (endpoint_mapping[0].interface_no < 0) {
		usb_log_error("Bad interface number.\n");
		free(descriptors);
		return EINVAL;
	}
	
	kbd_dev->iface = endpoint_mapping[0].interface_no;
	
	assert(endpoint_mapping[0].interface != NULL);
	
	rc = usbkbd_get_report_descriptor(kbd_dev, descriptors, transferred,
	    (uint8_t *)endpoint_mapping[0].interface);
	
	free(descriptors);
	
	if (rc != EOK) {
		usb_log_warning("Problem with parsing REPORT descriptor.\n");
		return rc;
	}
	
	usb_log_debug("Done parsing descriptors.\n");
	
	return EOK;
}

static usb_hid_dev_kbd_t *usbkbd_init_device(ddf_dev_t *dev)
{
	int rc;

	usb_hid_dev_kbd_t *kbd_dev = (usb_hid_dev_kbd_t *)calloc(1, 
	    sizeof(usb_hid_dev_kbd_t));

	if (kbd_dev == NULL) {
		usb_log_fatal("No memory!\n");
		return NULL;
	}

	kbd_dev->device = dev;

	/*
	 * Initialize the backing connection to the host controller.
	 */
	rc = usb_device_connection_initialize_from_device(&kbd_dev->wire, dev);
	if (rc != EOK) {
		printf("Problem initializing connection to device: %s.\n",
		    str_error(rc));
		goto error_leave;
	}

	/*
	 * Initialize device pipes.
	 */
	rc = usb_endpoint_pipe_initialize_default_control(&kbd_dev->ctrl_pipe,
	    &kbd_dev->wire);
	if (rc != EOK) {
		printf("Failed to initialize default control pipe: %s.\n",
		    str_error(rc));
		goto error_leave;
	}

	/*
	 * Get descriptors, parse descriptors and save endpoints.
	 */
	usb_endpoint_pipe_start_session(&kbd_dev->ctrl_pipe);
	
	rc = usbkbd_process_descriptors(kbd_dev);
	
	usb_endpoint_pipe_end_session(&kbd_dev->ctrl_pipe);
	if (rc != EOK) {
		goto error_leave;
	}
	
	// save the size of the report (boot protocol report by default)
	kbd_dev->keycode_count = BOOTP_REPORT_SIZE;
	kbd_dev->keycodes = (uint8_t *)calloc(
	    kbd_dev->keycode_count, sizeof(uint8_t));
	
	if (kbd_dev->keycodes == NULL) {
		usb_log_fatal("No memory!\n");
		goto error_leave;
	}
	
	kbd_dev->modifiers = 0;
	kbd_dev->mods = DEFAULT_ACTIVE_MODS;
	kbd_dev->lock_keys = 0;
	
	// set boot protocol
	usbkbd_req_set_protocol(kbd_dev, USB_HID_PROTOCOL_BOOT);
	
	// set LEDs according to internal setup (NUM LOCK enabled)
	usbkbd_set_led(kbd_dev);
	
	return kbd_dev;

error_leave:
	free(kbd_dev);
	return NULL;
}

static void usbkbd_process_interrupt_in(usb_hid_dev_kbd_t *kbd_dev,
                                        uint8_t *buffer, size_t actual_size)
{
	usb_hid_report_in_callbacks_t *callbacks =
	    (usb_hid_report_in_callbacks_t *)malloc(
	        sizeof(usb_hid_report_in_callbacks_t));
	callbacks->keyboard = usbkbd_process_keycodes;

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

static void usbkbd_poll_keyboard(usb_hid_dev_kbd_t *kbd_dev)
{
	int rc, sess_rc;
	uint8_t buffer[BUFFER_SIZE];
	size_t actual_size;

	usb_log_info("Polling keyboard...\n");

	while (true) {
		async_usleep(1000 * 10);

		sess_rc = usb_endpoint_pipe_start_session(&kbd_dev->poll_pipe);
		if (sess_rc != EOK) {
			usb_log_warning("Failed to start a session: %s.\n",
			    str_error(sess_rc));
			continue;
		}

		rc = usb_endpoint_pipe_read(&kbd_dev->poll_pipe, buffer,
		    BUFFER_SIZE, &actual_size);
		sess_rc = usb_endpoint_pipe_end_session(&kbd_dev->poll_pipe);

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
		usb_log_debug("Calling usbkbd_process_interrupt_in()\n");
		usbkbd_process_interrupt_in(kbd_dev, buffer, actual_size);
	}

	// not reached
	assert(0);
}

static int usbkbd_fibril_device(void *arg)
{
	if (arg == NULL) {
		usb_log_error("No device!\n");
		return -1;
	}
	
	usb_hid_dev_kbd_t *kbd_dev = (usb_hid_dev_kbd_t *)arg;

	usbkbd_poll_keyboard(kbd_dev);

	return EOK;
}

static int usbkbd_add_device(ddf_dev_t *dev)
{
	/*
	 * Create default function.
	 */
	// FIXME - check for errors
	ddf_fun_t *kbd_fun = ddf_fun_create(dev, fun_exposed, "keyboard");
	assert(kbd_fun != NULL);
	kbd_fun->ops = &keyboard_ops;

	int rc = ddf_fun_bind(kbd_fun);
	assert(rc == EOK);
	rc = ddf_fun_add_to_class(kbd_fun, "keyboard");
	assert(rc == EOK);
	
	/* 
	 * Initialize device (get and process descriptors, get address, etc.)
	 */
	usb_hid_dev_kbd_t *kbd_dev = usbkbd_init_device(dev);
	if (kbd_dev == NULL) {
		usb_log_error("Error while initializing device.\n");
		return -1;
	}

	/*
	 * Create new fibril for handling this keyboard
	 */
	fid_t fid = fibril_create(usbkbd_fibril_device, kbd_dev);
	if (fid == 0) {
		usb_log_error("Failed to start fibril for HID device\n");
		return ENOMEM;
	}
	fibril_add_ready(fid);

	//dev->ops = &keyboard_ops;
	(void)keyboard_ops;

	//add_device_to_class(dev, "keyboard");

	/*
	 * Hurrah, device is initialized.
	 */
	return EOK;
}

static driver_ops_t kbd_driver_ops = {
	.add_device = usbkbd_add_device,
};

static driver_t kbd_driver = {
	.name = NAME,
	.driver_ops = &kbd_driver_ops
};

int main(int argc, char *argv[])
{
	usb_log_enable(USB_LOG_LEVEL_MAX, NAME);
	return ddf_driver_main(&kbd_driver);
}

/**
 * @}
 */
