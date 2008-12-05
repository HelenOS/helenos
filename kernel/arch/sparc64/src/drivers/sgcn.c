/*
 * Copyright (c) 2008 Pavel Rimsky
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

/** @addtogroup sparc64
 * @{
 */
/**
 * @file
 * @brief	SGCN driver.
 */

#include <arch/drivers/sgcn.h>
#include <arch/drivers/kbd.h>
#include <genarch/ofw/ofw_tree.h>
#include <debug.h>
#include <func.h>
#include <print.h>
#include <mm/page.h>
#include <ipc/irq.h>
#include <ddi/ddi.h>
#include <ddi/device.h>
#include <console/chardev.h>
#include <console/console.h>
#include <ddi/device.h>
#include <sysinfo/sysinfo.h>
#include <synch/spinlock.h>

/*
 * Physical address at which the SBBC starts. This value has been obtained
 * by inspecting (using Simics) memory accesses made by OBP. It is valid
 * for the Simics-simulated Serengeti machine. The author of this code is
 * not sure whether this value is valid generally. 
 */
#define SBBC_START		0x63000000000

/* offset of SRAM within the SBBC memory */
#define SBBC_SRAM_OFFSET	0x900000

/* size (in bytes) of the physical memory area which will be mapped */
#define MAPPED_AREA_SIZE	(128 * 1024)

/* magic string contained at the beginning of SRAM */
#define SRAM_TOC_MAGIC		"TOCSRAM"

/*
 * Key into the SRAM table of contents which identifies the entry
 * describing the OBP console buffer. It is worth mentioning
 * that the OBP console buffer is not the only console buffer
 * which can be used. It is, however, used because when the kernel
 * is running, the OBP buffer is not used by OBP any more but OBP
 * has already made neccessary arangements so that the output will
 * be read from the OBP buffer and input will go to the OBP buffer.
 * Therefore HelenOS needs to make no such arrangements any more.
 */
#define CONSOLE_KEY		"OBPCONS"

/* magic string contained at the beginning of the console buffer */
#define SGCN_BUFFER_MAGIC	"CON"

/**
 * The driver is polling based, but in order to notify the userspace
 * of a key being pressed, we need to supply the interface with some
 * interrupt number. The interrupt number can be arbitrary as it it
 * will never be used for identifying HW interrupts, but only in
 * notifying the userspace. 
 */
#define FICTIONAL_INR		1


/*
 * Returns a pointer to the object of a given type which is placed at the given
 * offset from the SRAM beginning.
 */
#define SRAM(type, offset)	((type *) (sram_begin + (offset)))

/* Returns a pointer to the SRAM table of contents. */
#define SRAM_TOC		(SRAM(iosram_toc_t, 0))

/*
 * Returns a pointer to the object of a given type which is placed at the given
 * offset from the console buffer beginning.
 */
#define SGCN_BUFFER(type, offset) \
				((type *) (sgcn_buffer_begin + (offset)))

/** Returns a pointer to the console buffer header. */
#define SGCN_BUFFER_HEADER	(SGCN_BUFFER(sgcn_buffer_header_t, 0))

/** defined in drivers/kbd.c */
extern kbd_type_t kbd_type;

/** starting address of SRAM, will be set by the init_sram_begin function */
static uintptr_t sram_begin;

/**
 * starting address of the SGCN buffer, will be set by the
 * init_sgcn_buffer_begin function
 */
static uintptr_t sgcn_buffer_begin;

/**
 * SGCN IRQ structure. So far used only for notifying the userspace of the
 * key being pressed, not for kernel being informed about keyboard interrupts.
 */ 
static irq_t sgcn_irq;

// TODO think of a way how to synchronize accesses to SGCN buffer between the kernel and the userspace

/* 
 * Ensures that writing to the buffer and consequent update of the write pointer
 * are together one atomic operation.
 */
SPINLOCK_INITIALIZE(sgcn_output_lock);

/* 
 * Prevents the input buffer read/write pointers from getting to inconsistent
 * state. 
 */
SPINLOCK_INITIALIZE(sgcn_input_lock);


/* functions referenced from definitions of I/O operations structures */
static void sgcn_noop(chardev_t *);
static void sgcn_putchar(chardev_t *, const char);
static char sgcn_key_read(chardev_t *);

/** character device operations */
static chardev_operations_t sgcn_ops = {
	.suspend = sgcn_noop,
	.resume = sgcn_noop,
	.read = sgcn_key_read,
	.write = sgcn_putchar
};

/** SGCN character device */
chardev_t sgcn_io;

/**
 * Registers the physical area of the SRAM so that the userspace SGCN
 * driver can map it. Moreover, it sets some sysinfo values (SRAM address
 * and SRAM size).
 */
