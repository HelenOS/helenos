#include "private.h"
#include <usb/request.h>
#include <assert.h>
#include <errno.h>

void usbvirt_control_reply_helper(const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size,
    void *actual_data, size_t actual_data_size)
{
	size_t expected_size = setup_packet->length;
	if (expected_size < actual_data_size) {
		actual_data_size = expected_size;
	}

	memcpy(data, actual_data, actual_data_size);

	if (act_size != NULL) {
		*act_size = actual_data_size;
	}
}

/** GET_DESCRIPTOR handler. */
static int req_get_descriptor(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet, uint8_t *data, size_t *act_size)
{
	uint8_t type = setup_packet->value_high;
	uint8_t index = setup_packet->value_low;

	/*
	 * Standard device descriptor.
	 */
	if ((type == USB_DESCTYPE_DEVICE) && (index == 0)) {
		if (device->descriptors && device->descriptors->device) {
			usbvirt_control_reply_helper(setup_packet, data, act_size,
			    device->descriptors->device,
			    device->descriptors->device->length);
			return EOK;
		} else {
			return EFORWARD;
		}
	}

	/*
	 * Configuration descriptor together with interface, endpoint and
	 * class-specific descriptors.
	 */
	if (type == USB_DESCTYPE_CONFIGURATION) {
		if (!device->descriptors) {
			return EFORWARD;
		}
		if (index >= device->descriptors->configuration_count) {
			return EFORWARD;
		}
		/* Copy the data. */
		usbvirt_device_configuration_t *config = &device->descriptors
		    ->configuration[index];
		uint8_t *all_data = malloc(config->descriptor->total_length);
		if (all_data == NULL) {
			return ENOMEM;
		}

		uint8_t *ptr = all_data;
		memcpy(ptr, config->descriptor, config->descriptor->length);
		ptr += config->descriptor->length;
		size_t i;
		for (i = 0; i < config->extra_count; i++) {
			usbvirt_device_configuration_extras_t *extra
			    = &config->extra[i];
			memcpy(ptr, extra->data, extra->length);
			ptr += extra->length;
		}

		usbvirt_control_reply_helper(setup_packet, data, act_size,
		    all_data, config->descriptor->total_length);

		free(all_data);

		return EOK;
	}

	return EFORWARD;
}

static int req_set_address(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet, uint8_t *data, size_t *act_size)
{
	uint16_t new_address = setup_packet->value;
	uint16_t zero1 = setup_packet->index;
	uint16_t zero2 = setup_packet->length;

	if ((zero1 != 0) || (zero2 != 0)) {
		return EINVAL;
	}

	if (new_address > 127) {
		return EINVAL;
	}

	device->address = new_address;

	return EOK;
}

static int req_set_configuration(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet, uint8_t *data, size_t *act_size)
{
	uint16_t configuration_value = setup_packet->value;
	uint16_t zero1 = setup_packet->index;
	uint16_t zero2 = setup_packet->length;

	if ((zero1 != 0) || (zero2 != 0)) {
		return EINVAL;
	}

	/*
	 * Configuration value is 1 byte information.
	 */
	if (configuration_value > 255) {
		return EINVAL;
	}

	/*
	 * Do nothing when in default state. According to specification,
	 * this is not specified.
	 */
	if (device->state == USBVIRT_STATE_DEFAULT) {
		return EOK;
	}

	usbvirt_device_state_t new_state;
	if (configuration_value == 0) {
		new_state = USBVIRT_STATE_ADDRESS;
	} else {
		// FIXME: check that this configuration exists
		new_state = USBVIRT_STATE_CONFIGURED;
	}

	if (device->ops && device->ops->state_changed) {
		device->ops->state_changed(device, device->state, new_state);
	}
	device->state = new_state;

	return EOK;
}

usbvirt_control_request_handler_t library_handlers[] = {
	{
		.req_direction = USB_DIRECTION_OUT,
		.req_recipient = USB_REQUEST_RECIPIENT_DEVICE,
		.req_type = USB_REQUEST_TYPE_STANDARD,
		.request = USB_DEVREQ_SET_ADDRESS,
		.name = "SetAddress",
		.callback = req_set_address
	},
	{
		.req_direction = USB_DIRECTION_IN,
		.req_recipient = USB_REQUEST_RECIPIENT_DEVICE,
		.req_type = USB_REQUEST_TYPE_STANDARD,
		.request = USB_DEVREQ_GET_DESCRIPTOR,
		.name = "GetDescriptor",
		.callback = req_get_descriptor
	},
	{
		.req_direction = USB_DIRECTION_OUT,
		.req_recipient = USB_REQUEST_RECIPIENT_DEVICE,
		.req_type = USB_REQUEST_TYPE_STANDARD,
		.request = USB_DEVREQ_SET_CONFIGURATION,
		.name = "SetConfiguration",
		.callback = req_set_configuration
	},

	{ .callback = NULL }
};

