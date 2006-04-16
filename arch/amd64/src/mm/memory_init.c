/*
 * Copyright (C) 2005 Josef Cejka
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

#include <arch/boot/memmap.h>
#include <arch/mm/memory_init.h>
#include <arch/mm/page.h>
#include <print.h>

__u8 e820counter = 0xff;
struct e820memmap_ e820table[MEMMAP_E820_MAX_RECORDS];
__u32 e801memorysize;

size_t get_memory_size(void) 
{
	return e801memorysize*1024;
}

void memory_print_map(void)
{
	__u8 i;
	
	for (i=0;i<e820counter;i++) {
		printf("E820 base: %#llX size: %#llX type: ", e820table[i].base_address, e820table[i].size);
		switch (e820table[i].type) {
			case MEMMAP_MEMORY_AVAILABLE: 
				printf("available memory\n");
				break;
			case MEMMAP_MEMORY_RESERVED: 
				printf("reserved memory\n");
				break;
			case MEMMAP_MEMORY_ACPI: 
				printf("ACPI table\n");
				break;
			case MEMMAP_MEMORY_NVS: 
				printf("NVS\n");
				break;
			case MEMMAP_MEMORY_UNUSABLE: 
				printf("unusable memory\n");
				break;
			default:
				printf("undefined memory type\n");
		}
	}

}

