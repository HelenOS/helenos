/*
 * Copyright (C) 2005 Martin Decky
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
 
#include "ofw.h"
#include "asm.h"
#include "printf.h"

#define MAX_OFW_ARGS	10

typedef unsigned int ofw_arg_t;
typedef unsigned int ihandle;
typedef unsigned int phandle;

/** OpenFirmware command structure
 *
 */
typedef struct {
	const char *service;          /**< Command name */
	unsigned int nargs;           /**< Number of in arguments */
	unsigned int nret;            /**< Number of out arguments */
	ofw_arg_t args[MAX_OFW_ARGS]; /**< List of arguments */
} ofw_args_t;

typedef void (*ofw_entry)(ofw_args_t *);


ofw_entry ofw;

phandle ofw_chosen;
ihandle ofw_mmu;
ihandle ofw_stdout;


static int ofw_call(const char *service, const int nargs, const int nret, ...)
{
	va_list list;
	ofw_args_t args;
	int i;
	
	args.service = service;
	args.nargs = nargs;
	args.nret = nret;
	
	va_start(list, nret);
	for (i = 0; i < nargs; i++)
		args.args[i] = va_arg(list, ofw_arg_t);
	va_end(list);
	
	for (i = 0; i < nret; i++)
		args.args[i + nargs] = 0;
	
	ofw(&args);
	
	return args.args[nargs];
}


static phandle ofw_find_device(const char *name)
{
	return ofw_call("finddevice", 1, 1, name);
}


static int ofw_get_property(const phandle device, const char *name, const void *buf, const int buflen)
{
	return ofw_call("getprop", 4, 1, device, name, buf, buflen);
}

static ihandle ofw_open(const char *name)
{
	return ofw_call("open", 1, 1, name);
}


void init(void)
{
	ofw_chosen = ofw_find_device("/chosen");
	if (ofw_chosen == -1)
		halt();
	
	if (ofw_get_property(ofw_chosen, "stdout",  &ofw_stdout, sizeof(ofw_stdout)) <= 0)	
		ofw_stdout = 0;
	
	ofw_mmu = ofw_open("/mmu");
	if (ofw_mmu == -1) {
		puts("Unable to open /mmu node\n");
		halt();
	}
}


void ofw_write(const char *str, const int len)
{
	if (ofw_stdout == 0)
		return;
	
	ofw_call("write", 3, 1, ofw_stdout, str, len);
}


void *ofw_translate(const void *virt)
{
	return (void *) ofw_call("call-method", 7, 1, "translate", ofw_mmu, virt, 0, 0, 0, 0);
}


int ofw_memmap(memmap_t *map)
{
	int i;
	int ret;

	phandle handle = ofw_find_device("/memory");
	if (handle == -1)
		return false;
	
	ret = ofw_get_property(handle, "reg", &map->zones, sizeof(map->zones));
	if (ret == -1)
		return false;
	
	map->total = 0;
	map->count = 0;
	for (i = 0; i < MEMMAP_MAX_RECORDS; i++) {
		if (map->zones[i].size == 0)
			break;
		map->count++;
		map->total += map->zones[i].size;
	}
}
