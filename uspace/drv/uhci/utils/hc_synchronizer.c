#include "hc_synchronizer.h"

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
