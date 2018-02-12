/*
 * Copyright (c) 2009 Jakub Jermar
 * Copyright (c) 2018 CZ.NIC, z.s.p.o.
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

/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief NS 16550 serial controller driver.
 */

#include <assert.h>
#include <genarch/drivers/ns16550/ns16550.h>
#include <ddi/irq.h>
#include <arch/asm.h>
#include <console/chardev.h>
#include <mm/slab.h>
#include <str.h>

#define LSR_DATA_READY  0x01
#define LSR_TH_READY    0x20

static inline uint8_t _read(ns16550_instance_t *inst, int reg) {
	return pio_read_8(&inst->ns16550[reg << inst->reg_shift]);
}

static inline void _write(ns16550_instance_t *inst, int reg, uint8_t val) {
	pio_write_8(&inst->ns16550[reg << inst->reg_shift], val);
}

static irq_ownership_t ns16550_claim(irq_t *irq)
{
	ns16550_instance_t *instance = irq->instance;

	if (_read(instance, NS16550_REG_LSR) & LSR_DATA_READY)
		return IRQ_ACCEPT;
	else
		return IRQ_DECLINE;
}

static void ns16550_irq_handler(irq_t *irq)
{
	ns16550_instance_t *instance = irq->instance;
	
	while (_read(instance, NS16550_REG_LSR) & LSR_DATA_READY) {
		uint8_t data = _read(instance, NS16550_REG_RBR);
		indev_push_character(instance->input, data);
	}
}

/**< Clear input buffer. */
static void ns16550_clear_buffer(ns16550_instance_t *instance)
{
	while (_read(instance, NS16550_REG_LSR) & LSR_DATA_READY)
		(void) _read(instance, NS16550_REG_RBR);
}

static void ns16550_sendb(ns16550_instance_t *instance, uint8_t byte)
{
	while (!(_read(instance, NS16550_REG_LSR) & LSR_TH_READY))
		;
	_write(instance, NS16550_REG_THR, byte);
}

static void ns16550_putchar(outdev_t *dev, wchar_t ch)
{
	ns16550_instance_t *instance = (ns16550_instance_t *) dev->data;
	
	if ((!instance->parea.mapped) || (console_override)) {
		if (ascii_check(ch))
			ns16550_sendb(instance, (uint8_t) ch);
		else
			ns16550_sendb(instance, U_SPECIAL);
	}
}

static outdev_operations_t ns16550_ops = {
	.write = ns16550_putchar,
	.redraw = NULL
};

/** Initialize ns16550.
 *
 * @param dev        Address of the beginning of the device in I/O space.
 * @param reg_shift  Spacing between individual register addresses, in log2.
 *                   The individual register location is calculated as
 *                   `base + (register offset << reg_shift)`.
 * @param inr        Interrupt number.
 * @param cir        Clear interrupt function.
 * @param cir_arg    First argument to cir.
 * @param output     Where to store pointer to the output device
 *                   or NULL if the caller is not interested in
 *                   writing to the serial port.
 *
 * @return Keyboard instance or NULL on failure.
 *
 */
ns16550_instance_t *ns16550_init(ioport8_t *dev, int reg_shift, inr_t inr,
    cir_t cir, void *cir_arg, outdev_t **output)
{
	ns16550_instance_t *instance
	    = malloc(sizeof(ns16550_instance_t), FRAME_ATOMIC);
	if (instance) {
		instance->ns16550 = dev;
		instance->reg_shift = reg_shift;
		instance->input = NULL;
		instance->output = NULL;
		
		if (output) {
			instance->output = malloc(sizeof(outdev_t),
			    FRAME_ATOMIC);
			if (!instance->output) {
				free(instance);
				return NULL;
			}
			
			outdev_initialize("ns16550", instance->output,
			    &ns16550_ops);
			instance->output->data = instance;
			*output = instance->output;
		}
		
		irq_initialize(&instance->irq);
		instance->irq.inr = inr;
		instance->irq.claim = ns16550_claim;
		instance->irq.handler = ns16550_irq_handler;
		instance->irq.instance = instance;
		instance->irq.cir = cir;
		instance->irq.cir_arg = cir_arg;
		
		instance->parea.pbase = (uintptr_t) dev;
		instance->parea.frames = 1;
		instance->parea.unpriv = false;
		instance->parea.mapped = false;
		ddi_parea_register(&instance->parea);
	}
	
	return instance;
}

void ns16550_wire(ns16550_instance_t *instance, indev_t *input)
{
	assert(instance);
	assert(input);
	
	instance->input = input;
	irq_register(&instance->irq);
	
	ns16550_clear_buffer(instance);
	
	/* Enable interrupts */
	_write(instance, NS16550_REG_IER, IER_ERBFI);
	_write(instance, NS16550_REG_MCR, MCR_OUT2);
}

/** @}
 */
