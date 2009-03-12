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

#include <arch.h>
#include <arch/drivers/sgcn.h>
#include <arch/drivers/kbd.h>
#include <genarch/ofw/ofw_tree.h>
#include <debug.h>
#include <string.h>
#include <print.h>
#include <mm/page.h>
#include <proc/thread.h>
#include <console/chardev.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <synch/spinlock.h>

#define POLL_INTERVAL		10000

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
 * has already made necessary arrangements so that the output will
 * be read from the OBP buffer and input will go to the OBP buffer.
 * Therefore HelenOS needs to make no such arrangements any more.
 */
#define CONSOLE_KEY		"OBPCONS"

/* magic string contained at the beginning of the console buffer */
#define SGCN_BUFFER_MAGIC	"CON"

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

/* true iff the kernel driver should ignore pressed keys */
static bool kbd_disabled;

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
static void sgcn_putchar(outdev_t *, const char, bool);

/** SGCN output device operations */
static outdev_operations_t sgcnout_ops = {
	.write = sgcn_putchar
};

/** SGCN input device operations */
static indev_operations_t sgcnin_ops = {
	.poll = NULL
};

static indev_t sgcnin;		/**< SGCN input device. */
static outdev_t sgcnout;	/**< SGCN output device. */

/**
 * Set some sysinfo values (SRAM address and SRAM size).
 */
static void register_sram(uintptr_t sram_begin_physical)
{
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
 */
static void init_sram_begin(void)
{
	ofw_tree_node_t *chosen;
	ofw_tree_property_t *iosram_toc;
	uintptr_t sram_begin_physical;

	chosen = ofw_tree_lookup("/chosen");
	if (!chosen)
		panic("Cannot find '/chosen'.");

	iosram_toc = ofw_tree_getprop(chosen, "iosram-toc");
	if (!iosram_toc)
		panic("Cannot find property 'iosram-toc'.");
	if (!iosram_toc->value)
		panic("Cannot find SRAM TOC.");

	sram_begin_physical = SBBC_START + SBBC_SRAM_OFFSET
	    + *((uint32_t *) iosram_toc->value);
	sram_begin = hw_map(sram_begin_physical, MAPPED_AREA_SIZE);
	
	register_sram(sram_begin_physical);
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
	static bool initialized;
	
	if (initialized)
		return;

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
	
	initialized = true;
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
static void sgcn_putchar(outdev_t *od, const char c, bool silent)
{
	if (!silent) {
		spinlock_lock(&sgcn_output_lock);
		
		sgcn_do_putchar(c);
		if (c == '\n')
			sgcn_do_putchar('\r');
		
		spinlock_unlock(&sgcn_output_lock);
	}
}

/**
 * Grabs the input for kernel.
 */
void sgcn_grab(void)
{
	kbd_disabled = true;
}

/**
 * Releases the input so that userspace can use it.
 */
void sgcn_release(void)
{
	kbd_disabled = true;
}

/**
 * Function regularly called by the keyboard polling thread. Finds out whether
 * there are some unread characters in the input queue. If so, it picks them up
 * and sends them to the upper layers of HelenOS.
 */
static void sgcn_poll()
{
	uint32_t begin = SGCN_BUFFER_HEADER->in_begin;
	uint32_t end = SGCN_BUFFER_HEADER->in_end;
	uint32_t size = end - begin;

	if (kbd_disabled)
		return;

	spinlock_lock(&sgcn_input_lock);
	
	/* we need pointers to volatile variables */
	volatile char *buf_ptr = (volatile char *)
	    SGCN_BUFFER(char, SGCN_BUFFER_HEADER->in_rdptr);
	volatile uint32_t *in_wrptr_ptr = &(SGCN_BUFFER_HEADER->in_wrptr);
	volatile uint32_t *in_rdptr_ptr = &(SGCN_BUFFER_HEADER->in_rdptr);
	
	while (*in_rdptr_ptr != *in_wrptr_ptr) {
		
		buf_ptr = (volatile char *)
		    SGCN_BUFFER(char, SGCN_BUFFER_HEADER->in_rdptr);
		char c = *buf_ptr;
		*in_rdptr_ptr = (((*in_rdptr_ptr) - begin + 1) % size) + begin;
			
		indev_push_character(&sgcnin, c);	
	}	

	spinlock_unlock(&sgcn_input_lock);
}

/**
 * Polling thread function.
 */
static void kkbdpoll(void *arg) {
	while (1) {
		if (!silent) {
			sgcn_poll();
		}
		thread_usleep(POLL_INTERVAL);
	}
}

/**
 * A public function which initializes input from the Serengeti console.
 */
indev_t *sgcnin_init(void)
{
	sgcn_buffer_begin_init();

	kbd_type = KBD_SGCN;

	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.type", NULL, KBD_SGCN);

	thread_t *t = thread_create(kkbdpoll, NULL, TASK, 0, "kkbdpoll", true);
	if (!t)
		panic("Cannot create kkbdpoll.");
	thread_ready(t);
	
	indev_initialize("sgcnin", &sgcnin, &sgcnin_ops);

	return &sgcnin;
}

/**
 * A public function which initializes output to the Serengeti console.
 */
void sgcnout_init(void)
{
	sgcn_buffer_begin_init();

	sysinfo_set_item_val("fb.kind", NULL, 4);

	outdev_initialize("sgcnout", &sgcnout, &sgcnout_ops);	
	stdout = &sgcnout;
}

/** @}
 */
