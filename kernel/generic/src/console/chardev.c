/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_console
 * @{
 */
/** @file
 */

#include <assert.h>
#include <adt/list.h>
#include <console/chardev.h>
#include <synch/waitq.h>
#include <synch/spinlock.h>
#include <stdio.h>
#include <halt.h>
#include <cpu.h>

/** Initialize input character device.
 *
 * @param indev Input character device.
 * @param op    Implementation of input character device operations.
 *
 */
void indev_initialize(const char *name, indev_t *indev,
    indev_operations_t *op)
{
	indev->name = name;
	waitq_initialize(&indev->wq);
	irq_spinlock_initialize(&indev->lock, "chardev.indev.lock");
	indev->counter = 0;
	indev->index = 0;
	indev->op = op;
}

/** Push character read from input character device.
 *
 * @param indev Input character device.
 * @param ch    Character being pushed.
 *
 */
void indev_push_character(indev_t *indev, char32_t ch)
{
	assert(indev);

	irq_spinlock_lock(&indev->lock, true);
	if (indev->counter == INDEV_BUFLEN - 1) {
		/* Buffer full */
		irq_spinlock_unlock(&indev->lock, true);
		return;
	}

	indev->counter++;
	indev->buffer[indev->index++] = ch;

	/* Index modulo size of buffer */
	indev->index = indev->index % INDEV_BUFLEN;
	waitq_wakeup(&indev->wq, WAKEUP_FIRST);
	irq_spinlock_unlock(&indev->lock, true);
}

/** Pop character from input character device.
 *
 * @param indev Input character device.
 *
 * @return Character read.
 *
 */
char32_t indev_pop_character(indev_t *indev)
{
	if (atomic_load(&haltstate)) {
		/*
		 * If we are here, we are hopefully on the processor that
		 * issued the 'halt' command, so proceed to read the character
		 * directly from input
		 */
		if (check_poll(indev))
			return indev->op->poll(indev);

		/* No other way of interacting with user */
		interrupts_disable();

		if (CPU)
			printf("cpu%u: ", CPU->id);
		else
			printf("cpu: ");

		printf("halted (no polling input)\n");
		cpu_halt();
	}

	waitq_sleep(&indev->wq);
	irq_spinlock_lock(&indev->lock, true);
	char32_t ch = indev->buffer[(indev->index - indev->counter) %
	    INDEV_BUFLEN];
	indev->counter--;
	irq_spinlock_unlock(&indev->lock, true);

	return ch;
}

/** Signal out-of-band condition
 *
 * @param indev  Input character device.
 * @param signal Out-of-band condition to signal.
 *
 */
void indev_signal(indev_t *indev, indev_signal_t signal)
{
	if ((indev != NULL) && (indev->op != NULL) &&
	    (indev->op->signal != NULL))
		indev->op->signal(indev, signal);
}

/** Initialize output character device.
 *
 * @param outdev Output character device.
 * @param op     Implementation of output character device operations.
 *
 */
void outdev_initialize(const char *name, outdev_t *outdev,
    outdev_operations_t *op)
{
	outdev->name = name;
	spinlock_initialize(&outdev->lock, "chardev.outdev.lock");
	link_initialize(&outdev->link);
	list_initialize(&outdev->list);
	outdev->op = op;
}

bool check_poll(indev_t *indev)
{
	if (indev == NULL)
		return false;

	if (indev->op == NULL)
		return false;

	return (indev->op->poll != NULL);
}

/** @}
 */
