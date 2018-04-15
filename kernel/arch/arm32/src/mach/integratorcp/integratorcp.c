/*
 * Copyright (c) 2009 Vineeth Pillai
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

/** @addtogroup arm32integratorcp
 * @{
 */
/** @file
 *  @brief ICP drivers.
 */

#include <interrupt.h>
#include <ipc/irq.h>
#include <console/chardev.h>
#include <genarch/drivers/pl011/pl011.h>
#include <genarch/drivers/pl050/pl050.h>
#include <genarch/kbrd/kbrd.h>
#include <genarch/srln/srln.h>
#include <console/console.h>
#include <stdbool.h>
#include <sysinfo/sysinfo.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/km.h>
#include <arch/mm/frame.h>
#include <arch/mach/integratorcp/integratorcp.h>
#include <genarch/fb/fb.h>
#include <abi/fb/visuals.h>
#include <ddi/ddi.h>
#include <log.h>


#define SDRAM_SIZE \
	sdram[(*(uint32_t *) (ICP_CMCR + ICP_SDRAMCR_OFFSET) & ICP_SDRAM_MASK) >> 2]

static struct {
	icp_hw_map_t hw_map;
	irq_t timer_irq;
	pl011_uart_t uart;
} icp;

struct arm_machine_ops icp_machine_ops = {
	icp_init,
	icp_timer_irq_start,
	icp_cpu_halt,
	icp_get_memory_extents,
	icp_irq_exception,
	icp_frame_init,
	icp_output_init,
	icp_input_init,
	icp_get_irq_count,
	icp_get_platform_name
};

static bool hw_map_init_called = false;
uint32_t sdram[8] = {
	16777216,	/* 16mb */
	33554432,	/* 32mb */
	67108864,	/* 64mb */
	134217728,	/* 128mb */
	268435456,	/* 256mb */
	0,		/* Reserved */
	0,		/* Reserved */
	0		/* Reserved */
};

void icp_vga_init(void);

/** Initializes the vga
 *
 */
void icp_vga_init(void)
{
	*(uint32_t *) ((char *)(icp.hw_map.cmcr) + 0x14) = 0xA05F0000;
	*(uint32_t *) ((char *)(icp.hw_map.cmcr) + 0x1C) = 0x12C11000;
	*(uint32_t *) icp.hw_map.vga = 0x3F1F3F9C;
	*(uint32_t *) ((char *)(icp.hw_map.vga) + 0x4) = 0x080B61DF;
	*(uint32_t *) ((char *)(icp.hw_map.vga) + 0x8) = 0x067F3800;
	*(uint32_t *) ((char *)(icp.hw_map.vga) + 0x10) = ICP_FB;
	*(uint32_t *) ((char *)(icp.hw_map.vga) + 0x1C) = 0x182B;
	*(uint32_t *) ((char *)(icp.hw_map.cmcr) + 0xC) = 0x33805000;

}

/** Returns the mask of active interrupts. */
static inline uint32_t icp_irqc_get_sources(void)
{
	return *((uint32_t *) icp.hw_map.irqc);
}

/** Masks interrupt.
 *
 * @param irq interrupt number
 */
static inline void icp_irqc_mask(uint32_t irq)
{
	*((uint32_t *) icp.hw_map.irqc_mask) = (1 << irq);
}


/** Unmasks interrupt.
 *
 * @param irq interrupt number
 */
static inline void icp_irqc_unmask(uint32_t irq)
{
	*((uint32_t *) icp.hw_map.irqc_unmask) |= (1 << irq);
}

