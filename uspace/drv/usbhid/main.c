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

#include <driver.h>
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
#include "hid.h"
#include "descparser.h"
#include "descdump.h"
#include "conv.h"
#include "layout.h"

#define BUFFER_SIZE 8
#define NAME "usbhid"

#define GUESSED_POLL_ENDPOINT 1

/** Keyboard polling endpoint description for boot protocol class. */
static usb_endpoint_description_t poll_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_HID,
	.interface_subclass = USB_HID_SUBCLASS_BOOT,
	.interface_protocol = USB_HID_PROTOCOL_KEYBOARD,
	.flags = 0
};

static void default_connection_handler(device_t *, ipc_callid_t, ipc_call_t *);
static device_ops_t keyboard_ops = {
	.default_handler = default_connection_handler
};

static int console_callback_phone = -1;

/** Default handler for IPC methods not handled by DDF.
 *
 * @param dev Device handling the call.
 * @param icallid Call id.
 * @param icall Call data.
 */
void default_connection_handler(device_t *dev,
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
{
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

/** Currently active modifiers. 
 *
 * TODO: put to device?
 */
static unsigned mods = KM_NUM_LOCK;

/** Currently pressed lock keys. We track these to tackle autorepeat.  
 *
 * TODO: put to device? 
 */
static unsigned lock_keys;

#define NUM_LAYOUTS 3

static layout_op_t *layout[NUM_LAYOUTS] = {
	&us_qwerty_op,
	&us_dvorak_op,
	&cz_op
};

static int active_layout = 0;

static void kbd_push_ev(int type, unsigned int key)
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
			mods = mods | mod_mask;
		else
			mods = mods & ~mod_mask;
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
			mods = mods ^ (mod_mask & ~lock_keys);
			lock_keys = lock_keys | mod_mask;

			/* Update keyboard lock indicator lights. */
			// TODO
			//kbd_ctl_set_ind(mods);
		} else {
			lock_keys = lock_keys & ~mod_mask;
		}
	}
/*
	printf("type: %d\n", type);
	printf("mods: 0x%x\n", mods);
	printf("keycode: %u\n", key);
*/
	
	if (type == KEY_PRESS && (mods & KM_LCTRL) &&
		key == KC_F1) {
		active_layout = 0;
		layout[active_layout]->reset();
		return;
	}

	if (type == KEY_PRESS && (mods & KM_LCTRL) &&
		key == KC_F2) {
		active_layout = 1;
		layout[active_layout]->reset();
		return;
	}

	if (type == KEY_PRESS && (mods & KM_LCTRL) &&
		key == KC_F3) {
		active_layout = 2;
		layout[active_layout]->reset();
		return;
	}
	
	ev.type = type;
	ev.key = key;
	ev.mods = mods;

	ev.c = layout[active_layout]->parse_ev(&ev);

	printf("Sending key %d to the console\n", ev.key);
	assert(console_callback_phone != -1);
	async_msg_4(console_callback_phone, KBD_EVENT, ev.type, ev.key, ev.mods, ev.c);
}
/*
 * End of copy-paste
 */

	/*
	 * TODO:
	 * 1) key press / key release - how does the keyboard notify about release?
	 * 2) layouts (use the already defined), not important now
	 * 3) 
	 */

/*
 * Callbacks for parser
 */
static void usbkbd_process_keycodes(const uint8_t *key_codes, size_t count,
    uint8_t modifiers, void *arg)
{
	printf("Got keys: ");
	unsigned i;
	for (i = 0; i < count; ++i) {
		printf("%d ", key_codes[i]);
	}
	printf("\n");

	for (i = 0; i < count; ++i) {
		// TODO: Key press / release

		// TODO: NOT WORKING
		unsigned int key = usbkbd_parse_scancode(key_codes[i]);

		if (key == 0) {
			continue;
		}
		kbd_push_ev(KEY_PRESS, key);
	}
	printf("\n");
}

/*
 * Kbd functions
 */
static int usbkbd_get_report_descriptor(usb_hid_dev_kbd_t *kbd_dev)
{
	// iterate over all configurations and interfaces
	// TODO: more configurations!!
	unsigned i;
	for (i = 0; i < kbd_dev->conf->config_descriptor.interface_count; ++i) {
		// TODO: endianness
		uint16_t length = 
		    kbd_dev->conf->interfaces[i].hid_desc.report_desc_info.length;
		size_t actual_size = 0;

		// allocate space for the report descriptor
		kbd_dev->conf->interfaces[i].report_desc = (uint8_t *)malloc(length);
		
		// get the descriptor from the device
		int rc = usb_request_get_descriptor(&kbd_dev->ctrl_pipe,
		    USB_REQUEST_TYPE_CLASS, USB_DESCTYPE_HID_REPORT,
		    i, 0,
		    kbd_dev->conf->interfaces[i].report_desc, length,
		    &actual_size);

		if (rc != EOK) {
			return rc;
		}

		assert(actual_size == length);

		//dump_hid_class_descriptor(0, USB_DESCTYPE_HID_REPORT, 
		//    kbd_dev->conf->interfaces[i].report_desc, length);
	}

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
		return rc;
	}
	if (!endpoint_mapping[0].present) {
		usb_log_warning("Not accepting device, " \
		    "not boot-protocol keyboard.\n");
		return EREFUSED;
	}




	kbd_dev->conf = (usb_hid_configuration_t *)calloc(1, 
	    sizeof(usb_hid_configuration_t));
	if (kbd_dev->conf == NULL) {
		free(descriptors);
		return ENOMEM;
	}
	
	/*rc = usbkbd_parse_descriptors(descriptors, transferred, kbd_dev->conf);
	free(descriptors);
	if (rc != EOK) {
		printf("Problem with parsing standard descriptors.\n");
		return rc;
	}

	// get and report descriptors*/
	rc = usbkbd_get_report_descriptor(kbd_dev);
	if (rc != EOK) {
		printf("Problem with parsing HID REPORT descriptor.\n");
		return rc;
	}
	
	//usbkbd_print_config(kbd_dev->conf);

	/*
	 * TODO: 
	 * 1) select one configuration (lets say the first)
	 * 2) how many interfaces?? how to select one??
     *    ("The default setting for an interface is always alternate setting zero.")
	 * 3) find endpoint which is IN and INTERRUPT (parse), save its number
     *    as the endpoint for polling
	 */

	return EOK;
}

