/*
 * Copyright (c) 2005 Martin Decky
 * Copyright (c) 2006 Jakub Jermar
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

/**
 * @file
 * @brief Architecture dependent parts of OpenFirmware interface.
 */

#include <arch/arch.h>
#include <arch/ofw.h>
#include <genarch/ofw.h>
#include <stddef.h>
#include <stdint.h>
#include <printf.h>
#include <halt.h>
#include <putchar.h>
#include <str.h>

void putchar(const wchar_t ch)
{
	if (ch == '\n')
		ofw_putchar('\r');

	if (ascii_check(ch))
		ofw_putchar(ch);
	else
		ofw_putchar(U_SPECIAL);
}

/** Start all CPUs represented by following siblings of the given node.
 *
 * Except for the current CPU.
 *
 * @param child         The first child of the OFW tree node whose children
 *                      represent CPUs to be woken up.
 * @param current_mid   MID of the current CPU, the current CPU will
 *                      (of course) not be woken up.
 * @param physmem_start Starting address of the physical memory.
 *
 * @return Number of CPUs which have the same parent node as
 *         "child".
 *
 */
static size_t wake_cpus_in_node(phandle child, uint64_t current_mid,
    uintptr_t physmem_start)
{
	size_t cpus;

	for (cpus = 0; (child != 0) && (child != (phandle) -1);
	    child = ofw_get_peer_node(child), cpus++) {
		char type_name[OFW_TREE_PROPERTY_MAX_VALUELEN];

		if (ofw_get_property(child, "device_type", type_name,
		    OFW_TREE_PROPERTY_MAX_VALUELEN) > 0) {
			type_name[OFW_TREE_PROPERTY_MAX_VALUELEN - 1] = 0;

			if (str_cmp(type_name, "cpu") == 0) {
				uint32_t mid;

				/*
				 * "upa-portid" for US, "portid" for US-III,
				 * "cpuid" for US-IV
				 */
				if ((ofw_get_property(child, "upa-portid", &mid, sizeof(mid)) <= 0) &&
				    (ofw_get_property(child, "portid", &mid, sizeof(mid)) <= 0) &&
				    (ofw_get_property(child, "cpuid", &mid, sizeof(mid)) <= 0))
					continue;

				if (current_mid != mid) {
					/*
					 * Start secondary processor.
					 */
					(void) ofw_call("SUNW,start-cpu", 3, 1,
					    NULL, child, KERNEL_ADDRESS,
					    physmem_start | AP_PROCESSOR);
				}
			}
		}
	}

	return cpus;
}

/** Find out the current CPU's MID and wake up all AP processors.
 *
 */
void ofw_cpu(uint16_t mid_mask, uintptr_t physmem_start)
{
	/* Get the current CPU MID */
	uint64_t current_mid;

	asm volatile (
	    "ldxa [%[zero]] %[asi], %[current_mid]\n"
	    : [current_mid] "=r" (current_mid)
	    : [zero] "r" (0),
	      [asi] "i" (ASI_ICBUS_CONFIG)
	);

	current_mid >>= ICBUS_CONFIG_MID_SHIFT;
	current_mid &= mid_mask;

	/* Wake up the CPUs */

	phandle cpus_parent = ofw_find_device("/ssm@0,0");
	if ((cpus_parent == 0) || (cpus_parent == (phandle) -1))
		cpus_parent = ofw_find_device("/");

	phandle node = ofw_get_child_node(cpus_parent);
	size_t cpus = wake_cpus_in_node(node, current_mid, physmem_start);

	while ((node != 0) && (node != (phandle) -1)) {
		char name[OFW_TREE_PROPERTY_MAX_VALUELEN];

		if (ofw_get_property(node, "name", name,
		    OFW_TREE_PROPERTY_MAX_VALUELEN) > 0) {
			name[OFW_TREE_PROPERTY_MAX_VALUELEN - 1] = 0;

			if (str_cmp(name, "cmp") == 0) {
				phandle subnode = ofw_get_child_node(node);
				cpus += wake_cpus_in_node(subnode,
				    current_mid, physmem_start);
			}
		}

		node = ofw_get_peer_node(node);
	}

	if (cpus == 0)
		printf("Warning: Unable to get CPU properties.\n");
}

/** Get physical memory starting address.
 *
 * @return Physical memory starting address.
 *
 */
uintptr_t ofw_get_physmem_start(void)
{
	uint32_t memreg[4];
	if ((ofw_ret_t) ofw_get_property(ofw_memory, "reg", &memreg,
	    sizeof(memreg)) <= 0) {
		printf("Error: Unable to get physical memory starting address, halting.\n");
		halt();
	}

	return ((((uintptr_t) memreg[0]) << 32) | memreg[1]);
}
