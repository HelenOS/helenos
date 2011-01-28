#include "transfer_descriptor.h"

void transfer_descriptor_init(transfer_descriptor_t *instance,
  int error_count, size_t size, bool isochronous, usb_target_t target,
	int pid, void *buffer)
{
	assert(instance);

	instance->next =
	  0 | LINK_POINTER_TERMINATE_FLAG;

	uhci_print_verbose("Creating link field: %x.\n", instance->next);

	assert(size < 1024);
	instance->status = 0
	  | ((error_count & TD_STATUS_ERROR_COUNT_MASK) << TD_STATUS_ERROR_COUNT_POS)
	  | TD_STATUS_ERROR_ACTIVE;

	uhci_print_verbose("Creating status field: %x.\n", instance->status);

	instance->device = 0
		| (((size - 1) & TD_DEVICE_MAXLEN_MASK) << TD_DEVICE_MAXLEN_POS)
		| ((target.address & TD_DEVICE_ADDRESS_MASK) << TD_DEVICE_ADDRESS_POS)
		| ((target.endpoint & TD_DEVICE_ENDPOINT_MASK) << TD_DEVICE_ENDPOINT_POS)
		| ((pid & TD_DEVICE_PID_MASK) << TD_DEVICE_PID_POS);

	uhci_print_verbose("Creating device field: %x.\n", instance->device);

	if (size) {
		instance->buffer_ptr = (uintptr_t)addr_to_phys(buffer);

		uhci_print_verbose("Creating buffer field: %p(%p).\n",
			buffer, instance->buffer_ptr);

		if (size >= 8) {
			char * buff = buffer;

			uhci_print_verbose("Buffer dump(8B): %x %x %x %x %x %x %x %x.\n",
				buff[0], buff[1], buff[2], buff[3], buff[4], buff[5], buff[6], buff[7]);
		}
	} else {
		instance->buffer_ptr = 0;
	}

	instance->next_va = NULL;
	instance->callback = NULL;
	uhci_print_info("Created a new TD.\n");
}

static inline usb_transaction_outcome_t convert_outcome(uint32_t status)
{
	/*TODO: refactor into something sane */
	/*TODO: add additional usb_errors to usb_outcome_t */

	if (status & TD_STATUS_ERROR_STALLED)
		return USB_OUTCOME_CRCERROR;

	if (status & TD_STATUS_ERROR_BUFFER)
		return USB_OUTCOME_CRCERROR;

	if (status & TD_STATUS_ERROR_BABBLE)
		return USB_OUTCOME_BABBLE;

	if (status & TD_STATUS_ERROR_NAK)
		return USB_OUTCOME_CRCERROR;

  if (status & TD_STATUS_ERROR_CRC)
		return USB_OUTCOME_CRCERROR;

	if (status & TD_STATUS_ERROR_BIT_STUFF)
		return USB_OUTCOME_CRCERROR;

	assert((((status >> TD_STATUS_ERROR_POS) & TD_STATUS_ERROR_MASK)
	| TD_STATUS_ERROR_RESERVED) == TD_STATUS_ERROR_RESERVED);
	return USB_OUTCOME_OK;
}

void transfer_descriptor_fini(transfer_descriptor_t *instance)
{
	assert(instance);
	callback_run(instance->callback,
		convert_outcome(instance->status),
		((instance->status >> TD_STATUS_ACTLEN_POS) + 1) & TD_STATUS_ACTLEN_MASK
	);
}