static void register_sram_parea(uintptr_t sram_begin_physical)
{
	static parea_t sram_parea;
	sram_parea.pbase = sram_begin_physical;
	sram_parea.vbase = (uintptr_t) sram_begin;
	sram_parea.frames = MAPPED_AREA_SIZE / FRAME_SIZE;
	sram_parea.cacheable = false;
	ddi_parea_register(&sram_parea);
	
	sysinfo_set_item_val("sram.area.size", NULL, MAPPED_AREA_SIZE);
	sysinfo_set_item_val("sram.address.physical", NULL,
		sram_begin_physical);
}

/**
 * Initializes the starting address of SRAM.
 *
 * The SRAM starts 0x900000 + C bytes behind the SBBC start in the
 * physical memory, where C is the value read from the "iosram-toc"
 * property of the "/chosen" OBP node. The sram_begin variable will
 * be set to the virtual address which maps to the SRAM physical
 * address.
 *
 * It also registers the physical area of SRAM and sets some sysinfo
 * values (SRAM address and SRAM size).
 */
static void init_sram_begin(void)
{
	ofw_tree_node_t *chosen;
	ofw_tree_property_t *iosram_toc;
	uintptr_t sram_begin_physical;

	chosen = ofw_tree_lookup("/chosen");
	if (!chosen)
		panic("Can't find /chosen.\n");

	iosram_toc = ofw_tree_getprop(chosen, "iosram-toc");
	if (!iosram_toc)
		panic("Can't find property \"iosram-toc\".\n");
	if (!iosram_toc->value)
		panic("Can't find SRAM TOC.\n");

	sram_begin_physical = SBBC_START + SBBC_SRAM_OFFSET
		+ *((uint32_t *) iosram_toc->value);
	sram_begin = hw_map(sram_begin_physical, MAPPED_AREA_SIZE);
	
	register_sram_parea(sram_begin_physical);
}

/**
 * Initializes the starting address of the SGCN buffer.
 *
 * The offset of the SGCN buffer within SRAM is obtained from the
 * SRAM table of contents. The table of contents contains
 * information about several buffers, among which there is an OBP
 * console buffer - this one will be used as the SGCN buffer. 
 *
 * This function also writes the offset of the SGCN buffer within SRAM
 * under the sram.buffer.offset sysinfo key.
 */
static void sgcn_buffer_begin_init(void)
{
	init_sram_begin();
		
	ASSERT(strcmp(SRAM_TOC->magic, SRAM_TOC_MAGIC) == 0);
	
	/* lookup TOC entry with the correct key */
	uint32_t i;
	for (i = 0; i < MAX_TOC_ENTRIES; i++) {
		if (strcmp(SRAM_TOC->keys[i].key, CONSOLE_KEY) == 0)
			break;
	}
	ASSERT(i < MAX_TOC_ENTRIES);
	
	sgcn_buffer_begin = sram_begin + SRAM_TOC->keys[i].offset;
	
	sysinfo_set_item_val("sram.buffer.offset", NULL,
		SRAM_TOC->keys[i].offset);
}

/**
 * Default suspend/resume operation for the input device.
 */
static void sgcn_noop(chardev_t *d)
{
}

/**
 * Writes a single character to the SGCN (circular) output buffer
 * and updates the output write pointer so that SGCN gets to know
 * that the character has been written.
 */
static void sgcn_do_putchar(const char c)
{
	uint32_t begin = SGCN_BUFFER_HEADER->out_begin;
	uint32_t end = SGCN_BUFFER_HEADER->out_end;
	uint32_t size = end - begin;
	
	/* we need pointers to volatile variables */
	volatile char *buf_ptr = (volatile char *)
		SGCN_BUFFER(char, SGCN_BUFFER_HEADER->out_wrptr);
	volatile uint32_t *out_wrptr_ptr = &(SGCN_BUFFER_HEADER->out_wrptr);
	volatile uint32_t *out_rdptr_ptr = &(SGCN_BUFFER_HEADER->out_rdptr);

	/*
	 * Write the character and increment the write pointer modulo the
	 * output buffer size. Note that if we are to rewrite a character
	 * which has not been read by the SGCN controller yet (i.e. the output
	 * buffer is full), we need to wait until the controller reads some more
	 * characters. We wait actively, which means that all threads waiting
	 * for the lock are blocked. However, this situation is
	 *   1) rare - the output buffer is big, so filling the whole
	 *             output buffer is improbable
	 *   2) short-lasting - it will take the controller only a fraction
	 *             of millisecond to pick the unread characters up
	 *   3) not serious - the blocked threads are those that print something
	 *             to user console, which is not a time-critical operation
	 */
	uint32_t new_wrptr = (((*out_wrptr_ptr) - begin + 1) % size) + begin;
	while (*out_rdptr_ptr == new_wrptr)
		;
	*buf_ptr = c;
	*out_wrptr_ptr = new_wrptr;
}

/**
 * SGCN output operation. Prints a single character to the SGCN. If the line
 * feed character is written ('\n'), the carriage return character ('\r') is
 * written straight away. 
 */
