/*
 * Copyright (c) 2005 Martin Decky
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
#include <string.h>

#define RED(i)    (((i) >> 5) & ((1 << 3) - 1))
#define GREEN(i)  (((i) >> 3) & ((1 << 2) - 1))
#define BLUE(i)   ((i) & ((1 << 3) - 1))
#define CLIP(i)   ((i) <= 255 ? (i) : 255)

uintptr_t ofw_cif;

phandle ofw_chosen;
ihandle ofw_stdout;
phandle ofw_root;
ihandle ofw_mmu;
ihandle ofw_memory_prop;
phandle ofw_memory;

void ofw_init(void)
{
	ofw_chosen = ofw_find_device("/chosen");
	if (ofw_chosen == -1)
		halt();
	
	if (ofw_get_property(ofw_chosen, "stdout", &ofw_stdout,
	    sizeof(ofw_stdout)) <= 0)
		ofw_stdout = 0;
	
	ofw_root = ofw_find_device("/");
	if (ofw_root == -1) {
		puts("\r\nError: Unable to find / device, halted.\r\n");
		halt();
	}
	
	if (ofw_get_property(ofw_chosen, "mmu", &ofw_mmu,
	    sizeof(ofw_mmu)) <= 0) {
		puts("\r\nError: Unable to get mmu property, halted.\r\n");
		halt();
	}
	if (ofw_get_property(ofw_chosen, "memory", &ofw_memory_prop,
	    sizeof(ofw_memory_prop)) <= 0) {
		puts("\r\nError: Unable to get memory property, halted.\r\n");
		halt();
	}
	
	ofw_memory = ofw_find_device("/memory");
	if (ofw_memory == -1) {
		puts("\r\nError: Unable to find /memory device, halted.\r\n");
		halt();
	}
}

/** Perform a call to OpenFirmware client interface.
 *
 * @param service String identifying the service requested.
 * @param nargs   Number of input arguments.
 * @param nret    Number of output arguments. This includes the return
 *                value.
 * @param rets    Buffer for output arguments or NULL. The buffer must
 *                accommodate nret - 1 items.
 *
 * @return Return value returned by the client interface.
 *
 */
unsigned long
ofw_call(const char *service, const int nargs, const int nret, ofw_arg_t *rets,
    ...)
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

int ofw_get_property(const phandle device, const char *name, void *buf,
    const int buflen)
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

int ofw_package_to_path(const phandle device, char *buf, const int buflen)
{
	return ofw_call("package-to-path", 3, 1, NULL, device, buf, buflen);
}

unsigned int ofw_get_address_cells(const phandle device)
{
	unsigned int ret = 1;
	
	if (ofw_get_property(device, "#address-cells", &ret, sizeof(ret)) <= 0)
		if (ofw_get_property(ofw_root, "#address-cells", &ret,
		    sizeof(ret)) <= 0)
			ret = OFW_ADDRESS_CELLS;
	
	return ret;
}

unsigned int ofw_get_size_cells(const phandle device)
{
	unsigned int ret;
	
	if (ofw_get_property(device, "#size-cells", &ret, sizeof(ret)) <= 0)
		if (ofw_get_property(ofw_root, "#size-cells", &ret,
		    sizeof(ret)) <= 0)
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

	if (ofw_call("call-method", 4, 5, result, "translate", ofw_mmu,
	    virt, 0) != 0) {
		puts("Error: MMU method translate() failed, halting.\n");
		halt();
	}

	if (ofw_translate_failed(result[0]))
		return NULL;

	if (sizeof(unative_t) == 8)
		shift = 32;
	else
		shift = 0;

	return (void *) ((result[2] << shift) | result[3]);
}

void *ofw_claim_virt(const void *virt, const unsigned int len)
{
	ofw_arg_t retaddr;

	if (ofw_call("call-method", 5, 2, &retaddr, "claim", ofw_mmu, 0, len,
	    virt) != 0) {
		puts("Error: MMU method claim() failed, halting.\n");
		halt();
	}

	return (void *) retaddr;
}

static void *ofw_claim_phys_internal(const void *phys, const unsigned int len, const unsigned int alignment)
{
	/*
	 * Note that the return value check will help
	 * us to discover conflicts between OpenFirmware
	 * allocations and our use of physical memory.
	 * It is better to detect collisions here
	 * than to cope with weird errors later.
	 *
	 * So this is really not to make the loader
	 * more generic; it is here for debugging
	 * purposes.
	 */
	
	if (sizeof(unative_t) == 8) {
		ofw_arg_t retaddr[2];
		int shift = 32;
		
		if (ofw_call("call-method", 6, 3, retaddr, "claim",
		    ofw_memory_prop, alignment, len, ((uintptr_t) phys) >> shift,
		    ((uintptr_t) phys) & ((uint32_t) -1)) != 0) {
			puts("Error: memory method claim() failed, halting.\n");
			halt();
		}
		
		return (void *) ((retaddr[0] << shift) | retaddr[1]);
	} else {
		ofw_arg_t retaddr[1];
		
		if (ofw_call("call-method", 5, 2, retaddr, "claim",
		    ofw_memory_prop, alignment, len, (uintptr_t) phys) != 0) {
			puts("Error: memory method claim() failed, halting.\n");
			halt();
		}
		
		return (void *) retaddr[0];
	}
}

