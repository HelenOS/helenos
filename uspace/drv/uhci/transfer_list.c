#include <errno.h>

#include "transfer_list.h"

int transfer_list_init(transfer_list_t *instance, transfer_list_t *next)
{
	assert(instance);
	instance->first = NULL;
	instance->last = NULL;
	instance->queue_head = trans_malloc(sizeof(queue_head_t));
	if (!instance->queue_head) {
		uhci_print_error("Failed to allocate queue head.\n");
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
	return EOK;
}
