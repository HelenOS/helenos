#include "hc_synchronizer.h"

void sync_wait_for(sync_value_t *value)
{
	assert( value );
	value->waiting_fibril = fibril_get_id();
	uhci_print_verbose("turning off fibril %p.\n", value->waiting_fibril);
	fibril_switch(FIBRIL_TO_MANAGER);
}
/*----------------------------------------------------------------------------*/
void sync_in_callback(
  device_t *device, usb_transaction_outcome_t result, size_t size, void *arg)
{
	sync_value_t *value = arg;
	assert(value);
	value->size = size;
	value->result = result;
	fibril_add_ready(value->waiting_fibril);
}
/*----------------------------------------------------------------------------*/
void sync_out_callback(
  device_t *device, usb_transaction_outcome_t result, void *arg)
{
	sync_value_t *value = arg;
	assert(value);
	value->result = result;
	uhci_print_verbose("resuming fibril %p.\n", value->waiting_fibril);
	fibril_add_ready(value->waiting_fibril);
}
