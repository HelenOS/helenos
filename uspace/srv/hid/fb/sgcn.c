/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2008 Martin Decky
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

/** @defgroup sgcnfb SGCN
 * @brief	userland driver of the Serengeti console output
 * @{
 */ 
/** @file
 */

#include <async.h>
#include <sysinfo.h>
#include <as.h>
#include <errno.h>
#include <stdio.h>
#include <ddi.h>

#include "serial_console.h"
#include "sgcn.h"

#define WIDTH 80
#define HEIGHT 24

/**
 * Virtual address mapped to SRAM.
 */
static uintptr_t sram_virt_addr;

/**
 * SGCN buffer offset within SGCN.
 */
static uintptr_t sram_buffer_offset;

/**
 * SGCN buffer header. It is placed at the very beginning of the SGCN
 * buffer. 
 */
typedef struct {
	/** hard-wired to "CON" */
	char magic[4];
	
	/** we don't need this */
	char unused[24];

	/** offset within the SGCN buffer of the output buffer start */
	uint32_t out_begin;
	
	/** offset within the SGCN buffer of the output buffer end */
	uint32_t out_end;
	
	/** offset within the SGCN buffer of the output buffer read pointer */
	uint32_t out_rdptr;
	
	/** offset within the SGCN buffer of the output buffer write pointer */
	uint32_t out_wrptr;
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
 * Pushes the character to the SGCN serial.
 * @param c	character to be pushed
 */
static void sgcn_putc(char c)
{
	uint32_t begin = SGCN_BUFFER_HEADER->out_begin;
	uint32_t end = SGCN_BUFFER_HEADER->out_end;
	uint32_t size = end - begin;
	
	/* we need pointers to volatile variables */
	volatile char *buf_ptr = (volatile char *)
		SGCN_BUFFER(char, SGCN_BUFFER_HEADER->out_wrptr);
	volatile uint32_t *out_wrptr_ptr = &(SGCN_BUFFER_HEADER->out_wrptr);
	volatile uint32_t *out_rdptr_ptr = &(SGCN_BUFFER_HEADER->out_rdptr);

	uint32_t new_wrptr = (((*out_wrptr_ptr) - begin + 1) % size) + begin;
	while (*out_rdptr_ptr == new_wrptr)
		;
	*buf_ptr = c;
	*out_wrptr_ptr = new_wrptr;
}

/**
 * Initializes the SGCN serial driver.
 */
int sgcn_init(void)
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
	    sram_size / PAGE_SIZE, AS_AREA_READ | AS_AREA_WRITE) != 0)
		return -1;
	
	serial_console_init(sgcn_putc, WIDTH, HEIGHT);
	
	async_set_client_connection(serial_client_connection);
	return 0;
}

/** 
 * @}
 */
 
