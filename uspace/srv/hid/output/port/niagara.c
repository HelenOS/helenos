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

/** @file
 */

#include <sys/types.h>
#include <errno.h>
#include <sysinfo.h>
#include <ddi.h>
#include <as.h>
#include <align.h>
#include <str.h>
#include "../ctl/serial.h"
#include "niagara.h"

#define OUTPUT_FIFO_SIZE  ((PAGE_SIZE) - 2 * sizeof(uint64_t))

typedef volatile struct {
	uint64_t read_ptr;
	uint64_t write_ptr;
	char data[OUTPUT_FIFO_SIZE];
} __attribute__((packed)) output_fifo_t;

typedef struct {
	output_fifo_t *fifo;
} niagara_t;

static niagara_t niagara;

static void niagara_putc(const char c)
{
	while (niagara.fifo->write_ptr ==
	    (niagara.fifo->read_ptr + OUTPUT_FIFO_SIZE - 1)
	    % OUTPUT_FIFO_SIZE);
	
	niagara.fifo->data[niagara.fifo->write_ptr] = c;
	niagara.fifo->write_ptr =
	    ((niagara.fifo->write_ptr) + 1) % OUTPUT_FIFO_SIZE;
}

static void niagara_putchar(wchar_t ch)
{
	if (ascii_check(ch))
		niagara_putc(ch);
	else
		niagara_putc('?');
}

static void niagara_control_puts(const char *str)
{
	while (*str)
		niagara_putc(*(str++));
}

int niagara_init(void)
{
	sysarg_t present;
	int rc = sysinfo_get_value("fb", &present);
	if (rc != EOK)
		present = false;
	
	if (!present)
		return ENOENT;
	
	sysarg_t kind;
	rc = sysinfo_get_value("fb.kind", &kind);
	if (rc != EOK)
		kind = (sysarg_t) -1;
	
	if (kind != 5)
		return EINVAL;
	
	sysarg_t paddr;
	rc = sysinfo_get_value("niagara.outbuf.address", &paddr);
	if (rc != EOK)
		return rc;
	
	niagara.fifo = (output_fifo_t *) AS_AREA_ANY;
	
	rc = physmem_map(paddr, 1, AS_AREA_READ | AS_AREA_WRITE,
	    (void *) &niagara.fifo);
	if (rc != EOK)
		return rc;
	
	return serial_init(niagara_putchar, niagara_control_puts);
}

/** @}
 */
