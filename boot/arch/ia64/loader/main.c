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

#include "main.h" 
#include <printf.h>
#include "asm.h"
#include "_components.h"
#include <align.h>
#include <balloc.h>
#include <macros.h>

extern bootinfo_t binfo;
component_t components[COMPONENTS];

char *release = STRING(RELEASE);

void write(const char *str, const int len)
{
    return;
}

#define DEFAULT_MEMORY_BASE		0x4000000
#define DEFAULT_MEMORY_SIZE		0x4000000
#define DEFAULT_LEGACY_IO_BASE		0x00000FFFFC000000 
#define DEFAULT_LEGACY_IO_SIZE		0x4000000 

#define DEFAULT_FREQ_SCALE		0x0000000100000001 /* 1/1 */
#define DEFAULT_SYS_FREQ		100000000 /* 100MHz */

#ifdef REVISION
	char *revision = ", revision " STRING(REVISION);
#else
	char *revision = "";
#endif

#ifdef TIMESTAMP
	char *timestamp = "\nBuilt on " STRING(TIMESTAMP);
#else
	char *timestamp = "";
#endif

/** Print version information. */
static void version_print(void)
{
	printf("HelenOS IA64 Bootloader\nRelease %s%s%s\n"
	    "Copyright (c) 2006 HelenOS project\n", release, revision,
	    timestamp);
}

void bootstrap(void)
{
	int ii;
	bootinfo_t *bootinfo = &binfo;

	version_print();

	init_components(components);

	printf("\nSystem info\n");
	printf("\nMemory statistics\n");
	printf(" %P: boot info structure\n", &bootinfo);
	
	unsigned int i;
	for (i = 0; i < COMPONENTS; i++)
		printf(" %P: %s image (size %d bytes)\n", components[i].start,
		    components[i].name, components[i].size);

	if (!bootinfo->hello_configured) {
		/*
		 * Load configuration defaults for simulators.
		 */
		 bootinfo->memmap_items = 0;
		 
		 bootinfo->memmap[bootinfo->memmap_items].base =
		     DEFAULT_MEMORY_BASE;
		 bootinfo->memmap[bootinfo->memmap_items].size =
		     DEFAULT_MEMORY_SIZE;
		 bootinfo->memmap[bootinfo->memmap_items].type =
		     EFI_MEMMAP_FREE_MEM;
		 bootinfo->memmap_items++;

		 bootinfo->memmap[bootinfo->memmap_items].base =
		     DEFAULT_LEGACY_IO_BASE;
		 bootinfo->memmap[bootinfo->memmap_items].size =
		     DEFAULT_LEGACY_IO_SIZE;
		 bootinfo->memmap[bootinfo->memmap_items].type =
		     EFI_MEMMAP_IO_PORTS;
		 bootinfo->memmap_items++;
		 
		 bootinfo->freq_scale = DEFAULT_FREQ_SCALE;
		 bootinfo->sys_freq = DEFAULT_SYS_FREQ;
	}

	bootinfo->taskmap.count = 0;
	for (i = 0; i < COMPONENTS; i++) {
		if (i > 0) {
			bootinfo->taskmap.tasks[bootinfo->taskmap.count].addr =
			    components[i].start;
			bootinfo->taskmap.tasks[bootinfo->taskmap.count].size =
			    components[i].size;
			bootinfo->taskmap.count++;
		}
	}

	jump_to_kernel(bootinfo);
}