/** Initializes icp.hw_map. */
void icp_init(void)
{
	icp.hw_map.uart = km_map(ICP_UART, PAGE_SIZE,
	    PAGE_WRITE | PAGE_NOT_CACHEABLE);
	icp.hw_map.kbd_ctrl = km_map(ICP_KBD, PAGE_SIZE, PAGE_NOT_CACHEABLE);
	icp.hw_map.kbd_stat = icp.hw_map.kbd_ctrl + ICP_KBD_STAT;
	icp.hw_map.kbd_data = icp.hw_map.kbd_ctrl + ICP_KBD_DATA;
	icp.hw_map.kbd_intstat = icp.hw_map.kbd_ctrl + ICP_KBD_INTR_STAT;
	icp.hw_map.rtc = km_map(ICP_RTC, PAGE_SIZE,
	    PAGE_WRITE | PAGE_NOT_CACHEABLE);
	icp.hw_map.rtc1_load = icp.hw_map.rtc + ICP_RTC1_LOAD_OFFSET;
	icp.hw_map.rtc1_read = icp.hw_map.rtc + ICP_RTC1_READ_OFFSET;
	icp.hw_map.rtc1_ctl = icp.hw_map.rtc + ICP_RTC1_CTL_OFFSET;
	icp.hw_map.rtc1_intrclr = icp.hw_map.rtc + ICP_RTC1_INTRCLR_OFFSET;
	icp.hw_map.rtc1_bgload = icp.hw_map.rtc + ICP_RTC1_BGLOAD_OFFSET;
	icp.hw_map.rtc1_intrstat = icp.hw_map.rtc + ICP_RTC1_INTRSTAT_OFFSET;

	icp.hw_map.irqc = km_map(ICP_IRQC, PAGE_SIZE,
	    PAGE_WRITE | PAGE_NOT_CACHEABLE);
	icp.hw_map.irqc_mask = icp.hw_map.irqc + ICP_IRQC_MASK_OFFSET;
	icp.hw_map.irqc_unmask = icp.hw_map.irqc + ICP_IRQC_UNMASK_OFFSET;
	icp.hw_map.cmcr = km_map(ICP_CMCR, PAGE_SIZE,
	    PAGE_WRITE | PAGE_NOT_CACHEABLE);
	icp.hw_map.sdramcr = icp.hw_map.cmcr + ICP_SDRAMCR_OFFSET;
	icp.hw_map.vga = km_map(ICP_VGA, PAGE_SIZE,
	    PAGE_WRITE | PAGE_NOT_CACHEABLE);

	hw_map_init_called = true;
}

/** Starts icp Real Time Clock device, which asserts regular interrupts.
 *
 * @param frequency Interrupts frequency (0 disables RTC).
 */
static void icp_timer_start(uint32_t frequency)
{
	icp_irqc_mask(ICP_TIMER_IRQ);
	*((uint32_t *) icp.hw_map.rtc1_load) = frequency;
	*((uint32_t *) icp.hw_map.rtc1_bgload) = frequency;
	*((uint32_t *) icp.hw_map.rtc1_ctl) = ICP_RTC_CTL_VALUE;
	icp_irqc_unmask(ICP_TIMER_IRQ);
}

static irq_ownership_t icp_timer_claim(irq_t *irq)
{
	if (icp.hw_map.rtc1_intrstat) {
		*((uint32_t *) icp.hw_map.rtc1_intrclr) = 1;
		return IRQ_ACCEPT;
	} else
		return IRQ_DECLINE;
}

/** Timer interrupt handler.
 *
 * @param irq Interrupt information.
 * @param arg Not used.
 */
static void icp_timer_irq_handler(irq_t *irq)
{
	/*
	* We are holding a lock which prevents preemption.
	* Release the lock, call clock() and reacquire the lock again.
	*/

	spinlock_unlock(&irq->lock);
	clock();
	spinlock_lock(&irq->lock);

}

/** Initializes and registers timer interrupt handler. */
static void icp_timer_irq_init(void)
{
	irq_initialize(&icp.timer_irq);
	icp.timer_irq.inr = ICP_TIMER_IRQ;
	icp.timer_irq.claim = icp_timer_claim;
	icp.timer_irq.handler = icp_timer_irq_handler;

	irq_register(&icp.timer_irq);
}

/** Starts timer.
 *
 * Initiates regular timer interrupts after initializing
 * corresponding interrupt handler.
 */
void icp_timer_irq_start(void)
{
	icp_timer_irq_init();
	icp_timer_start(ICP_TIMER_FREQ);
}

/** Get extents of available memory.
 *
 * @param start		Place to store memory start address.
 * @param size		Place to store memory size.
 */
