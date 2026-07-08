/*
 * Copyright (c) 2026 Jakub Kodym
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

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 * @brief Synopsys DesignWare DW_apb_uart driver.
 */

#include <assert.h>
#include <genarch/drivers/dwapb/dwapb.h>
#include <ddi/irq.h>
#include <ddi/ddi.h>
#include <arch/asm.h>
#include <console/chardev.h>
#include <stdlib.h>
#include <align.h>
#include <str.h>
#include <macros.h>

#define RETRY_CNT	100000

#define NOTHING \
	do { \
	} while(0)

#define WHILE_CNT_AND_COND_DO(cnt, expr, statement) \
	do { \
		for (volatile unsigned i = 0; i < (cnt); i++) \
			if ((expr)) \
				statement; \
			else \
				break; \
	} while (0)

/** Claim the IRQ. */
static irq_ownership_t dwapb_claim(irq_t *irq)
{
	dwapb_instance_t *instance = irq->instance;
	dwapb_regs_t *regs = instance->regs;

	uint32_t iir_iid = pio_read_32(&regs->iir) & IIR_IID_MASK;

	if (iir_iid == IID_NO_INTERRUPT)
		return IRQ_DECLINE;
	else
		return IRQ_ACCEPT;
}

/** Clear input buffer. */
static void dwapb_clear_buffer(dwapb_instance_t *instance)
{
	dwapb_regs_t *regs = instance->regs;

	WHILE_CNT_AND_COND_DO(RETRY_CNT,
	    pio_read_32(&regs->lsr) & LSR_DR,
	    (void) pio_read_32(&regs->rbr));
}

/** Send a byte through the UART. */
static void dwapb_sendb(dwapb_instance_t *instance, uint8_t byte)
{
	dwapb_regs_t *regs = instance->regs;

	WHILE_CNT_AND_COND_DO(RETRY_CNT,
	    !(pio_read_32(&regs->lsr) & LSR_THRE),
	    NOTHING);

	pio_write_32(&regs->thr, (uint32_t)byte);
}

/** Wait until the UART is not busy. */
static void dwapb_wait_busy(dwapb_regs_t *regs)
{
	/* If not implemented, the register reads as zero. */
	WHILE_CNT_AND_COND_DO(RETRY_CNT,
	    pio_read_32(&regs->usr) & USR_BUSY,
	    NOTHING);
}

/** IRQ handler for Busy detect interrupt. */
static void dwapb_irq_busy_detect(dwapb_instance_t *instance)
{
	dwapb_regs_t *regs = instance->regs;

	dwapb_wait_busy(regs);

	/* Clear the interrupt. */
	pio_read_32(&regs->usr);
}

/** IRQ handler for Received data or Character timeout interrupts. */
static void dwapb_irq_received_data(dwapb_instance_t *instance)
{
	dwapb_regs_t *regs = instance->regs;

	while (pio_read_32(&regs->lsr) & LSR_DR) {
		uint32_t data = pio_read_32(&regs->rbr);
		indev_push_character(instance->input, data);
	}
}

/** IRQ handler for the DW_APB_UART. */
static void dwapb_irq_handler(irq_t *irq)
{
	dwapb_instance_t *instance = irq->instance;
	uint32_t iir_iid = pio_read_32(&instance->regs->iir) & IIR_IID_MASK;

	switch (iir_iid) {
	case IID_BUSY_DETECT:
		dwapb_irq_busy_detect(instance);
		break;
	case IID_RECEIVED_DATA:
	case IID_CHAR_TIMEOUT:
		dwapb_irq_received_data(instance);
		break;
	default:
		/* Not implemented. */
		break;
	}
}

/** Put a character to the output device. */
static void dwapb_putuchar(outdev_t *dev, char32_t ch)
{
	dwapb_instance_t *instance = (dwapb_instance_t *) dev->data;

	if ((!instance->parea.mapped) || (console_override)) {
		if (ch == '\n')
			dwapb_sendb(instance, '\r');

		if (ascii_check(ch))
			dwapb_sendb(instance, (uint8_t) ch);
		else
			dwapb_sendb(instance, U_SPECIAL);
	}
}

static outdev_operations_t dwapb_ops = {
	.write = dwapb_putuchar,
	.redraw = NULL
};

/** Initialize the DW_APB_UART device. */
static void dwapb_device_init(dwapb_instance_t *instance)
{
	(void) instance;

	/* Nothing to do. We rely on the bootloader to initialize the device. */
}

/** Enable interrupts for the DW_APB_UART device. */
static void dwapb_enable_interrupts(dwapb_instance_t *instance)
{
	dwapb_regs_t *regs = instance->regs;

	/* Enable Received Data Available Interrupt. */
	pio_write_32(&regs->ier, IER_ERBFI);
}

/** Initialize dwapb.
 *
 * @param dev_phys   Address of the beginning of the device in I/O space.
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
dwapb_instance_t *dwapb_init(ioport32_t *dev_phys, inr_t inr, cir_t cir,
    void *cir_arg, outdev_t **output)
{
	size_t size = sizeof(dwapb_regs_t);
	ioport32_t *dev = pio_map((void *) dev_phys, size);

	dwapb_instance_t *instance = malloc(sizeof(dwapb_instance_t));
	if (instance) {
		instance->regs = (dwapb_regs_t *)dev;
		instance->input = NULL;
		instance->output = NULL;

		if (output) {
			instance->output = malloc(sizeof(outdev_t));
			if (!instance->output) {
				free(instance);
				pio_unmap((void *) dev_phys, (void *) dev,
				    size);
				return NULL;
			}

			outdev_initialize("dwapb", instance->output,
			    &dwapb_ops);
			instance->output->data = instance;
			*output = instance->output;
		}

		irq_initialize(&instance->irq);
		instance->irq.inr = inr;
		instance->irq.claim = dwapb_claim;
		instance->irq.handler = dwapb_irq_handler;
		instance->irq.instance = instance;
		instance->irq.cir = cir;
		instance->irq.cir_arg = cir_arg;

		ddi_parea_init(&instance->parea);
		instance->parea.pbase = ALIGN_DOWN((uintptr_t) dev_phys,
		    PAGE_SIZE);
		instance->parea.frames = ALIGN_UP(size, PAGE_SIZE);
		instance->parea.unpriv = false;
		instance->parea.mapped = false;
		ddi_parea_register(&instance->parea);
	}

	dwapb_device_init(instance);

	return instance;
}

/** Wire the DW_APB_UART device to an input device.
 *
 * @param input     Input device to wire to.
 */
void dwapb_wire(dwapb_instance_t *instance, indev_t *input)
{
	assert(instance);
	assert(input);

	instance->input = input;
	irq_register(&instance->irq);

	dwapb_clear_buffer(instance);
	dwapb_enable_interrupts(instance);
}

/** @}
 */
