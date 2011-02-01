#include <errno.h>

#include <usb/debug.h>

#include "transfer_list.h"

int transfer_list_init(transfer_list_t *instance, transfer_list_t *next)
{
	assert(instance);
	instance->first = NULL;
	instance->last = NULL;
	instance->queue_head = malloc32(sizeof(queue_head_t));
	if (!instance->queue_head) {
		usb_log_error("Failed to allocate queue head.\n");
		return ENOMEM;
	}
	instance->queue_head_pa = (uintptr_t)addr_to_phys(instance->queue_head);

	uint32_t next_pa = next ? next->queue_head_pa : 0;
	queue_head_init(instance->queue_head, next_pa);
	return EOK;
}
/*----------------------------------------------------------------------------*/
int transfer_list_append(
  transfer_list_t *instance, transfer_descriptor_t *transfer)
{
	assert(instance);
	assert(transfer);

	uint32_t pa = (uintptr_t)addr_to_phys(transfer);
	assert((pa & LINK_POINTER_ADDRESS_MASK) == pa);

	/* empty list */
	if (instance->first == NULL) {
		assert(instance->last == NULL);
		instance->first = instance->last = transfer;
	} else {
		assert(instance->last);
		instance->last->next_va = transfer;

		assert(instance->last->next & LINK_POINTER_TERMINATE_FLAG);
		instance->last->next = (pa & LINK_POINTER_ADDRESS_MASK);
		instance->last = transfer;
	}

	assert(instance->queue_head);
	if (instance->queue_head->element & LINK_POINTER_TERMINATE_FLAG) {
		instance->queue_head->element = (pa & LINK_POINTER_ADDRESS_MASK);
	}
	usb_log_debug("Successfully added transfer to the hc queue %p.\n",
	  instance);
	return EOK;
}
