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
#define BUF_SIZE		1024

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
ihandle ofw_stdout;
phandle ofw_root;
ihandle ofw_mmu;
phandle ofw_memory;
phandle ofw_aliases;


static int ofw_call(const char *service, const int nargs, const int nret, ofw_arg_t *rets, ...)
{
	va_list list;
	ofw_args_t args;
	int i;
	
	args.service = service;
	args.nargs = nargs;
	args.nret = nret;
	
	va_start(list, rets);
	for (i = 0; i < nargs; i++)
		args.args[i] = va_arg(list, ofw_arg_t);
	va_end(list);
	
	for (i = 0; i < nret; i++)
		args.args[i + nargs] = 0;
	
	ofw(&args);
	
	for (i = 1; i < nret; i++)
		rets[i - 1] = args.args[i + nargs];
	
	return args.args[nargs];
}


static phandle ofw_find_device(const char *name)
{
	return ofw_call("finddevice", 1, 1, NULL, name);
}


static int ofw_get_property(const phandle device, const char *name, const void *buf, const int buflen)
{
	return ofw_call("getprop", 4, 1, NULL, device, name, buf, buflen);
}


static unsigned int ofw_get_address_cells(const phandle device)
{
	unsigned int ret;
	
	if (ofw_get_property(device, "#address-cells", &ret, sizeof(ret)) <= 0)
		if (ofw_get_property(ofw_root, "#address-cells", &ret, sizeof(ret)) <= 0)
			ret = 1;
	
	return ret;
}


static unsigned int ofw_get_size_cells(const phandle device)
{
	unsigned int ret;
	
	if (ofw_get_property(device, "#size-cells", &ret, sizeof(ret)) <= 0)
		if (ofw_get_property(ofw_root, "#size-cells", &ret, sizeof(ret)) <= 0)
			ret = 1;
	
	return ret;
}


static ihandle ofw_open(const char *name)
{
	return ofw_call("open", 1, 1, NULL, name);
}


void init(void)
{
	ofw_chosen = ofw_find_device("/chosen");
	if (ofw_chosen == -1)
		halt();
	
	if (ofw_get_property(ofw_chosen, "stdout",  &ofw_stdout, sizeof(ofw_stdout)) <= 0)
		ofw_stdout = 0;
	
	ofw_root = ofw_find_device("/");
	if (ofw_root == -1) {
		puts("\r\nError: Unable to find / device, halted.\r\n");
		halt();
	}
	
	if (ofw_get_property(ofw_chosen, "mmu",  &ofw_mmu, sizeof(ofw_mmu)) <= 0) {
		puts("\r\nError: Unable to get mmu property, halted.\r\n");
		halt();
	}
	
	ofw_memory = ofw_find_device("/memory");
	if (ofw_memory == -1) {
		puts("\r\nError: Unable to find /memory device, halted.\r\n");
		halt();
	}
	
	ofw_aliases = ofw_find_device("/aliases");
	if (ofw_aliases == -1) {
		puts("\r\nError: Unable to find /aliases device, halted.\r\n");
		halt();
	}
}


void ofw_write(const char *str, const int len)
{
	if (ofw_stdout == 0)
		return;
	
	ofw_call("write", 3, 1, NULL, ofw_stdout, str, len);
}


void *ofw_translate(const void *virt)
{
	ofw_arg_t result[3];
	
	if (ofw_call("call-method", 4, 4, result, "translate", ofw_mmu, virt, 1) != 0) {
		puts("Error: MMU method translate() failed, halting.\n");
		halt();
	}
	return (void *) result[2];
}


int ofw_map(const void *phys, const void *virt, const int size, const int mode)
{
	return ofw_call("call-method", 6, 1, NULL, "map", ofw_mmu, mode, size, virt, phys);
}


int ofw_memmap(memmap_t *map)
{
	unsigned int buf[BUF_SIZE];
	int ret = ofw_get_property(ofw_memory, "reg", buf, sizeof(unsigned int) * BUF_SIZE);
	if (ret <= 0)
		return false;
		
	unsigned int ac = ofw_get_address_cells(ofw_memory);
	unsigned int sc = ofw_get_size_cells(ofw_memory);
	
	int pos;
	map->total = 0;
	map->count = 0;
	for (pos = 0; (pos < ret / sizeof(unsigned int)) && (map->count < MEMMAP_MAX_RECORDS); pos += ac + sc) {
		void * start = (void *) buf[pos + ac - 1];
		unsigned int size = buf[pos + ac + sc - 1];
		
		if (size > 0) {
			map->zones[map->count].start = start;
			map->zones[map->count].size = size;
			map->count++;
			map->total += size;
		}
	}
}


int ofw_screen(screen_t *screen)
{
	char device_name[BUF_SIZE];
	
	if (ofw_get_property(ofw_aliases, "screen", device_name, sizeof(char) * BUF_SIZE) <= 0)
		return false;
	
	phandle device = ofw_find_device(device_name);
	if (device == -1)
		return false;
	
	if (ofw_get_property(device, "address", &screen->addr, sizeof(screen->addr)) <= 0)
		return false;
	
	if (ofw_get_property(device, "width", &screen->width, sizeof(screen->width)) <= 0)
		return false;
	
	if (ofw_get_property(device, "height", &screen->height, sizeof(screen->height)) <= 0)
		return false;
	
	if (ofw_get_property(device, "depth", &screen->bpp, sizeof(screen->bpp)) <= 0)
		return false;
	
	if (ofw_get_property(device, "linebytes", &screen->scanline, sizeof(screen->scanline)) <= 0)
		return false;
	
	return true;
}
