/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief i8042 processor driver
 *
 * It takes care of the i8042 serial communication.
 *
 */

#include <assert.h>
#include <genarch/drivers/i8042/i8042.h>
#include <genarch/drivers/legacy/ia32/io.h>
#include <arch/asm.h>
#include <console/chardev.h>
#include <stdlib.h>
#include <time/delay.h>

#define i8042_SET_COMMAND  0x60
#define i8042_COMMAND      0x69
#define i8042_CPU_RESET    0xfe

#define i8042_BUFFER_FULL_MASK  0x01
#define i8042_WAIT_MASK         0x02

#define i8042_TIMEOUT  65536

static irq_ownership_t i8042_claim(irq_t *irq)
{
	i8042_instance_t *i8042_instance = irq->instance;
	i8042_t *dev = i8042_instance->i8042;

	if (pio_read_8(&dev->status) & i8042_BUFFER_FULL_MASK)
		return IRQ_ACCEPT;
	else
		return IRQ_DECLINE;
}

static void i8042_irq_handler(irq_t *irq)
{
	i8042_instance_t *instance = irq->instance;
	i8042_t *dev = instance->i8042;

	if (pio_read_8(&dev->status) & i8042_BUFFER_FULL_MASK) {
		uint8_t data = pio_read_8(&dev->data);
		indev_push_character(instance->kbrdin, data);
	}
}

/** Clear input buffer. */
static void i8042_clear_buffer(i8042_t *dev)
{
	for (uint32_t i = 0; i < i8042_TIMEOUT; i++) {
		if ((pio_read_8(&dev->status) & i8042_BUFFER_FULL_MASK) == 0)
			break;

		(void) pio_read_8(&dev->data);
		delay(50);  /* 50 us think time */
	}
}

static void i8042_send_command(i8042_t *dev, uint8_t cmd)
{
	for (uint32_t i = 0; i < i8042_TIMEOUT; i++) {
		if ((pio_read_8(&dev->status) & i8042_WAIT_MASK) == 0)
			break;

		delay(50);  /* 50 us think time */
	}

	pio_write_8(&dev->status, cmd);
	delay(10000);  /* 10 ms think time */
}

/** Initialize i8042. */
i8042_instance_t *i8042_init(i8042_t *dev, inr_t inr)
{
	i8042_instance_t *instance =
	    malloc(sizeof(i8042_instance_t));
	if (instance) {
		instance->i8042 = dev;
		instance->kbrdin = NULL;

		irq_initialize(&instance->irq);
		instance->irq.inr = inr;
		instance->irq.claim = i8042_claim;
		instance->irq.handler = i8042_irq_handler;
		instance->irq.instance = instance;
	}

	return instance;
}

void i8042_wire(i8042_instance_t *instance, indev_t *kbrdin)
{
	assert(instance);
	assert(kbrdin);

	i8042_clear_buffer(instance->i8042);

	instance->kbrdin = kbrdin;
	irq_register(&instance->irq);
}

/* Reset CPU by pulsing pin 0 */
void i8042_cpu_reset(i8042_t *dev)
{
	interrupts_disable();
	i8042_clear_buffer(dev);
	i8042_send_command(dev, i8042_CPU_RESET);
}

/** @}
 */