void *ofw_claim_phys(const void *phys, const unsigned int len)
{
	return ofw_claim_phys_internal(phys, len, 0);
}

void *ofw_claim_phys_any(const unsigned int len, const unsigned int alignment)
{
	return ofw_claim_phys_internal(NULL, len, alignment);
}

int ofw_map(const void *phys, const void *virt, const unsigned int size, const int mode)
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

	return ofw_call("call-method", 7, 1, NULL, "map", ofw_mmu, mode, size,
	    virt, phys_hi, phys_lo);
}

/** Save OpenFirmware physical memory map.
 *
 * @param map Memory map structure where the map will be saved.
 *
 * @return Zero on failure, non-zero on success.
 */
int ofw_memmap(memmap_t *map)
{
	unsigned int ac = ofw_get_address_cells(ofw_memory) /
	    (sizeof(uintptr_t) / sizeof(uint32_t));
	unsigned int sc = ofw_get_size_cells(ofw_memory) /
	    (sizeof(uintptr_t) / sizeof(uint32_t));

	uintptr_t buf[((ac + sc) * MEMMAP_MAX_RECORDS)];
	int ret = ofw_get_property(ofw_memory, "reg", buf, sizeof(buf));
	if (ret <= 0)		/* ret is the number of written bytes */
		return false;

	int pos;
	map->total = 0;
	map->count = 0;
	for (pos = 0; (pos < ret / sizeof(uintptr_t)) &&
	    (map->count < MEMMAP_MAX_RECORDS); pos += ac + sc) {
		void *start = (void *) (buf[pos + ac - 1]);
		unsigned int size = buf[pos + ac + sc - 1];

		/*
		 * This is a hot fix of the issue which occurs on machines
		 * where there are holes in the physical memory (such as
		 * SunBlade 1500). Should we detect a hole in the physical
		 * memory, we will ignore any memory detected behind
		 * the hole and pretend the hole does not exist.
		 */
		if ((map->count > 0) && (map->zones[map->count - 1].start +
		    map->zones[map->count - 1].size < start))
			break;

		if (size > 0) {
			map->zones[map->count].start = start;
			map->zones[map->count].size = size;
			map->count++;
			map->total += size;
		}
	}
	
	return true;
}

static void ofw_setup_screen(phandle handle)
{
	/* Check for device type */
	char device_type[OFW_TREE_PROPERTY_MAX_VALUELEN];
	if (ofw_get_property(handle, "device_type", device_type, OFW_TREE_PROPERTY_MAX_VALUELEN) <= 0)
		return;
	
	device_type[OFW_TREE_PROPERTY_MAX_VALUELEN - 1] = '\0';
	if (strcmp(device_type, "display") != 0)
		return;
	
	/* Check for 8 bit depth */
	uint32_t depth;
	if (ofw_get_property(handle, "depth", &depth, sizeof(uint32_t)) <= 0)
		depth = 0;
	
	/* Get device path */
	static char path[OFW_TREE_PATH_MAX_LEN + 1];
	size_t len = ofw_package_to_path(handle, path, OFW_TREE_PATH_MAX_LEN);
	if (len == -1)
		return;
	
	path[len] = '\0';
	
	/* Open the display to initialize it */
	ihandle screen = ofw_open(path);
	if (screen == -1)
		return;
	
	if (depth == 8) {
		/* Setup the palette so that the (inverted) 3:2:3 scheme is usable */
		unsigned int i;
		for (i = 0; i < 256; i++) {
			ofw_call("call-method", 6, 1, NULL, "color!", screen,
			    255 - i, CLIP(BLUE(i) * 37), GREEN(i) * 85, CLIP(RED(i) * 37));
		}
	}
}

static void ofw_setup_screens_internal(phandle current)
{
	while ((current != 0) && (current != -1)) {
		ofw_setup_screen(current);
		
		/*
		 * Recursively process the potential child node.
		 */
		phandle child = ofw_get_child_node(current);
		if ((child != 0) && (child != -1))
			ofw_setup_screens_internal(child);
		
		/*
		 * Iteratively process the next peer node.
		 * Note that recursion is a bad idea here.
		 * Due to the topology of the OpenFirmware device tree,
		 * the nesting of peer nodes could be to wide and the
		 * risk of overflowing the stack is too real.
		 */
		phandle peer = ofw_get_peer_node(current);
		if ((peer != 0) && (peer != -1)) {
			current = peer;
			/*
			 * Process the peer in next iteration.
			 */
			continue;
		}
		
		/*
		 * No more peers on this level.
		 */
		break;
	}
}

/** Setup all screens which can be detected.
 *
 * Open all screens which can be detected and set up the palette for the 8-bit
 * color depth configuration so that the 3:2:3 color scheme can be used.
 * Check that setting the palette makes sense (the color depth is not greater
 * than 8).
 *
 */
void ofw_setup_screens(void)
{
	ofw_setup_screens_internal(ofw_root);
}

void ofw_quiesce(void)
{
	ofw_call("quiesce", 0, 0, NULL);
}