static usb_hid_dev_kbd_t *usbkbd_init_device(device_t *dev)
{
	int rc;

	usb_hid_dev_kbd_t *kbd_dev = (usb_hid_dev_kbd_t *)calloc(1, 
	    sizeof(usb_hid_dev_kbd_t));

	if (kbd_dev == NULL) {
		fprintf(stderr, NAME ": No memory!\n");
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
	 * will need all descriptors:
	 * 1) choose one configuration from configuration descriptors
	 *    (set it to the device)
	 * 2) set endpoints from endpoint descriptors
	 */

	// TODO: get descriptors, parse descriptors and save endpoints
	usb_endpoint_pipe_start_session(&kbd_dev->ctrl_pipe);
	//usb_request_set_configuration(&kbd_dev->ctrl_pipe, 1);
	rc = usbkbd_process_descriptors(kbd_dev);
	usb_endpoint_pipe_end_session(&kbd_dev->ctrl_pipe);
	if (rc != EOK) {
		goto error_leave;
	}

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
	printf("Calling usb_hid_boot_keyboard_input_report() with size %zu\n",
	    actual_size);
	//dump_buffer("bufffer: ", buffer, actual_size);
	int rc = usb_hid_boot_keyboard_input_report(buffer, actual_size, callbacks, 
	    NULL);
	if (rc != EOK) {
		printf("Error in usb_hid_boot_keyboard_input_report(): %d\n", rc);
	}
}

static void usbkbd_poll_keyboard(usb_hid_dev_kbd_t *kbd_dev)
{
	int rc, sess_rc;
	uint8_t buffer[BUFFER_SIZE];
	size_t actual_size;

	printf("Polling keyboard...\n");

	while (true) {
		async_usleep(1000 * 10);

		sess_rc = usb_endpoint_pipe_start_session(&kbd_dev->poll_pipe);
		if (sess_rc != EOK) {
			printf("Failed to start a session: %s.\n",
			    str_error(sess_rc));
			continue;
		}

		rc = usb_endpoint_pipe_read(&kbd_dev->poll_pipe, buffer,
		    BUFFER_SIZE, &actual_size);
		sess_rc = usb_endpoint_pipe_end_session(&kbd_dev->poll_pipe);

		if (rc != EOK) {
			printf("Error polling the keyboard: %s.\n",
			    str_error(rc));
			continue;
		}

		if (sess_rc != EOK) {
			printf("Error closing session: %s.\n",
			    str_error(sess_rc));
			continue;
		}

		/*
		 * If the keyboard answered with NAK, it returned no data.
		 * This implies that no change happened since last query.
		 */
		if (actual_size == 0) {
			printf("Keyboard returned NAK\n");
			continue;
		}

		/*
		 * TODO: Process pressed keys.
		 */
		printf("Calling usbkbd_process_interrupt_in()\n");
		usbkbd_process_interrupt_in(kbd_dev, buffer, actual_size);
	}

	// not reached
	assert(0);
}

static int usbkbd_fibril_device(void *arg)
{
	printf("!!! USB device fibril\n");

	if (arg == NULL) {
		printf("No device!\n");
		return -1;
	}

	device_t *dev = (device_t *)arg;

	// initialize device (get and process descriptors, get address, etc.)
	usb_hid_dev_kbd_t *kbd_dev = usbkbd_init_device(dev);
	if (kbd_dev == NULL) {
		printf("Error while initializing device.\n");
		return -1;
	}

	usbkbd_poll_keyboard(kbd_dev);

	return EOK;
}

static int usbkbd_add_device(device_t *dev)
{
	/* For now, fail immediately. */
	//return ENOTSUP;

	/*
	 * When everything is okay, connect to "our" HC.
	 *
	 * Not supported yet, skip..
	 */
//	int phone = usb_drv_hc_connect_auto(dev, 0);
//	if (phone < 0) {
//		/*
//		 * Connecting to HC failed, roll-back and announce
//		 * failure.
//		 */
//		return phone;
//	}

//	dev->parent_phone = phone;

	/*
	 * Create new fibril for handling this keyboard
	 */
	fid_t fid = fibril_create(usbkbd_fibril_device, dev);
	if (fid == 0) {
		printf("%s: failed to start fibril for HID device\n", NAME);
		return ENOMEM;
	}
	fibril_add_ready(fid);

	dev->ops = &keyboard_ops;

	add_device_to_class(dev, "keyboard");

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
	usb_log_enable(USB_LOG_LEVEL_INFO, "usbhid");
	return driver_main(&kbd_driver);
}

/**
 * @}
 */
