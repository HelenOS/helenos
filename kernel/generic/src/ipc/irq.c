/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2006 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup kernel_generic_ipc
 * @{
 */

/**
 * @file
 * @brief IRQ notification framework.
 *
 * This framework allows applications to subscribe to receive a notification
 * when an interrupt is detected. The application may provide a simple
 * 'top-half' handler as part of its registration, which can perform simple
 * operations (read/write port/memory, add information to notification IPC
 * message).
 *
 * The structure of a notification message is as follows:
 * - IMETHOD: interface and method as set by the SYS_IPC_IRQ_SUBSCRIBE syscall
 * - ARG1: payload modified by a 'top-half' handler (scratch[1])
 * - ARG2: payload modified by a 'top-half' handler (scratch[2])
 * - ARG3: payload modified by a 'top-half' handler (scratch[3])
 * - ARG4: payload modified by a 'top-half' handler (scratch[4])
 * - ARG5: payload modified by a 'top-half' handler (scratch[5])
 * - request_label: interrupt counter (may be needed to assure correct order
 *                  in multithreaded drivers)
 */

#include <arch.h>
#include <assert.h>
#include <mm/slab.h>
#include <mm/page.h>
#include <mm/km.h>
#include <errno.h>
#include <ddi/irq.h>
#include <ipc/ipc.h>
#include <ipc/irq.h>
#include <syscall/copy.h>
#include <console/console.h>
#include <macros.h>
#include <cap/cap.h>
#include <stdlib.h>

static void ranges_unmap(irq_pio_range_t *ranges, size_t rangecount)
{
	for (size_t i = 0; i < rangecount; i++) {
#ifdef IO_SPACE_BOUNDARY
		if ((void *) ranges[i].base >= IO_SPACE_BOUNDARY)
#endif
			km_unmap(ranges[i].base, ranges[i].size);
	}
}

static errno_t ranges_map_and_apply(irq_pio_range_t *ranges, size_t rangecount,
    irq_cmd_t *cmds, size_t cmdcount)
{
	/* Copy the physical base addresses aside. */
	uintptr_t *pbase = malloc(rangecount * sizeof(uintptr_t));
	if (!pbase)
		return ENOMEM;
	for (size_t i = 0; i < rangecount; i++)
		pbase[i] = ranges[i].base;

	/* Map the PIO ranges into the kernel virtual address space. */
	for (size_t i = 0; i < rangecount; i++) {
#ifdef IO_SPACE_BOUNDARY
		if ((void *) ranges[i].base < IO_SPACE_BOUNDARY)
			continue;
#endif
		ranges[i].base = km_map(pbase[i], ranges[i].size,
		    KM_NATURAL_ALIGNMENT,
		    PAGE_READ | PAGE_WRITE | PAGE_KERNEL | PAGE_NOT_CACHEABLE);
		if (!ranges[i].base) {
			ranges_unmap(ranges, i);
			free(pbase);
			return ENOMEM;
		}
	}

	/* Rewrite the IRQ code addresses from physical to kernel virtual. */
	for (size_t i = 0; i < cmdcount; i++) {
		uintptr_t addr;
		size_t size;

		/* Process only commands that use an address. */
		switch (cmds[i].cmd) {
		case CMD_PIO_READ_8:
		case CMD_PIO_WRITE_8:
		case CMD_PIO_WRITE_A_8:
			size = 1;
			break;
		case CMD_PIO_READ_16:
		case CMD_PIO_WRITE_16:
		case CMD_PIO_WRITE_A_16:
			size = 2;
			break;
		case CMD_PIO_READ_32:
		case CMD_PIO_WRITE_32:
		case CMD_PIO_WRITE_A_32:
			size = 4;
			break;
		default:
			/* Move onto the next command. */
			continue;
		}

		addr = (uintptr_t) cmds[i].addr;

		size_t j;
		for (j = 0; j < rangecount; j++) {
			/* Find the matching range. */
			if (!iswithin(pbase[j], ranges[j].size, addr, size))
				continue;

			/* Switch the command to a kernel virtual address. */
			addr -= pbase[j];
			addr += ranges[j].base;

			cmds[i].addr = (void *) addr;
			break;
		}

		if (j == rangecount) {
			/*
			 * The address used in this command is outside of all
			 * defined ranges.
			 */
			ranges_unmap(ranges, rangecount);
			free(pbase);
			return EINVAL;
		}
	}

	free(pbase);
	return EOK;
}

/** Statically check the top-half IRQ code
 *
 * Check the top-half IRQ code for invalid or unsafe constructs.
 *
 */
static errno_t code_check(irq_cmd_t *cmds, size_t cmdcount)
{
	for (size_t i = 0; i < cmdcount; i++) {
		/*
		 * Check for accepted ranges.
		 */
		if (cmds[i].cmd >= CMD_LAST)
			return EINVAL;

		if (cmds[i].srcarg >= IPC_CALL_LEN)
			return EINVAL;

		if (cmds[i].dstarg >= IPC_CALL_LEN)
			return EINVAL;

		switch (cmds[i].cmd) {
		case CMD_PREDICATE:
			/*
			 * Check for control flow overflow.
			 * Note that jumping just beyond the last
			 * command is a correct behaviour.
			 */
			if (i + cmds[i].value > cmdcount)
				return EINVAL;

			break;
		default:
			break;
		}
	}

	return EOK;
}

/** Free the top-half IRQ code.
 *
 * @param code Pointer to the top-half IRQ code.
 *
 */
static void code_free(irq_code_t *code)
{
	if (code) {
		ranges_unmap(code->ranges, code->rangecount);
		free(code->ranges);
		free(code->cmds);
		free(code);
	}
}

/** Copy the top-half IRQ code from userspace into the kernel.
 *
 * @param ucode Userspace address of the top-half IRQ code.
 *
 * @return Kernel address of the copied IRQ code.
 *
 */
static irq_code_t *code_from_uspace(uspace_ptr_irq_code_t ucode)
{
	irq_pio_range_t *ranges = NULL;
	irq_cmd_t *cmds = NULL;

	irq_code_t *code = malloc(sizeof(*code));
	if (!code)
		return NULL;
	errno_t rc = copy_from_uspace(code, ucode, sizeof(*code));
	if (rc != EOK)
		goto error;

	if ((code->rangecount > IRQ_MAX_RANGE_COUNT) ||
	    (code->cmdcount > IRQ_MAX_PROG_SIZE))
		goto error;

	ranges = malloc(sizeof(code->ranges[0]) * code->rangecount);
	if (!ranges)
		goto error;
	rc = copy_from_uspace(ranges, (uintptr_t) code->ranges,
	    sizeof(code->ranges[0]) * code->rangecount);
	if (rc != EOK)
		goto error;

	cmds = malloc(sizeof(code->cmds[0]) * code->cmdcount);
	if (!cmds)
		goto error;
	rc = copy_from_uspace(cmds, (uintptr_t) code->cmds,
	    sizeof(code->cmds[0]) * code->cmdcount);
	if (rc != EOK)
		goto error;

	rc = code_check(cmds, code->cmdcount);
	if (rc != EOK)
		goto error;

	rc = ranges_map_and_apply(ranges, code->rangecount, cmds,
	    code->cmdcount);
	if (rc != EOK)
		goto error;

	code->ranges = ranges;
	code->cmds = cmds;

	return code;

error:
	if (cmds)
		free(cmds);

	if (ranges)
		free(ranges);

	free(code);
	return NULL;
}

static void irq_hash_out(irq_t *irq)
{
	irq_spinlock_lock(&irq_uspace_hash_table_lock, true);
	irq_spinlock_lock(&irq->lock, false);

	if (irq->notif_cfg.hashed_in) {
		/* Remove the IRQ from the uspace IRQ hash table. */
		hash_table_remove_item(&irq_uspace_hash_table, &irq->link);
		irq->notif_cfg.hashed_in = false;
	}

	irq_spinlock_unlock(&irq->lock, false);
	irq_spinlock_unlock(&irq_uspace_hash_table_lock, true);
}

static void irq_destroy(void *arg)
{
	irq_t *irq = (irq_t *) arg;

	irq_hash_out(irq);

	/* Free up the IRQ code and associated structures. */
	code_free(irq->notif_cfg.code);
	slab_free(irq_cache, irq);
}

kobject_ops_t irq_kobject_ops = {
	.destroy = irq_destroy
};

/** Subscribe an answerbox as a receiving end for IRQ notifications.
 *
 * @param box     Receiving answerbox.
 * @param inr     IRQ number.
 * @param imethod Interface and method to be associated with the notification.
 * @param ucode   Uspace pointer to top-half IRQ code.
 *
 * @param[out] uspace_handle  Uspace pointer to IRQ capability handle
 *
 * @return  Error code.
 *
 */
errno_t ipc_irq_subscribe(answerbox_t *box, inr_t inr, sysarg_t imethod,
    uspace_ptr_irq_code_t ucode, uspace_ptr_cap_irq_handle_t uspace_handle)
{
	if ((inr < 0) || (inr > last_inr))
		return ELIMIT;

	irq_code_t *code;
	if (ucode) {
		code = code_from_uspace(ucode);
		if (!code)
			return EBADMEM;
	} else
		code = NULL;

	/*
	 * Allocate and populate the IRQ kernel object.
	 */
	cap_handle_t handle;
	errno_t rc = cap_alloc(TASK, &handle);
	if (rc != EOK)
		return rc;

	rc = copy_to_uspace(uspace_handle, &handle, sizeof(cap_handle_t));
	if (rc != EOK) {
		cap_free(TASK, handle);
		return rc;
	}

	irq_t *irq = (irq_t *) slab_alloc(irq_cache, FRAME_ATOMIC);
	if (!irq) {
		cap_free(TASK, handle);
		return ENOMEM;
	}

	kobject_t *kobject = kobject_alloc(FRAME_ATOMIC);
	if (!kobject) {
		cap_free(TASK, handle);
		slab_free(irq_cache, irq);
		return ENOMEM;
	}

	irq_initialize(irq);
	irq->inr = inr;
	irq->claim = ipc_irq_top_half_claim;
	irq->handler = ipc_irq_top_half_handler;
	irq->notif_cfg.notify = true;
	irq->notif_cfg.answerbox = box;
	irq->notif_cfg.imethod = imethod;
	irq->notif_cfg.code = code;
	irq->notif_cfg.counter = 0;

	/*
	 * Insert the IRQ structure into the uspace IRQ hash table.
	 */
	irq_spinlock_lock(&irq_uspace_hash_table_lock, true);
	irq_spinlock_lock(&irq->lock, false);

	irq->notif_cfg.hashed_in = true;
	hash_table_insert(&irq_uspace_hash_table, &irq->link);

	irq_spinlock_unlock(&irq->lock, false);
	irq_spinlock_unlock(&irq_uspace_hash_table_lock, true);

	kobject_initialize(kobject, KOBJECT_TYPE_IRQ, irq);
	cap_publish(TASK, handle, kobject);

	return EOK;
}

/** Unsubscribe task from IRQ notification.
 *
 * @param box     Answerbox associated with the notification.
 * @param handle  IRQ capability handle.
 *
 * @return EOK on success or an error code.
 *
 */
errno_t ipc_irq_unsubscribe(answerbox_t *box, cap_irq_handle_t handle)
{
	kobject_t *kobj = cap_unpublish(TASK, handle, KOBJECT_TYPE_IRQ);
	if (!kobj)
		return ENOENT;

	assert(kobj->irq->notif_cfg.answerbox == box);

	irq_hash_out(kobj->irq);

	kobject_put(kobj);
	cap_free(TASK, handle);

	return EOK;
}

/** Add a call to the proper answerbox queue.
 *
 * Assume irq->lock is locked and interrupts disabled.
 *
 * @param irq  IRQ structure referencing the target answerbox.
 * @param call IRQ notification call.
 *
 */
static void send_call(irq_t *irq, call_t *call)
{
	irq_spinlock_lock(&irq->notif_cfg.answerbox->irq_lock, false);
	list_append(&call->ab_link, &irq->notif_cfg.answerbox->irq_notifs);
	irq_spinlock_unlock(&irq->notif_cfg.answerbox->irq_lock, false);

	waitq_wake_one(&irq->notif_cfg.answerbox->wq);
}

/** Apply the top-half IRQ code to find out whether to accept the IRQ or not.
 *
 * @param irq IRQ structure.
 *
 * @return IRQ_ACCEPT if the interrupt is accepted by the IRQ code.
 * @return IRQ_DECLINE if the interrupt is not accepted byt the IRQ code.
 *
 */
irq_ownership_t ipc_irq_top_half_claim(irq_t *irq)
{
	irq_code_t *code = irq->notif_cfg.code;
	uint32_t *scratch = irq->notif_cfg.scratch;

	if (!irq->notif_cfg.notify)
		return IRQ_DECLINE;

	if (!code)
		return IRQ_DECLINE;

	for (size_t i = 0; i < code->cmdcount; i++) {
		uintptr_t srcarg = code->cmds[i].srcarg;
		uintptr_t dstarg = code->cmds[i].dstarg;

		switch (code->cmds[i].cmd) {
		case CMD_PIO_READ_8:
			scratch[dstarg] =
			    pio_read_8((ioport8_t *) code->cmds[i].addr);
			break;
		case CMD_PIO_READ_16:
			scratch[dstarg] =
			    pio_read_16((ioport16_t *) code->cmds[i].addr);
			break;
		case CMD_PIO_READ_32:
			scratch[dstarg] =
			    pio_read_32((ioport32_t *) code->cmds[i].addr);
			break;
		case CMD_PIO_WRITE_8:
			pio_write_8((ioport8_t *) code->cmds[i].addr,
			    (uint8_t) code->cmds[i].value);
			break;
		case CMD_PIO_WRITE_16:
			pio_write_16((ioport16_t *) code->cmds[i].addr,
			    (uint16_t) code->cmds[i].value);
			break;
		case CMD_PIO_WRITE_32:
			pio_write_32((ioport32_t *) code->cmds[i].addr,
			    (uint32_t) code->cmds[i].value);
			break;
		case CMD_PIO_WRITE_A_8:
			pio_write_8((ioport8_t *) code->cmds[i].addr,
			    (uint8_t) scratch[srcarg]);
			break;
		case CMD_PIO_WRITE_A_16:
			pio_write_16((ioport16_t *) code->cmds[i].addr,
			    (uint16_t) scratch[srcarg]);
			break;
		case CMD_PIO_WRITE_A_32:
			pio_write_32((ioport32_t *) code->cmds[i].addr,
			    (uint32_t) scratch[srcarg]);
			break;
		case CMD_LOAD:
			scratch[dstarg] = code->cmds[i].value;
			break;
		case CMD_AND:
			scratch[dstarg] = scratch[srcarg] &
			    code->cmds[i].value;
			break;
		case CMD_PREDICATE:
			if (scratch[srcarg] == 0)
				i += code->cmds[i].value;

			break;
		case CMD_ACCEPT:
			return IRQ_ACCEPT;
		case CMD_DECLINE:
		default:
			return IRQ_DECLINE;
		}
	}

	return IRQ_DECLINE;
}

/** IRQ top-half handler.
 *
 * We expect interrupts to be disabled and the irq->lock already held.
 *
 * @param irq IRQ structure.
 *
 */
void ipc_irq_top_half_handler(irq_t *irq)
{
	assert(irq);

	assert(interrupts_disabled());
	assert(irq_spinlock_locked(&irq->lock));

	if (irq->notif_cfg.answerbox) {
		call_t *call = ipc_call_alloc();
		if (!call)
			return;

		call->flags |= IPC_CALL_NOTIF;
		/* Put a counter to the message */
		call->priv = ++irq->notif_cfg.counter;

		/* Set up args */
		ipc_set_imethod(&call->data, irq->notif_cfg.imethod);
		ipc_set_arg1(&call->data, irq->notif_cfg.scratch[1]);
		ipc_set_arg2(&call->data, irq->notif_cfg.scratch[2]);
		ipc_set_arg3(&call->data, irq->notif_cfg.scratch[3]);
		ipc_set_arg4(&call->data, irq->notif_cfg.scratch[4]);
		ipc_set_arg5(&call->data, irq->notif_cfg.scratch[5]);

		send_call(irq, call);
	}
}

/** Send notification message.
 *
 * @param irq IRQ structure.
 * @param a1  Driver-specific payload argument.
 * @param a2  Driver-specific payload argument.
 * @param a3  Driver-specific payload argument.
 * @param a4  Driver-specific payload argument.
 * @param a5  Driver-specific payload argument.
 *
 */
void ipc_irq_send_msg(irq_t *irq, sysarg_t a1, sysarg_t a2, sysarg_t a3,
    sysarg_t a4, sysarg_t a5)
{
	irq_spinlock_lock(&irq->lock, true);

	if (irq->notif_cfg.answerbox) {
		call_t *call = ipc_call_alloc();
		if (!call) {
			irq_spinlock_unlock(&irq->lock, true);
			return;
		}

		call->flags |= IPC_CALL_NOTIF;
		/* Put a counter to the message */
		call->priv = ++irq->notif_cfg.counter;

		ipc_set_imethod(&call->data, irq->notif_cfg.imethod);
		ipc_set_arg1(&call->data, a1);
		ipc_set_arg2(&call->data, a2);
		ipc_set_arg3(&call->data, a3);
		ipc_set_arg4(&call->data, a4);
		ipc_set_arg5(&call->data, a5);

		send_call(irq, call);
	}

	irq_spinlock_unlock(&irq->lock, true);
}

/** @}
 */
