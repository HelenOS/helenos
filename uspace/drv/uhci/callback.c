#include <errno.h>
#include <mem.h>

#include "callback.h"
int callback_init(callback_t *instance, device_t *dev,
  void *buffer, size_t size, usbhc_iface_transfer_in_callback_t func_in,
  usbhc_iface_transfer_out_callback_t func_out, void *arg)
{
	assert(instance);
	assert(func_in == NULL || func_out == NULL);
	instance->new_buffer = malloc32(size);
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
/*----------------------------------------------------------------------------*/
void callback_run(
callback_t *instance, usb_transaction_outcome_t outcome, size_t act_size)
{
	assert(instance);

	/* update the old buffer */
	if (instance->new_buffer) {
		memcpy(instance->new_buffer, instance->old_buffer, instance->buffer_size);
		free32(instance->new_buffer);
		instance->new_buffer = NULL;
	}

	if (instance->callback_in) {
		assert(instance->callback_out == NULL);
		instance->callback_in(
		  instance->dev, act_size, outcome, instance->arg);
	}
}
