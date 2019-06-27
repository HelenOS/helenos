/*
 * Copyright (c) 2013 Jakub Jermar
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

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 *  @brief MIPS Malta platform driver.
 */

#include <arch/mach/malta/malta.h>
#include <console/console.h>
#include <console/chardev.h>
#include <arch/mm/page.h>
#include <genarch/drivers/i8259/i8259.h>
#include <genarch/drivers/ns16550/ns16550.h>
#include <genarch/srln/srln.h>
#include <arch/interrupt.h>
#include <stdbool.h>
#include <byteorder.h>
#include <sysinfo/sysinfo.h>
#include <log.h>

static void malta_init(void);
static void malta_cpu_halt(void);
static void malta_get_memory_extents(uintptr_t *, size_t *);
static void malta_frame_init(void);
static void malta_output_init(void);
static void malta_input_init(void);
static const char *malta_get_platform_name(void);

struct mips32_machine_ops malta_machine_ops = {
	.machine_init = malta_init,
	.machine_cpu_halt = malta_cpu_halt,
	.machine_get_memory_extents = malta_get_memory_extents,
	.machine_frame_init = malta_frame_init,
	.machine_output_init = malta_output_init,
	.machine_input_init = malta_input_init,
	.machine_get_platform_name = malta_get_platform_name
};

#ifdef CONFIG_NS16550
static ns16550_instance_t *tty_instance;
#endif
#ifdef CONFIG_NS16550_OUT
static outdev_t *tty_out;
#endif

static void malta_isa_irq_handler(unsigned int i)
{
	uint8_t isa_irq = host2uint32_t_le(pio_read_32(GT64120_PCI0_INTACK));
	if (i8259_is_spurious(isa_irq)) {
		i8259_handle_spurious(isa_irq);
#ifdef CONFIG_DEBUG
		log(LF_ARCH, LVL_DEBUG, "cpu%u: PIC spurious interrupt %u",
		    CPU->id, isa_irq);
		return;
#endif
	}
	irq_t *irq = irq_dispatch_and_lock(isa_irq);
	if (irq) {
		irq->handler(irq);
		irq_spinlock_unlock(&irq->lock, false);
	} else {
#ifdef CONFIG_DEBUG
		log(LF_ARCH, LVL_DEBUG, "cpu%u: unhandled IRQ (irq=%u)",
		    CPU->id, isa_irq);
#endif
	}
	i8259_eoi(isa_irq);
}

void malta_init(void)
{
	irq_init(ISA_IRQ_COUNT, ISA_IRQ_COUNT);

	i8259_init((i8259_t *) PIC0_BASE, (i8259_t *) PIC1_BASE, 0);
	sysinfo_set_item_val("i8259", NULL, true);

	int_handler[INT_HW0] = malta_isa_irq_handler;
	cp0_unmask_int(INT_HW0);

#if (defined(CONFIG_NS16550) || defined(CONFIG_NS16550_OUT))
#ifdef CONFIG_NS16550_OUT
	outdev_t **tty_out_ptr = &tty_out;
#else
	outdev_t **tty_out_ptr = NULL;
#endif
	tty_instance = ns16550_init((ioport8_t *) TTY_BASE, 0, TTY_ISA_IRQ,
	    NULL, NULL, tty_out_ptr);
#endif
}

void malta_cpu_halt(void)
{
}

void malta_get_memory_extents(uintptr_t *start, size_t *size)
{
}

void malta_frame_init(void)
{
}

void malta_output_init(void)
{
#ifdef CONFIG_NS16550_OUT
	if (tty_out)
		stdout_wire(tty_out);
#endif
}

void malta_input_init(void)
{
#ifdef CONFIG_NS16550
	if (tty_instance) {
		srln_instance_t *srln_instance = srln_init();
		if (srln_instance) {
			indev_t *sink = stdin_wire();
			indev_t *srln = srln_wire(srln_instance, sink);
			ns16550_wire(tty_instance, srln);
			i8259_enable_irqs(1 << TTY_ISA_IRQ);
		}
	}
#endif
}

const char *malta_get_platform_name(void)
{
	return "malta";
}

/** @}
 */
