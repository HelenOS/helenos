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
 * @brief	Niagara input/output driver based on hypervisor calls.
 */

#include <arch/drivers/niagara.h>
#include <console/chardev.h>
#include <console/console.h>
#include <ddi/ddi.h>
#include <ddi/device.h>
#include <arch/asm.h>
#include <arch.h>
#include <mm/slab.h>
#include <arch/drivers/kbd.h>
#include <arch/sun4v/hypercall.h>
#include <sysinfo/sysinfo.h>
#include <ipc/irq.h>
#include <print.h>
#include <proc/thread.h>
#include <console/console.h>
#include <genarch/srln/srln.h>

#define POLL_INTERVAL  10000

/**
 * The driver is polling based, but in order to notify the userspace
 * of a key being pressed, we need to supply the interface with some
 * interrupt number. The interrupt number can be arbitrary as it it
 * will never be used for identifying HW interrupts, but only in
 * notifying the userspace. 
 */
#define FICTIONAL_INR		1


static niagara_instance_t *instance = NULL;

static void niagara_putchar(outdev_t *, const wchar_t, bool);

/** character device operations */
static outdev_operations_t niagara_ops = {
	.write = niagara_putchar,
	.redraw = NULL
};

/*
 * The driver uses hypercalls to print characters to the console. Since the
 * hypercall cannot be performed from the userspace, we do this:
 * The kernel "little brother" driver (which will be present no matter what the
 * DDI architecture is - as we need the kernel part for the kconsole)
 * defines a shared buffer. Kernel walks through the buffer (in the same thread
 * which is used for polling the keyboard) and prints any pending characters
 * to the console (using hypercalls). The userspace fb server maps this shared
 * buffer to its address space and output operation it does is performed using
 * the mapped buffer. The shared buffer definition follows.
 */
/*
#define OUTPUT_BUFFER_SIZE	((PAGE_SIZE) - 2 * 8)
static volatile struct {
	uint64_t read_ptr;
	uint64_t write_ptr;
	char data[OUTPUT_BUFFER_SIZE];
}
	__attribute__ ((packed))
	__attribute__ ((aligned(PAGE_SIZE)))
	output_buffer;
*/

#if 0
/** Niagara character device */
chardev_t niagara_io;

/** defined in drivers/kbd.c */
extern kbd_type_t kbd_type;

/**
 * The character read will be stored here until the (notified) uspace
 * driver picks it up.
 */
static char read_char;


#endif

/** Writes a single character to the standard output. */
static inline void do_putchar(const char c) {
	/* repeat until the buffer is non-full */
	while (__hypercall_fast1(CONS_PUTCHAR, c) == EWOULDBLOCK)
		;
}

/** Writes a single character to the standard output. */
static void niagara_putchar(outdev_t *dev, const wchar_t ch, bool silent)
{
	do_putchar(ch);
	if (ch == '\n')
		do_putchar('\r');
}

#if 0
/**
 * Grabs the input for kernel.
 */
void niagara_grab(void)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&niagara_irq.lock);
	niagara_irq.notif_cfg.notify = false;
	spinlock_unlock(&niagara_irq.lock);
	interrupts_restore(ipl);
}

/**
 * Releases the input so that userspace can use it.
 */
void niagara_release(void)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&niagara_irq.lock);
	if (niagara_irq.notif_cfg.answerbox)
		niagara_irq.notif_cfg.notify = true;
	spinlock_unlock(&niagara_irq.lock);
	interrupts_restore(ipl);
}

/**
 * Default suspend/resume operation for the input device.
 */
static void niagara_noop(chardev_t *d)
{
}

/**
 * Called when actively reading the character.
 */
static char niagara_read(chardev_t *d)
{
	uint64_t c;

	if (__hypercall_fast_ret1(0, 0, 0, 0, 0, CONS_GETCHAR, &c) == EOK) {
		return (char) c;
	}

	return '\0';
}

/**
 * Returns the character last read. This function is called from the
 * pseudocode - the character returned by this function is passed to
 * the userspace keyboard driver.
 */