void icp_get_memory_extents(uintptr_t *start, size_t *size)
{
	*start = 0;

	if (hw_map_init_called) {
		*size = sdram[(*(uint32_t *) icp.hw_map.sdramcr &
		    ICP_SDRAM_MASK) >> 2];
	} else {
		*size = SDRAM_SIZE;
	}
}

/** Stops icp. */
void icp_cpu_halt(void)
{
	while (true)
		;
}

/** interrupt exception handler.
 *
 * Determines sources of the interrupt from interrupt controller and
 * calls high-level handlers for them.
 *
 * @param exc_no Interrupt exception number.
 * @param istate Saved processor state.
 */
void icp_irq_exception(unsigned int exc_no, istate_t *istate)
{
	uint32_t sources = icp_irqc_get_sources();
	unsigned int i;

	for (i = 0; i < ICP_IRQC_MAX_IRQ; i++) {
		if (sources & (1 << i)) {
			irq_t *irq = irq_dispatch_and_lock(i);
			if (irq) {
				/* The IRQ handler was found. */
				irq->handler(irq);
				spinlock_unlock(&irq->lock);
			} else {
				/* Spurious interrupt.*/
				log(LF_ARCH, LVL_DEBUG,
				    "cpu%d: spurious interrupt (inum=%d)",
				    CPU->id, i);
			}
		}
	}
}

/*
 * Integrator specific frame initialization
 */
void
icp_frame_init(void)
{
	frame_mark_unavailable(ICP_FB_FRAME, ICP_FB_NUM_FRAME);
	frame_mark_unavailable(0, 256);
}

void icp_output_init(void)
{
#ifdef CONFIG_FB
	static bool vga_init = false;
	if (!vga_init) {
		icp_vga_init();
		vga_init = true;
	}

	fb_properties_t prop = {
		.addr = ICP_FB,
		.offset = 0,
		.x = 640,
		.y = 480,
		.scan = 2560,
		.visual = VISUAL_RGB_8_8_8_0,
	};

	outdev_t *fbdev = fb_init(&prop);
	if (fbdev)
		stdout_wire(fbdev);
#endif
#ifdef CONFIG_PL011_UART
	if (pl011_uart_init(&icp.uart, ICP_UART0_IRQ, ICP_UART))
		stdout_wire(&icp.uart.outdev);
#endif
}

void icp_input_init(void)
{

	pl050_t *pl050 = malloc(sizeof(pl050_t), FRAME_ATOMIC);
	pl050->status = (ioport8_t *) icp.hw_map.kbd_stat;
	pl050->data = (ioport8_t *) icp.hw_map.kbd_data;
	pl050->ctrl = (ioport8_t *) icp.hw_map.kbd_ctrl;

	pl050_instance_t *pl050_instance = pl050_init(pl050, ICP_KBD_IRQ);
	if (pl050_instance) {
		kbrd_instance_t *kbrd_instance = kbrd_init();
		if (kbrd_instance) {
			icp_irqc_mask(ICP_KBD_IRQ);
			indev_t *sink = stdin_wire();
			indev_t *kbrd = kbrd_wire(kbrd_instance, sink);
			pl050_wire(pl050_instance, kbrd);
			icp_irqc_unmask(ICP_KBD_IRQ);
		}
	}

	/*
	 * This is the necessary evil until the userspace driver is entirely
	 * self-sufficient.
	 */
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.inr", NULL, ICP_KBD_IRQ);
	sysinfo_set_item_val("kbd.address.physical", NULL, ICP_KBD);

#ifdef CONFIG_PL011_UART
	srln_instance_t *srln_instance = srln_init();
	if (srln_instance) {
		indev_t *sink = stdin_wire();
		indev_t *srln = srln_wire(srln_instance, sink);
		pl011_uart_input_wire(&icp.uart, srln);
		icp_irqc_unmask(ICP_UART0_IRQ);
	}
#endif
}

size_t icp_get_irq_count(void)
{
	return ICP_IRQ_COUNT;
}

const char *icp_get_platform_name(void)
{
	return "integratorcp";
}

/** @}
 */
