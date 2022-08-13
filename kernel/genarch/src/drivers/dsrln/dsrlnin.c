/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Dummy serial line input.
 */

#include <assert.h>
#include <genarch/drivers/dsrln/dsrlnin.h>
#include <console/chardev.h>
#include <stdlib.h>
#include <arch/asm.h>

static irq_ownership_t dsrlnin_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void dsrlnin_irq_handler(irq_t *irq)
{
	dsrlnin_instance_t *instance = irq->instance;
	dsrlnin_t *dev = instance->dsrlnin;

	indev_push_character(instance->srlnin, pio_read_8(&dev->data));
}

dsrlnin_instance_t *dsrlnin_init(dsrlnin_t *dev, inr_t inr)
{
	dsrlnin_instance_t *instance =
	    malloc(sizeof(dsrlnin_instance_t));
	if (instance) {
		instance->dsrlnin = dev;
		instance->srlnin = NULL;

		irq_initialize(&instance->irq);
		instance->irq.inr = inr;
		instance->irq.claim = dsrlnin_claim;
		instance->irq.handler = dsrlnin_irq_handler;
		instance->irq.instance = instance;
	}

	return instance;
}

void dsrlnin_wire(dsrlnin_instance_t *instance, indev_t *srlnin)
{
	assert(instance);
	assert(srlnin);

	instance->srlnin = srlnin;
	irq_register(&instance->irq);
}

/** @}
 */
