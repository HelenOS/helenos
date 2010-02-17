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

/** @defgroup niagarafb SGCN
 * @brief	userland driver of the Niagara console output
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
#include "niagara.h"

#define WIDTH 80
#define HEIGHT 24

/**
 * Virtual address mapped to the buffer shared with the kernel counterpart.
 */
static uintptr_t output_buffer_addr;

/*
 * Kernel counterpart of the driver reads characters to be printed from here.
 * Keep in sync with the definition from
 * kernel/arch/sparc64/src/drivers/niagara.c.
 */
#define OUTPUT_BUFFER_SIZE	((PAGE_SIZE) - 2 * 8)
typedef volatile struct {
	uint64_t read_ptr;
	uint64_t write_ptr;
	char data[OUTPUT_BUFFER_SIZE];
}
	__attribute__ ((packed))
	__attribute__ ((aligned(PAGE_SIZE)))
	*output_buffer_t;

output_buffer_t output_buffer;

/**
 * Pushes the character to the Niagara serial.
 * @param c	character to be pushed
 */
static void niagara_putc(char c)
{
	while (output_buffer->write_ptr ==
	       (output_buffer->read_ptr + OUTPUT_BUFFER_SIZE - 1)
	       % OUTPUT_BUFFER_SIZE)
		;
	output_buffer->data[output_buffer->write_ptr] = (uint64_t) c;
	output_buffer->write_ptr =
		((output_buffer->write_ptr) + 1) % OUTPUT_BUFFER_SIZE;
}

/**
 * Initializes the Niagara serial driver.
 */
int niagara_init(void)
{
	output_buffer_addr = (uintptr_t) as_get_mappable_page(PAGE_SIZE);
	int result = physmem_map(
		(void *) sysinfo_value("niagara.outbuf.address"),
		(void *) output_buffer_addr,
		1, AS_AREA_READ | AS_AREA_WRITE);

	if (result != 0) {
		printf("Niagara: uspace driver couldn't map physical memory: %d\n",
			result);
	}

	output_buffer = (output_buffer_t) output_buffer_addr;

	serial_console_init(niagara_putc, WIDTH, HEIGHT);
	async_set_client_connection(serial_client_connection);
	return 0;
}

/** 
 * @}
 */
 
