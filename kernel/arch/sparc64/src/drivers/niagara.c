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
#define OUTPUT_BUFFER_SIZE	((PAGE_SIZE) - 2 * 8)
static volatile struct {
	uint64_t read_ptr;
	uint64_t write_ptr;
	char data[OUTPUT_BUFFER_SIZE];
}
	__attribute__ ((packed))
	__attribute__ ((aligned(PAGE_SIZE)))
	output_buffer;

/**
 * Analogous to the output_buffer, see the previous definition.
 */
#define INPUT_BUFFER_SIZE	((PAGE_SIZE) - 2 * 8)
static volatile struct {
	uint64_t write_ptr;
	uint64_t read_ptr;
	char data[INPUT_BUFFER_SIZE];
}
	__attribute__ ((packed))
	__attribute__ ((aligned(PAGE_SIZE)))
	input_buffer;


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

/**
 * Function regularly called by the keyboard polling thread. Asks the
 * hypervisor whether there is any unread character. If so, it picks it up
 * and sends it to the upper layers of HelenOS.
 *
 * Apart from that, it also checks whether the userspace output driver has
 * pushed any characters to the output buffer. If so, it prints them.
 */
static void niagara_poll(niagara_instance_t *instance)
{
	while (output_buffer.read_ptr != output_buffer.write_ptr) {
		do_putchar(output_buffer.data[output_buffer.read_ptr]);
		output_buffer.read_ptr =
			((output_buffer.read_ptr) + 1) % OUTPUT_BUFFER_SIZE;
	}

	uint64_t c;

	if (__hypercall_fast_ret1(0, 0, 0, 0, 0, CONS_GETCHAR, &c) == EOK) {
		if (!silent) {
			indev_push_character(instance->srlnin, c);
		} else {
			input_buffer.data[input_buffer.write_ptr] = (char) c;
			input_buffer.write_ptr =
				((input_buffer.write_ptr) + 1) % INPUT_BUFFER_SIZE;
		}
	}

}

/**
 * Polling thread function.
 */
static void kniagarapoll(void *instance) {
	while (true) {
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
	sysinfo_set_item_val("fb.kind", NULL, 5);

	/*
	 * Set sysinfos and pareas so that the userspace counterpart of the
	 * niagara fb and kbd driver can communicate with kernel using shared
	 * buffers.
 	 */
	output_buffer.read_ptr = 0;
	output_buffer.write_ptr = 0;
	input_buffer.write_ptr = 0;
	input_buffer.read_ptr = 0;

	sysinfo_set_item_val("niagara.outbuf.address", NULL,
		KA2PA(&output_buffer));
	sysinfo_set_item_val("niagara.outbuf.size", NULL,
		PAGE_SIZE);
	sysinfo_set_item_val("niagara.outbuf.datasize", NULL,
		OUTPUT_BUFFER_SIZE);

	sysinfo_set_item_val("niagara.inbuf.address", NULL,
		KA2PA(&input_buffer));
	sysinfo_set_item_val("niagara.inbuf.size", NULL,
		PAGE_SIZE);
	sysinfo_set_item_val("niagara.inbuf.datasize", NULL,
		INPUT_BUFFER_SIZE);

	static parea_t outbuf_parea;
	outbuf_parea.pbase = (uintptr_t) (KA2PA(&output_buffer));
	outbuf_parea.frames = 1;
	ddi_parea_register(&outbuf_parea);

	static parea_t inbuf_parea;
	inbuf_parea.pbase = (uintptr_t) (KA2PA(&input_buffer));
	inbuf_parea.frames = 1;
	ddi_parea_register(&inbuf_parea);

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