char niagara_getc(void) {
	return read_char;
}

#endif

/**
 * Function regularly called by the keyboard polling thread. Asks the
 * hypervisor whether there is any unread character. If so, it picks it up
 * and sends it to the upper layers of HelenOS.
 */
static void niagara_poll(niagara_instance_t *instance)
{
	/*
	while (output_buffer.read_ptr != output_buffer.write_ptr) {
		do_putchar(output_buffer.data[output_buffer.read_ptr]);
		output_buffer.read_ptr =
			((output_buffer.read_ptr) + 1) % OUTPUT_BUFFER_SIZE;
	}
	*/

	uint64_t c;

	if (__hypercall_fast_ret1(0, 0, 0, 0, 0, CONS_GETCHAR, &c) == EOK) {
		indev_push_character(instance->srlnin, c);
	}

}

/**
 * Polling thread function.
 */
static void kniagarapoll(void *instance) {
	while (true) {
		//MH
		//if (!silent)
			niagara_poll(instance);
		
		thread_usleep(POLL_INTERVAL);
	}
}

/**
 * Initializes the input/output subsystem so that the Niagara standard
 * input/output is used.
 */
static void niagara_init(void)
{
	if (instance)
		return;
	
	instance = malloc(sizeof(niagara_instance_t), FRAME_ATOMIC);
	
	if (instance) {
		instance->thread = thread_create(kniagarapoll, instance, TASK, 0,
			"kniagarapoll", true);
		
		if (!instance->thread) {
			free(instance);
			instance = NULL;
			return;
		}
	}

	instance->srlnin = NULL;

	#if 0
	kbd_type = KBD_SUN4V;

	devno_t devno = device_assign_devno();
	irq_initialize(&niagara_irq);
	niagara_irq.devno = devno;
	niagara_irq.inr = FICTIONAL_INR;
	niagara_irq.claim = niagara_claim;
	niagara_irq.handler = niagara_irq_handler;
	irq_register(&niagara_irq);
	
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.type", NULL, KBD_SUN4V);
	sysinfo_set_item_val("kbd.devno", NULL, devno);
	sysinfo_set_item_val("kbd.inr", NULL, FICTIONAL_INR);
	sysinfo_set_item_val("fb.kind", NULL, 5);
	#endif

	/*
	 * Set sysinfos and pareas so that the userspace counterpart of the
	 * niagara fb driver can communicate with kernel using a shared buffer.
 	 */
	//output_buffer.read_ptr = 0;
	//output_buffer.write_ptr = 0;

	#if 0
	sysinfo_set_item_val("niagara.outbuf.address", NULL,
		KA2PA(&output_buffer));
	sysinfo_set_item_val("niagara.outbuf.size", NULL,
		PAGE_SIZE);
	sysinfo_set_item_val("niagara.outbuf.datasize", NULL,
		OUTPUT_BUFFER_SIZE);
	static parea_t outbuf_parea;
	outbuf_parea.pbase = (uintptr_t) (KA2PA(&output_buffer));
	outbuf_parea.vbase = (uintptr_t) (&output_buffer);
	outbuf_parea.frames = 1;
	outbuf_parea.cacheable = false;
	ddi_parea_register(&outbuf_parea);

	#endif

	outdev_t *niagara_dev = malloc(sizeof(outdev_t), FRAME_ATOMIC);
	outdev_initialize("niagara_dev", niagara_dev, &niagara_ops);
	stdout_wire(niagara_dev);
}

/**
 * A public function which initializes input from the Niagara console.
 */
niagara_instance_t *niagarain_init(void)
{
	niagara_init();

	if (instance) {
		srln_instance_t *srln_instance = srln_init();
		if (srln_instance) {
			indev_t *sink = stdin_wire();
			indev_t *srln = srln_wire(srln_instance, sink);

			// wire std. input to niagara
			instance->srlnin = srln;
			thread_ready(instance->thread);
		}
	}
	return instance;
}

/** @}
 */
