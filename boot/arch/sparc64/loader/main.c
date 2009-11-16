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
#include <balloc.h>
#include <ofw.h>
#include <ofw_tree.h>
#include "ofwarch.h"
#include <align.h>
#include <macros.h>
#include <string.h>
#include <memstr.h>

static bootinfo_t bootinfo;
static component_t components[COMPONENTS];
static char *release = STRING(RELEASE);

#ifdef REVISION
	static char *revision = ", revision " STRING(REVISION);
#else
	static char *revision = "";
#endif

#ifdef TIMESTAMP
	static char *timestamp = "\nBuilt on " STRING(TIMESTAMP);
#else
	static char *timestamp = "";
#endif

/** UltraSPARC subarchitecture - 1 for US, 3 for US3 */
static uint8_t subarchitecture;

/**
 * mask of the MID field inside the ICBUS_CONFIG register shifted by
 * MID_SHIFT bits to the right
 */
static uint16_t mid_mask;

/** Print version information. */
static void version_print(void)
{
	printf("HelenOS SPARC64 Bootloader\nRelease %s%s%s\n"
	    "Copyright (c) 2006 HelenOS project\n",
	    release, revision, timestamp);
}

/* the lowest ID (read from the VER register) of some US3 CPU model */
#define FIRST_US3_CPU  0x14

/* the greatest ID (read from the VER register) of some US3 CPU model */
#define LAST_US3_CPU   0x19

/* UltraSPARC IIIi processor implementation code */
#define US_IIIi_CODE   0x15

/**
 * Sets the global variables "subarchitecture" and "mid_mask" to
 * correct values.
 */
static void detect_subarchitecture(void)
{
	uint64_t v;
	asm volatile (
		"rdpr %%ver, %0\n"
		: "=r" (v)
	);
	
	v = (v << 16) >> 48;
	if ((v >= FIRST_US3_CPU) && (v <= LAST_US3_CPU)) {
		subarchitecture = SUBARCH_US3;
		if (v == US_IIIi_CODE)
			mid_mask = (1 << 5) - 1;
		else
			mid_mask = (1 << 10) - 1;
	} else if (v < FIRST_US3_CPU) {
		subarchitecture = SUBARCH_US;
		mid_mask = (1 << 5) - 1;
	} else
		printf("\nThis CPU is not supported by HelenOS.");
}

