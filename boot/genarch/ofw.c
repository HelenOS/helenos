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
 
#include <ofw.h>
#include <ofwarch.h>
#include <printf.h>
#include <asm.h>
#include <types.h>

uintptr_t ofw_cif;

phandle ofw_chosen;
ihandle ofw_stdout;
phandle ofw_root;
ihandle ofw_mmu;
phandle ofw_memory;
phandle ofw_aliases;

void ofw_init(void)
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

/** Perform a call to OpenFirmware client interface.
 *
 * @param service String identifying the service requested.
 * @param nargs Number of input arguments.
 * @param nret Number of output arguments. This includes the return value.
 * @param rets Buffer for output arguments or NULL. The buffer must accommodate nret - 1 items.
 *
 * @return Return value returned by the client interface.
 */
static unsigned long ofw_call(const char *service, const int nargs, const int nret, ofw_arg_t *rets, ...)
{
	va_list list;
	ofw_args_t args;
	int i;
	
	args.service = (ofw_arg_t) service;
	args.nargs = nargs;
	args.nret = nret;
	
	va_start(list, rets);
	for (i = 0; i < nargs; i++)
		args.args[i] = va_arg(list, ofw_arg_t);
	va_end(list);
	
	for (i = 0; i < nret; i++)
		args.args[i + nargs] = 0;
	
	(void) ofw(&args);

	for (i = 1; i < nret; i++)
		rets[i - 1] = args.args[i + nargs];

	return args.args[nargs];
}

phandle ofw_find_device(const char *name)
{
	return ofw_call("finddevice", 1, 1, NULL, name);
}

int ofw_get_property(const phandle device, const char *name, const void *buf, const int buflen)
{
	return ofw_call("getprop", 4, 1, NULL, device, name, buf, buflen);
}

int ofw_get_proplen(const phandle device, const char *name)
{
	return ofw_call("getproplen", 2, 1, NULL, device, name);
}

int ofw_next_property(const phandle device, char *previous, char *buf)
{
	return ofw_call("nextprop", 3, 1, NULL, device, previous, buf);
}

unsigned int ofw_get_address_cells(const phandle device)
{
	unsigned int ret = 1;
	
	if (ofw_get_property(device, "#address-cells", &ret, sizeof(ret)) <= 0)
		if (ofw_get_property(ofw_root, "#address-cells", &ret, sizeof(ret)) <= 0)
			ret = OFW_ADDRESS_CELLS;
	
	return ret;
}


unsigned int ofw_get_size_cells(const phandle device)
{
	unsigned int ret;
	
	if (ofw_get_property(device, "#size-cells", &ret, sizeof(ret)) <= 0)
		if (ofw_get_property(ofw_root, "#size-cells", &ret, sizeof(ret)) <= 0)
			ret = OFW_SIZE_CELLS;
	
	return ret;
}

phandle ofw_get_child_node(const phandle node)
{
	return ofw_call("child", 1, 1, NULL, node);
}

phandle ofw_get_peer_node(const phandle node)
{
	return ofw_call("peer", 1, 1, NULL, node);
}

static ihandle ofw_open(const char *name)
{
	return ofw_call("open", 1, 1, NULL, name);
}


void ofw_write(const char *str, const int len)
{
	if (ofw_stdout == 0)
		return;
	
	ofw_call("write", 3, 1, NULL, ofw_stdout, str, len);
}


void *ofw_translate(const void *virt)
{
	ofw_arg_t result[4];
	int shift;

	if (ofw_call("call-method", 3, 5, result, "translate", ofw_mmu, virt) != 0) {
		puts("Error: MMU method translate() failed, halting.\n");
		halt();
	}

	if (ofw_translate_failed(result[0]))
		return NULL;

	if (sizeof(unative_t) == 8)
		shift = 32;
	else
		shift = 0;

	return (void *) ((result[2]<<shift)|result[3]);
}

void *ofw_claim(const void *virt, const int len)
{
	ofw_arg_t retaddr;
	int shift;

	if (ofw_call("call-method", 5, 2, &retaddr, "claim", ofw_mmu, 0, len, virt) != 0) {
		puts("Error: MMU method claim() failed, halting.\n");
		halt();
	}

	return (void *) retaddr;
}

int ofw_map(const void *phys, const void *virt, const int size, const int mode)
{
	uintptr_t phys_hi, phys_lo;

	if (sizeof(unative_t) == 8) {
		int shift = 32;
		phys_hi = (uintptr_t) phys >> shift;
		phys_lo = (uintptr_t) phys & 0xffffffff;
	} else {
		phys_hi = 0;
		phys_lo = (uintptr_t) phys;
	}

	return ofw_call("call-method", 7, 1, NULL, "map", ofw_mmu, mode, size, virt,
		phys_hi, phys_lo);
}


int ofw_memmap(memmap_t *map)
{
	unsigned int ac = ofw_get_address_cells(ofw_memory);
	unsigned int sc = ofw_get_size_cells(ofw_memory);

	uint32_t buf[((ac+sc)*MEMMAP_MAX_RECORDS)];
	int ret = ofw_get_property(ofw_memory, "reg", buf, sizeof(buf));
	if (ret <= 0)		/* ret is the number of written bytes */
		return false;

	int pos;
	map->total = 0;
	map->count = 0;
	for (pos = 0; (pos < ret / sizeof(uint32_t)) && (map->count < MEMMAP_MAX_RECORDS); pos += ac + sc) {
		void * start = (void *) ((uintptr_t) buf[pos + ac - 1]);
		unsigned int size = buf[pos + ac + sc - 1];
		
		if (size > 0) {
			map->zones[map->count].start = start;
			map->zones[map->count].size = size;
			map->count++;
			map->total += size;
		}
	}
	
	return true;
}


int ofw_screen(screen_t *screen)
{
	char device_name[BUF_SIZE];
	uint32_t virtaddr;
	
	if (ofw_get_property(ofw_aliases, "screen", device_name, sizeof(device_name)) <= 0)
		return false;
	
	phandle device = ofw_find_device(device_name);
	if (device == -1)
		return false;
	
	if (ofw_get_property(device, "address", &virtaddr, sizeof(virtaddr)) <= 0)
		return false;

	screen->addr = (void *) ((uintptr_t) virtaddr);

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