static void sgcn_putchar(struct chardev * cd, const char c)
{
	spinlock_lock(&sgcn_output_lock);
	
	sgcn_do_putchar(c);
	if (c == '\n') {
		sgcn_do_putchar('\r');
	}
	
	spinlock_unlock(&sgcn_output_lock);
}

/**
 * Called when actively reading the character. Not implemented yet.
 */
static char sgcn_key_read(chardev_t *d)
{
	return (char) 0;
}

/**
 * The driver works in polled mode, so no interrupt should be handled by it.
 */
static irq_ownership_t sgcn_claim(void)
{
	return IRQ_DECLINE;
}

/**
 * The driver works in polled mode, so no interrupt should be handled by it.
 */
static void sgcn_irq_handler(irq_t *irq, void *arg, ...)
{
	panic("Not yet implemented, SGCN works in polled mode.\n");
}

/**
 * Grabs the input for kernel.
 */
void sgcn_grab(void)
{
	ipl_t ipl = interrupts_disable();
	
	volatile uint32_t *in_wrptr_ptr = &(SGCN_BUFFER_HEADER->in_wrptr);
	volatile uint32_t *in_rdptr_ptr = &(SGCN_BUFFER_HEADER->in_rdptr);
	
	/* skip all the user typed before the grab and hasn't been processed */
	spinlock_lock(&sgcn_input_lock);
	*in_rdptr_ptr = *in_wrptr_ptr;
	spinlock_unlock(&sgcn_input_lock);

	spinlock_lock(&sgcn_irq.lock);
	sgcn_irq.notif_cfg.notify = false;
	spinlock_unlock(&sgcn_irq.lock);
	
	interrupts_restore(ipl);
}

/**
 * Releases the input so that userspace can use it.
 */
void sgcn_release(void)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&sgcn_irq.lock);
	if (sgcn_irq.notif_cfg.answerbox)
		sgcn_irq.notif_cfg.notify = true;
	spinlock_unlock(&sgcn_irq.lock);
	interrupts_restore(ipl);
}

/**
 * Function regularly called by the keyboard polling thread. Finds out whether
 * there are some unread characters in the input queue. If so, it picks them up
 * and sends them to the upper layers of HelenOS.
 */
void sgcn_poll(void)
{
	uint32_t begin = SGCN_BUFFER_HEADER->in_begin;
	uint32_t end = SGCN_BUFFER_HEADER->in_end;
	uint32_t size = end - begin;
	
	spinlock_lock(&sgcn_input_lock);
	
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&sgcn_irq.lock);
	
	/* we need pointers to volatile variables */
	volatile char *buf_ptr = (volatile char *)
		SGCN_BUFFER(char, SGCN_BUFFER_HEADER->in_rdptr);
	volatile uint32_t *in_wrptr_ptr = &(SGCN_BUFFER_HEADER->in_wrptr);
	volatile uint32_t *in_rdptr_ptr = &(SGCN_BUFFER_HEADER->in_rdptr);
	
	if (*in_rdptr_ptr != *in_wrptr_ptr) {
		if (sgcn_irq.notif_cfg.notify && sgcn_irq.notif_cfg.answerbox) {
			ipc_irq_send_notif(&sgcn_irq);
			spinlock_unlock(&sgcn_irq.lock);
			interrupts_restore(ipl);
			spinlock_unlock(&sgcn_input_lock);
			return;
		}
	}
	
	spinlock_unlock(&sgcn_irq.lock);
	interrupts_restore(ipl);	

	while (*in_rdptr_ptr != *in_wrptr_ptr) {
		
		buf_ptr = (volatile char *)
			SGCN_BUFFER(char, SGCN_BUFFER_HEADER->in_rdptr);
		char c = *buf_ptr;
		*in_rdptr_ptr = (((*in_rdptr_ptr) - begin + 1) % size) + begin;
			
		if (c == '\r') {
			c = '\n';
		}
		chardev_push_character(&sgcn_io, c);	
	}	
	
	spinlock_unlock(&sgcn_input_lock);
}

/**
 * A public function which initializes I/O from/to Serengeti console
 * and sets it as a default input/output. 
 */
void sgcn_init(void)
{
	sgcn_buffer_begin_init();

	kbd_type = KBD_SGCN;

	devno_t devno = device_assign_devno();
	irq_initialize(&sgcn_irq);
	sgcn_irq.devno = devno;
	sgcn_irq.inr = FICTIONAL_INR;
	sgcn_irq.claim = sgcn_claim;
	sgcn_irq.handler = sgcn_irq_handler;
	irq_register(&sgcn_irq);
	
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.type", NULL, KBD_SGCN);
	sysinfo_set_item_val("kbd.devno", NULL, devno);
	sysinfo_set_item_val("kbd.inr", NULL, FICTIONAL_INR);
	sysinfo_set_item_val("fb.kind", NULL, 4);
	
	chardev_initialize("sgcn_io", &sgcn_io, &sgcn_ops);
	stdin = &sgcn_io;
	stdout = &sgcn_io;
}

/** @}
 */
