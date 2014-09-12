/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2008 Martin Decky
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
#include <str.h>
#include <sysinfo.h>
#include <ddi.h>
#include <align.h>
#include <as.h>
#include "../ctl/serial.h"
#include "kchar.h"

typedef struct {
	uint8_t *addr;
} kchar_t;

static kchar_t kchar;

static void kchar_putchar(wchar_t ch)
{
	if (ascii_check(ch))
		*kchar.addr = ch;
	else
		*kchar.addr = '?';
}

static void kchar_control_puts(const char *str)
{
	while (*str)
		*kchar.addr = *(str++);
}

int kchar_init(void)
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
	
	if (kind != 3)
		return EINVAL;
	
	sysarg_t paddr;
	rc = sysinfo_get_value("fb.address.physical", &paddr);
	if (rc != EOK)
		return rc;
	
	kchar.addr = AS_AREA_ANY;
	
	rc = physmem_map(paddr,
	    ALIGN_UP(1, PAGE_SIZE) >> PAGE_WIDTH,
	    AS_AREA_READ | AS_AREA_WRITE, (void *) &kchar.addr);
	if (rc != EOK)
		return rc;
	
	return serial_init(kchar_putchar, kchar_control_puts);
}

/** @}
 */
