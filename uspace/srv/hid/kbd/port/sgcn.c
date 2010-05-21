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

/** @addtogroup kbd_port
 * @ingroup  kbd
 * @{
 */
/** @file
 * @brief SGCN (Serengeti Console) keyboard port driver.
 */

#include <as.h>
#include <ddi.h>
#include <async.h>
#include <kbd.h>
#include <kbd_port.h>
#include <sysinfo.h>
#include <stdio.h>
#include <thread.h>
#include <bool.h>
#include <errno.h>

#define POLL_INTERVAL  10000

/**
 * SGCN buffer header. It is placed at the very beginning of the SGCN
 * buffer.
 */
typedef struct {
	/** hard-wired to "CON" */
	char magic[4];
	
	/** we don't need this */
	char unused[8];
	
	/** offset within the SGCN buffer of the input buffer start */
	uint32_t in_begin;
	
	/** offset within the SGCN buffer of the input buffer end */
	uint32_t in_end;
	
	/** offset within the SGCN buffer of the input buffer read pointer */
	uint32_t in_rdptr;
	
	/** offset within the SGCN buffer of the input buffer write pointer */
	uint32_t in_wrptr;
} __attribute__ ((packed)) sgcn_buffer_header_t;

/*
 * Returns a pointer to the object of a given type which is placed at the given
 * offset from the console buffer beginning.
 */
#define SGCN_BUFFER(type, offset) \
		((type *) (sram_virt_addr + sram_buffer_offset + (offset)))

/** Returns a pointer to the console buffer header. */
#define SGCN_BUFFER_HEADER	(SGCN_BUFFER(sgcn_buffer_header_t, 0))

/**
 * Virtual address mapped to SRAM.
 */
static uintptr_t sram_virt_addr;

/**
 * SGCN buffer offset within SGCN.
 */
static uintptr_t sram_buffer_offset;

/* polling thread */
static void sgcn_thread_impl(void *arg);

static volatile bool polling_disabled = false;

/**
 * Initializes the SGCN driver.
 * Maps the physical memory (SRAM) and creates the polling thread. 
 */
int kbd_port_init(void)
{
	sysarg_t sram_paddr;
	if (sysinfo_get_value("sram.address.physical", &sram_paddr) != EOK)
		return -1;
	
	sysarg_t sram_size;
	if (sysinfo_get_value("sram.area.size", &sram_size) != EOK)
		return -1;
	
	if (sysinfo_get_value("sram.buffer.offset", &sram_buffer_offset) != EOK)
		sram_buffer_offset = 0;
	
	sram_virt_addr = (uintptr_t) as_get_mappable_page(sram_size);
	
	if (physmem_map((void *) sram_paddr, (void *) sram_virt_addr,
	    sram_size / PAGE_SIZE, AS_AREA_READ | AS_AREA_WRITE) != 0) {
		printf("SGCN: uspace driver could not map physical memory.");
		return -1;
	}
	
	thread_id_t tid;
	int rc = thread_create(sgcn_thread_impl, NULL, "kbd_poll", &tid);
	if (rc != 0)
		return rc;
	
	return 0;
}

void kbd_port_yield(void)
{
	polling_disabled = true;
}

void kbd_port_reclaim(void)
{
	polling_disabled = false;
}

void kbd_port_write(uint8_t data)
{
	(void) data;
}

/**
 * Handler of the "key pressed" event. Reads codes of all the pressed keys from
 * the buffer. 
 */
static void sgcn_key_pressed(void)
{
	char c;
	
	uint32_t begin = SGCN_BUFFER_HEADER->in_begin;
	uint32_t end = SGCN_BUFFER_HEADER->in_end;
	uint32_t size = end - begin;
	
	volatile char *buf_ptr = (volatile char *)
		SGCN_BUFFER(char, SGCN_BUFFER_HEADER->in_rdptr);
	volatile uint32_t *in_wrptr_ptr = &(SGCN_BUFFER_HEADER->in_wrptr);
	volatile uint32_t *in_rdptr_ptr = &(SGCN_BUFFER_HEADER->in_rdptr);
	
	while (*in_rdptr_ptr != *in_wrptr_ptr) {
		c = *buf_ptr;
		*in_rdptr_ptr = (((*in_rdptr_ptr) - begin + 1) % size) + begin;
		buf_ptr = (volatile char *)
			SGCN_BUFFER(char, SGCN_BUFFER_HEADER->in_rdptr);
		kbd_push_scancode(c);
	}
}

/**
 * Thread to poll SGCN for keypresses.
 */
static void sgcn_thread_impl(void *arg)
{
	(void) arg;

	while (1) {
		if (polling_disabled == false)
			sgcn_key_pressed();
		usleep(POLL_INTERVAL);
	}
}

/** @}
 */
