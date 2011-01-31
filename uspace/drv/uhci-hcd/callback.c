#include <errno.h>
#include <mem.h>

#include "callback.h"
int callback_init(callback_t *instance, device_t *dev,
  void *buffer, size_t size, usbhc_iface_transfer_in_callback_t func_in,
  usbhc_iface_transfer_out_callback_t func_out, void *arg)
{
	assert(instance);
	assert(func_in == NULL || func_out == NULL);
	if (size > 0) {
		instance->new_buffer = malloc32(size);
		if (!instance->new_buffer) {
			uhci_print_error("Failed to allocate device acessible buffer.\n");
			return ENOMEM;
		}
		if (func_out)
			memcpy(instance->new_buffer, buffer, size);
	} else {
		instance->new_buffer = NULL;
	}


	instance->callback_out = func_out;
	instance->callback_in = func_in;
	instance->old_buffer = buffer;
	instance->buffer_size = size;
	instance->dev = dev;
	instance->arg = arg;
	return EOK;
}
/*----------------------------------------------------------------------------*/
void callback_run(
callback_t *instance, usb_transaction_outcome_t outcome, size_t act_size)
{
	assert(instance);

	/* update the old buffer */
	if (instance->new_buffer &&
	  (instance->new_buffer != instance->old_buffer)) {
		memcpy(instance->old_buffer, instance->new_buffer, instance->buffer_size);
		free32(instance->new_buffer);
		instance->new_buffer = NULL;
	}

	if (instance->callback_in) {
		assert(instance->callback_out == NULL);
		uhci_print_verbose("Callback in: %p %x %d.\n",
		  instance->callback_in, outcome, act_size);
		instance->callback_in(
		  instance->dev, outcome, act_size, instance->arg);
	} else {
		assert(instance->callback_out);
		assert(instance->callback_in == NULL);
		uhci_print_verbose("Callback out: %p %p %x %p .\n",
		 instance->callback_out, instance->dev, outcome, instance->arg);
		instance->callback_out(
		  instance->dev, outcome, instance->arg);
	}
}
