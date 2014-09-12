/*
 * Copyright (c) 2008 Pavel Rimsky
 * Copyright (c) 2011 Jiri Svoboda
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

/** @addtogroup kbd_port
 * @ingroup  kbd
 * @{
 */
/** @file
 * @brief Niagara console keyboard port driver.
 */

#include <as.h>
#include <ddi.h>
#include <async.h>
#include <sysinfo.h>
#include <stdio.h>
#include <thread.h>
#include <stdbool.h>
#include <errno.h>
#include "../kbd_port.h"
#include "../kbd.h"

static int niagara_port_init(kbd_dev_t *);
static void niagara_port_write(uint8_t data);

kbd_port_ops_t niagara_port = {
	.init = niagara_port_init,
	.write = niagara_port_write
};

static kbd_dev_t *kbd_dev;

#define POLL_INTERVAL  10000

/*
 * Kernel counterpart of the driver pushes characters (it has read) here.
 * Keep in sync with the definition from
 * kernel/arch/sparc64/src/drivers/niagara.c.
 */
#define INPUT_BUFFER_SIZE  ((PAGE_SIZE) - 2 * 8)

typedef volatile struct {
	uint64_t write_ptr;
	uint64_t read_ptr;
	char data[INPUT_BUFFER_SIZE];
} __attribute__((packed)) __attribute__((aligned(PAGE_SIZE))) *input_buffer_t;

/* virtual address of the shared buffer */
static input_buffer_t input_buffer = (input_buffer_t) AS_AREA_ANY;

static void niagara_thread_impl(void *arg);

/**
 * Initializes the Niagara driver.
 * Maps the shared buffer and creates the polling thread.
 */
static int niagara_port_init(kbd_dev_t *kdev)
{
	kbd_dev = kdev;
	
	sysarg_t paddr;
	if (sysinfo_get_value("niagara.inbuf.address", &paddr) != EOK)
		return -1;
	
	int rc = physmem_map(paddr, 1, AS_AREA_READ | AS_AREA_WRITE,
	    (void *) &input_buffer);
	if (rc != 0) {
		printf("Niagara: uspace driver couldn't map physical memory: %d\n",
		    rc);
		return rc;
	}
	
	thread_id_t tid;
	rc = thread_create(niagara_thread_impl, NULL, "kbd_poll", &tid);
	if (rc != 0)
		return rc;
	
	return 0;
}

static void niagara_port_write(uint8_t data)
{
	(void) data;
}

/**
 * Called regularly by the polling thread. Reads codes of all the
 * pressed keys from the buffer.
 */
static void niagara_key_pressed(void)
{
	char c;
	
	while (input_buffer->read_ptr != input_buffer->write_ptr) {
		c = input_buffer->data[input_buffer->read_ptr];
		input_buffer->read_ptr =
		    ((input_buffer->read_ptr) + 1) % INPUT_BUFFER_SIZE;
		kbd_push_data(kbd_dev, c);
	}
}

/**
 * Thread to poll Niagara console for keypresses.
 */
static void niagara_thread_impl(void *arg)
{
	(void) arg;

	while (1) {
		niagara_key_pressed();
		usleep(POLL_INTERVAL);
	}
}
/** @}
 */
