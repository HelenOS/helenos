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

/**
 * @file
 * @brief	Architecture dependent parts of OpenFirmware interface.
 */

#include <ofwarch.h>  
#include <ofw.h>
#include <printf.h>
#include <string.h>
#include <register.h>
#include "main.h"

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

int ofw_cpu(void)
{
	char type_name[BUF_SIZE];

	phandle node;
	node = ofw_get_child_node(ofw_root);
	if (node == 0 || node == -1) {
		printf("Could not find any child nodes of the root node.\n");
		return 0;
	}
	
	uint64_t current_mid;
	
	__asm__ volatile ("ldxa [%1] %2, %0\n" : "=r" (current_mid) : "r" (0), "i" (ASI_UPA_CONFIG));
	current_mid >>= UPA_CONFIG_MID_SHIFT;
	current_mid &= UPA_CONFIG_MID_MASK;

	int cpus;
	
	for (cpus = 0; node != 0 && node != -1; node = ofw_get_peer_node(node), cpus++) {
		if (ofw_get_property(node, "device_type", type_name, sizeof(type_name)) > 0) {
			if (strcmp(type_name, "cpu") == 0) {
				uint32_t mid;
				
				if (ofw_get_property(node, "upa-portid", &mid, sizeof(mid)) <= 0)
					continue;
					
				if (current_mid != mid) {
					/*
					 * Start secondary processor.
					 */
					(void) ofw_call("SUNW,start-cpu", 3, 1, NULL, node, KERNEL_VIRTUAL_ADDRESS, 0);
				}
			}
		}
	}

	return cpus;
}
