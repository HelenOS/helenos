#include "debug.h"
#include "hc_synchronizer.h"
#include "uhci.h"

void sync_init(sync_value_t *value)
{
	assert(value);
	fibril_semaphore_initialize(&value->done, 0);
}
/*----------------------------------------------------------------------------*/
void sync_wait_for(sync_value_t *value)
{
	assert( value );
	fibril_semaphore_down(&value->done);
}
/*----------------------------------------------------------------------------*/
void sync_in_callback(
  device_t *device, usb_transaction_outcome_t result, size_t size, void *arg)
{
	sync_value_t *value = arg;
	assert(value);
	value->size = size;
	value->result = result;
	fibril_semaphore_up(&value->done);
}
/*----------------------------------------------------------------------------*/
void sync_out_callback(
  device_t *device, usb_transaction_outcome_t result, void *arg)
{
	sync_value_t *value = arg;
	assert(value);
	value->result = result;
	fibril_semaphore_up(&value->done);
}
/*----------------------------------------------------------------------------*/
int uhci_setup_sync(
  device_t *hc,
  usb_target_t target,
  usb_transfer_type_t type,
  void *buffer, size_t size,
  sync_value_t *result
  )
{
	assert(result);
	sync_init(result);

	int ret =
	  uhci_setup(hc, target, type, buffer, size,
		  sync_out_callback, (void*)result);

	if (ret) {
		uhci_print_error("sync setup transaction failed(%d).\n", ret);
		return ret;
	}

	uhci_print_verbose("setup transaction sent, waiting to complete.\n");
	sync_wait_for(result);

	return ret;
}
