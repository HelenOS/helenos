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

#include <ofwarch.h>
#include <ofw.h>
#include <printf.h>
#include <string.h>
#include <register.h>
#include "main.h"
#include "asm.h"

void write(const char *str, const int len)
{
	int i;
	
	for (i = 0; i < len; i++) {
		if (str[i] == '\n')
			ofw_write("\r", 1);
		ofw_write(&str[i], 1);
	}
}

int ofw_translate_failed(ofw_arg_t flag)
{
	return flag != -1;
}

/**
 * Starts all CPUs represented by following siblings of the given node,
 * except for the current CPU.
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
static int wake_cpus_in_node(phandle child, uint64_t current_mid,
    uintptr_t physmem_start)
{
	int cpus;
	
	for (cpus = 0; (child != 0) && (child != -1);
	    child = ofw_get_peer_node(child), cpus++) {
		char type_name[BUF_SIZE];
		
		if (ofw_get_property(child, "device_type", type_name,
		    sizeof(type_name)) > 0) {
			if (strcmp(type_name, "cpu") == 0) {
				uint32_t mid;
				
				/*
				 * "upa-portid" for US, "portid" for US-III,
				 * "cpuid" for US-IV
				 */
				if ((ofw_get_property(child, "upa-portid", &mid, sizeof(mid)) <= 0)
				    && (ofw_get_property(child, "portid", &mid, sizeof(mid)) <= 0)
				    && (ofw_get_property(child, "cpuid", &mid, sizeof(mid)) <= 0))
					continue;
				
				if (current_mid != mid) {
					/*
					 * Start secondary processor.
					 */
					(void) ofw_call("SUNW,start-cpu", 3, 1,
					    NULL, child, KERNEL_VIRTUAL_ADDRESS,
					    physmem_start | AP_PROCESSOR);
				}
			}
		}
	}
	
	return cpus;
}

/**
 * Finds out the current CPU's MID and wakes up all AP processors.
 */
int ofw_cpu(uint16_t mid_mask, uintptr_t physmem_start)
{
	/* Get the current CPU MID */
	uint64_t current_mid;
	
	asm volatile (
		"ldxa [%1] %2, %0\n"
		: "=r" (current_mid)
		: "r" (0), "i" (ASI_ICBUS_CONFIG)
	);
	
	current_mid >>= ICBUS_CONFIG_MID_SHIFT;
	current_mid &= mid_mask;
	
	/* Wake up the CPUs */
	
	phandle cpus_parent = ofw_find_device("/ssm@0,0");
	if ((cpus_parent == 0) || (cpus_parent == -1))
		cpus_parent = ofw_find_device("/");
	
	phandle node = ofw_get_child_node(cpus_parent);
	int cpus = wake_cpus_in_node(node, current_mid, physmem_start);
	while ((node != 0) && (node != -1)) {
		char name[BUF_SIZE];
		
		if (ofw_get_property(node, "name", name,
		    sizeof(name)) > 0) {
			if (strcmp(name, "cmp") == 0) {
				phandle subnode = ofw_get_child_node(node);
				cpus += wake_cpus_in_node(subnode,
					current_mid, physmem_start);
			}
		}
		node = ofw_get_peer_node(node);
	}
	
	return cpus;
}

/** Get physical memory starting address.
 *
 * @param start Pointer to variable where the physical memory starting
 *              address will be stored.
 *
 * @return Non-zero on succes, zero on failure.
 *
 */
int ofw_get_physmem_start(uintptr_t *start)
{
	uint32_t memreg[4];
	if (ofw_get_property(ofw_memory, "reg", &memreg, sizeof(memreg)) <= 0)
		return 0;
	
	*start = (((uint64_t) memreg[0]) << 32) | memreg[1];
	return 1;
}
