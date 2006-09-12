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
#include "main.h"

int bpp2align[] = {
	[0] = 0,		/** Invalid bpp. */
	[1] = 1,		/** 8bpp is not aligned. */
	[2] = 2,		/** 16bpp is naturally aligned. */
	[3] = 4,		/** 24bpp is aligned on 4 byte boundary. */
	[4] = 4,		/** 32bpp is naturally aligned. */
};

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

int ofw_keyboard(keyboard_t *keyboard)
{
	char device_name[BUF_SIZE];
	uint32_t virtaddr;
		
	if (ofw_get_property(ofw_aliases, "keyboard", device_name, sizeof(device_name)) <= 0)
		return false;
					
	phandle device = ofw_find_device(device_name);
	if (device == -1)
		return false;
									
	if (ofw_get_property(device, "address", &virtaddr, sizeof(virtaddr)) <= 0)
		return false;
												
	if (!(keyboard->addr = ofw_translate((void *) ((uintptr_t) virtaddr))))
		return false;

	return true;
}

int ofw_cpu(cpu_t *cpu)
{
	char type_name[BUF_SIZE];

	phandle node;
	node = ofw_get_child_node(ofw_root);
	if (node == 0 || node == -1) {
		printf("Could not find any child nodes of the root node.\n");
		return;
	}
	
	for (; node != 0 && node != -1; node = ofw_get_peer_node(node)) {
		if (ofw_get_property(node, "device_type", type_name, sizeof(type_name)) > 0) {
			if (strncmp(type_name, "cpu", 3) == 0) {
				uint32_t mhz;
				
				if (ofw_get_property(node, "clock-frequency", &mhz, sizeof(mhz)) <= 0)
					continue;
					
				cpu->clock_frequency = mhz;
				return 1;
			}
		}
	};

	return 0;
}
