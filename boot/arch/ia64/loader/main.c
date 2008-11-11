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

extern bootinfo_t binfo;
component_t components[COMPONENTS];

char *release = RELEASE;

void write(const char *str, const int len)
{
    return;
}


#ifdef REVISION
	char *revision = ", revision " REVISION;
#else
	char *revision = "";
#endif

#ifdef TIMESTAMP
	char *timestamp = "\nBuilt on " TIMESTAMP;
#else
	char *timestamp = "";
#endif

/** Print version information. */
static void version_print(void)
{
	printf("HelenOS IA64 Bootloader\nRelease %s%s%s\nCopyright (c) 2006 HelenOS project\n", release, revision, timestamp);
}

void bootstrap(void)
{

	int ii;
	
	
	bootinfo_t *bootinfo=&binfo;
	
	//for(ii=0;ii<KERNEL_SIZE;ii++) ((char *)(0x100000))[ii] = ((char *)KERNEL_START)[ii+1];
	
	
	//((int *)(0x100000))[0]++;
	



	version_print();

	
	init_components(components);

	printf("\nSystem info\n");
	printf("\nMemory statistics\n");
	printf(" %P: boot info structure\n", &bootinfo);
	
	unsigned int i;
	for (i = 0; i < COMPONENTS; i++)
		printf(" %P: %s image (size %d bytes)\n", components[i].start,
		    components[i].name, components[i].size);


	bootinfo->taskmap.count = 0;
	for (i = 0; i < COMPONENTS; i++) {

		if (i > 0) {
			bootinfo->taskmap.tasks[bootinfo->taskmap.count].addr = components[i].start;
			bootinfo->taskmap.tasks[bootinfo->taskmap.count].size = components[i].size;
			bootinfo->taskmap.count++;
		}
	}

	jump_to_kernel(bootinfo);


}