void bootstrap(void)
{
	void *base = (void *) KERNEL_VIRTUAL_ADDRESS;
	void *balloc_base;
	unsigned int top = 0;
	unsigned int i;
	unsigned int j;
	
	version_print();
	
	detect_subarchitecture();
	init_components(components);
	
	if (!ofw_get_physmem_start(&bootinfo.physmem_start)) {
		printf("Error: unable to get start of physical memory.\n");
		halt();
	}
	
	if (!ofw_memmap(&bootinfo.memmap)) {
		printf("Error: unable to get memory map, halting.\n");
		halt();
	}
	
	if (bootinfo.memmap.total == 0) {
		printf("Error: no memory detected, halting.\n");
		halt();
	}
	
	/*
	 * SILO for some reason adds 0x400000 and subtracts
	 * bootinfo.physmem_start to/from silo_ramdisk_image.
	 * We just need plain physical address so we fix it up.
	 */
	if (silo_ramdisk_image) {
		silo_ramdisk_image += bootinfo.physmem_start;
		silo_ramdisk_image -= 0x400000;
		
		/* Install 1:1 mapping for the RAM disk. */
		if (ofw_map((void *) ((uintptr_t) silo_ramdisk_image),
		    (void *) ((uintptr_t) silo_ramdisk_image),
		    silo_ramdisk_size, -1) != 0) {
			printf("Failed to map RAM disk.\n");
			halt();
		}
	}
	
	printf("\nMemory statistics (total %d MB, starting at %P)\n",
	    bootinfo.memmap.total >> 20, bootinfo.physmem_start);
	printf(" %P: kernel entry point\n", KERNEL_VIRTUAL_ADDRESS);
	printf(" %P: boot info structure\n", &bootinfo);
	
	/*
	 * Figure out destination address for each component.
	 * In this phase, we don't copy the components yet because we want to
	 * to be careful not to overwrite anything, especially the components
	 * which haven't been copied yet.
	 */
	bootinfo.taskmap.count = 0;
	for (i = 0; i < COMPONENTS; i++) {
		printf(" %P: %s image (size %d bytes)\n", components[i].start,
		    components[i].name, components[i].size);
		top = ALIGN_UP(top, PAGE_SIZE);
		if (i > 0) {
			if (bootinfo.taskmap.count == TASKMAP_MAX_RECORDS) {
				printf("Skipping superfluous components.\n");
				break;
			}
			
			bootinfo.taskmap.tasks[bootinfo.taskmap.count].addr =
			    base + top;
			bootinfo.taskmap.tasks[bootinfo.taskmap.count].size =
			    components[i].size;
			strncpy(bootinfo.taskmap.tasks[
			    bootinfo.taskmap.count].name, components[i].name,
			    BOOTINFO_TASK_NAME_BUFLEN);
			bootinfo.taskmap.count++;
		}
		top += components[i].size;
	}
	
	printf("\n");
	
	/* Do not consider RAM disk */
	j = bootinfo.taskmap.count - 1;
	
	if (silo_ramdisk_image) {
		/* Treat the RAM disk as the last bootinfo task. */
		if (bootinfo.taskmap.count == TASKMAP_MAX_RECORDS) {
			printf("Skipping RAM disk.\n");
			goto skip_ramdisk;
		}
		
		top = ALIGN_UP(top, PAGE_SIZE);
		bootinfo.taskmap.tasks[bootinfo.taskmap.count].addr = 
		    base + top;
		bootinfo.taskmap.tasks[bootinfo.taskmap.count].size =
		    silo_ramdisk_size;
		bootinfo.taskmap.count++;
		printf("Copying RAM disk...");
		
		/*
		 * Claim and map the whole ramdisk as it may exceed the area
		 * given to us by SILO.
		 */
		(void) ofw_claim_phys(base + top, silo_ramdisk_size);
		(void) ofw_map(bootinfo.physmem_start + base + top, base + top,
		    silo_ramdisk_size, -1);
		memmove(base + top, (void *) ((uintptr_t) silo_ramdisk_image),
		    silo_ramdisk_size);
		
		printf("done.\n");
		top += silo_ramdisk_size;
	}
skip_ramdisk:
	
	/*
	 * Now we can proceed to copy the components. We do it in reverse order
	 * so that we don't overwrite anything even if the components overlap
	 * with base.
	 */
	printf("Copying tasks...");
	for (i = COMPONENTS - 1; i > 0; i--, j--) {
		printf("%s ", components[i].name);
		
		/*
		 * At this point, we claim the physical memory that we are
		 * going to use. We should be safe in case of the virtual
		 * address space because the OpenFirmware, according to its
		 * SPARC binding, should restrict its use of virtual memory
		 * to addresses from [0xffd00000; 0xffefffff] and
		 * [0xfe000000; 0xfeffffff].
		 *
		 * XXX We don't map this piece of memory. We simply rely on
		 *     SILO to have it done for us already in this case.
		 */
		(void) ofw_claim_phys(bootinfo.physmem_start +
		    bootinfo.taskmap.tasks[j].addr,
		    ALIGN_UP(components[i].size, PAGE_SIZE));
		
		memcpy((void *) bootinfo.taskmap.tasks[j].addr,
		    components[i].start, components[i].size);
		
	}
	printf(".\n");
	
	printf("Copying kernel...");
	(void) ofw_claim_phys(bootinfo.physmem_start + base,
	    ALIGN_UP(components[0].size, PAGE_SIZE));
	memcpy(base, components[0].start, components[0].size);
	printf("done.\n");
	
	/*
	 * Claim and map the physical memory for the boot allocator.
	 * Initialize the boot allocator.
	 */
	balloc_base = base + ALIGN_UP(top, PAGE_SIZE);
	(void) ofw_claim_phys(bootinfo.physmem_start + balloc_base,
	    BALLOC_MAX_SIZE);
	(void) ofw_map(bootinfo.physmem_start + balloc_base, balloc_base,
	    BALLOC_MAX_SIZE, -1);
	balloc_init(&bootinfo.ballocs, (uintptr_t) balloc_base,
	    (uintptr_t) balloc_base);
	
	printf("Setting up screens...");
	ofw_setup_screens();
	printf("done.\n");
	
	printf("Canonizing OpenFirmware device tree...");
	bootinfo.ofw_root = ofw_tree_build();
	printf("done.\n");
	
#ifdef CONFIG_AP
	printf("Checking for secondary processors...");
	if (!ofw_cpu(mid_mask, bootinfo.physmem_start))
		printf("Error: unable to get CPU properties\n");
	printf("done.\n");
#endif
	
	printf("Booting the kernel...\n");
	jump_to_kernel((void *) KERNEL_VIRTUAL_ADDRESS,
	    bootinfo.physmem_start | BSP_PROCESSOR, &bootinfo,
	    sizeof(bootinfo), subarchitecture);
}
