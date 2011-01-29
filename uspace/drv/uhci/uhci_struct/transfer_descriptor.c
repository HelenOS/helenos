#include <stdio.h>
#include "transfer_descriptor.h"

#define BUFFER_LEN 10

static void buffer_to_str(char *str, size_t str_size,
    uint8_t *buffer, size_t buffer_size)
{
	if (buffer_size == 0) {
		*str = 0;
		return;
	}
	while (str_size >= 4) {
		snprintf(str, 4, " %02X", (int) *buffer);
		str += 3;
		str_size -= 3;
		buffer++;
		buffer_size--;
		if (buffer_size == 0) {
			break;
		}
	}
}

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

	instance->buffer_ptr = (uintptr_t)addr_to_phys(buffer);

	uhci_print_verbose("Creating buffer field: %p(%p).\n",
	  buffer, instance->buffer_ptr);

	char buffer_dump[BUFFER_LEN];
	buffer_to_str(buffer_dump, BUFFER_LEN, buffer, size);
	uhci_print_verbose("Buffer dump (%zuB): %s.\n", size, buffer_dump);

	instance->next_va = NULL;
	instance->callback = NULL;
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
		instance->status >> TD_STATUS_ACTLEN_POS & TD_STATUS_ACTLEN_MASK
	);
}
