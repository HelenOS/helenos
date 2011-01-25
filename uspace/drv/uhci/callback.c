#include <errno.h>

#include "callback.h"


int callback_init(callback_t *instance, device_t *dev,
  void *buffer, size_t size, usbhc_iface_transfer_in_callback_t func_in,
  usbhc_iface_transfer_out_callback_t func_out, void *arg)
{
	assert(instance);
	assert(func_in == NULL || func_out == NULL);
	instance->new_buffer = trans_malloc(size);
	if (!instance->new_buffer) {
		uhci_print_error("Failed to allocate device acessible buffer.\n");
		return ENOMEM;
	}

	if (func_out)
		memcpy(instance->new_buffer, buffer, size);

	instance->callback_out = func_out;
	instance->callback_in = func_in;
	instance->old_buffer = buffer;
	instance->buffer_size = size;
	instance->dev = dev;
	return EOK;
}
