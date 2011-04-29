#include "private.h"
#include <usb/request.h>
#include <usb/debug.h>
#include <assert.h>
#include <errno.h>

int process_control_transfer(usbvirt_device_t *dev,
    usbvirt_control_request_handler_t *control_handlers,
    usb_device_request_setup_packet_t *setup,
    uint8_t *data, size_t *data_sent_size)
{
	assert(dev);
	assert(setup);

	if (control_handlers == NULL) {
		return EFORWARD;
	}

	usb_direction_t direction = setup->request_type & 128 ?
	    USB_DIRECTION_IN : USB_DIRECTION_OUT;
	usb_request_recipient_t req_recipient = setup->request_type & 31;
	usb_request_type_t req_type = (setup->request_type >> 5) & 3;

	usbvirt_control_request_handler_t *handler = control_handlers;
	while (handler->callback != NULL) {
		if (handler->req_direction != direction) {
			goto next;
		}
		if (handler->req_recipient != req_recipient) {
			goto next;
		}
		if (handler->req_type != req_type) {
			goto next;
		}
		if (handler->request != setup->request) {
			goto next;
		}

		usb_log_debug("Control transfer: %s(%s)\n", handler->name,
		    usb_debug_str_buffer((uint8_t*) setup, sizeof(*setup), 0));
		int rc = handler->callback(dev, setup, data, data_sent_size);
		if (rc == EFORWARD) {
			goto next;
		}

		return rc;

next:
		handler++;
	}

	return EFORWARD;
}
