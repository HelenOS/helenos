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

#include <arch/main.h>
#include <arch/arch.h>
#include <arch/asm.h>
#include <arch/ofw.h>
#include <genarch/ofw.h>
#include <genarch/ofw_tree.h>
#include <halt.h>
#include <printf.h>
#include <mem.h>
#include <version.h>
#include <macros.h>
#include <align.h>
#include <str.h>
#include <errno.h>
#include <payload.h>
#include <kernel.h>

/* The lowest ID (read from the VER register) of some US3 CPU model */
#define FIRST_US3_CPU  0x14

/* The greatest ID (read from the VER register) of some US3 CPU model */
#define LAST_US3_CPU  0x19

/* UltraSPARC IIIi processor implementation code */
#define US_IIIi_CODE  0x15

#define OBP_BIAS  0x400000

#define BALLOC_MAX_SIZE  131072

#define TOP2ADDR(top)  (((void *) KERNEL_ADDRESS) + (top))

/** UltraSPARC architecture */
static uint8_t arch;

/** UltraSPARC subarchitecture */
static uint8_t subarch;

/** Mask of the MID field inside the ICBUS_CONFIG register
 *
 * Shifted by MID_SHIFT bits to the right
 *
 */
static uint16_t mid_mask;

static bootinfo_t bootinfo;

/** Detect the UltraSPARC architecture
 *
 * Detection is done by inspecting the property called "compatible"
 * in the OBP root node. Currently sun4u and sun4v are supported.
 * Set global variable "arch" to the correct value.
 *
 */
static void arch_detect(void)
{
	phandle root = ofw_find_device("/");
	char compatible[OFW_TREE_PROPERTY_MAX_VALUELEN];

	if (ofw_get_property(root, "compatible", compatible,
	    OFW_TREE_PROPERTY_MAX_VALUELEN) <= 0) {
		printf("Warning: Unable to determine architecture, assuming sun4u.\n");
		arch = ARCH_SUN4U;
		return;
	}

	if (str_cmp(compatible, "sun4v") != 0) {
		/*
		 * As not all sun4u machines have "sun4u" in their "compatible"
		 * OBP property (e.g. Serengeti's OBP "compatible" property is
		 * "SUNW,Serengeti"), we will by default fallback to sun4u if
		 * an unknown value of the "compatible" property is encountered.
		 */
		if (str_cmp(compatible, "sun4u") != 0)
			printf("Warning: Unknown architecture, assuming sun4u.\n");
		arch = ARCH_SUN4U;
	} else
		arch = ARCH_SUN4V;
}

/** Detect the subarchitecture (US, US3) of sun4u
 *
 * Set the global variables "subarch" and "mid_mask" to
 * correct values.
 *
 */
static void sun4u_subarch_detect(void)
{
	uint64_t ver;
	asm volatile (
	    "rdpr %%ver, %[ver]\n"
	    : [ver] "=r" (ver)
	);

	ver = (ver << 16) >> 48;

	if ((ver >= FIRST_US3_CPU) && (ver <= LAST_US3_CPU)) {
		subarch = SUBARCH_US3;

		if (ver == US_IIIi_CODE)
			mid_mask = (1 << 5) - 1;
		else
			mid_mask = (1 << 10) - 1;

	} else if (ver < FIRST_US3_CPU) {
		subarch = SUBARCH_US;
		mid_mask = (1 << 5) - 1;
	} else {
		printf("Warning: This CPU is not supported.");
		subarch = SUBARCH_UNKNOWN;
	}
}

/** Perform sun4u-specific SMP initialization.
 *
 */
static void sun4u_smp(void)
{
#ifdef CONFIG_AP
	printf("Checking for secondary processors ...\n");
	ofw_cpu(mid_mask, bootinfo.physmem_start);
#endif
}

/** Perform sun4v-specific fixups.
 *
 */
static void sun4v_fixups(void)
{
	/*
	 * When SILO booted, the OBP had established a virtual to physical
	 * memory mapping. This mapping is not an identity since the
	 * physical memory starts at non-zero address.
	 *
	 * However, the mapping even does not map virtual address 0
	 * onto the starting address of the physical memory, but onto an
	 * address which is 0x400000 (OBP_BIAS) bytes higher.
	 *
	 * The reason is that the OBP had already used the memory just
	 * at the beginning of the physical memory. Thus that memory cannot
	 * be used by SILO (nor the bootloader).
	 *
	 * So far, we solve it by a nasty workaround:
	 *
	 * We pretend that the physical memory starts 0x400000 (OBP_BIAS)
	 * bytes further than it actually does (and hence pretend that the
	 * physical memory is 0x400000 bytes smaller). Of course, the value
	 * 0x400000 will most probably depend on the machine and OBP version
	 * (the workaround works on Simics).
	 *
	 * A proper solution would be to inspect the "available" property
	 * of the "/memory" node to find out which parts of memory
	 * are used by OBP and redesign the algorithm of copying
	 * kernel/init tasks/ramdisk from the bootable image to memory.
	 */
	bootinfo.physmem_start += OBP_BIAS;
	bootinfo.memmap.zones[0].start += OBP_BIAS;
	bootinfo.memmap.zones[0].size -= OBP_BIAS;
	bootinfo.memmap.total -= OBP_BIAS;
}

void bootstrap(void)
{
	version_print();

	arch_detect();
	if (arch == ARCH_SUN4U)
		sun4u_subarch_detect();
	else
		subarch = SUBARCH_UNKNOWN;

	bootinfo.physmem_start = ofw_get_physmem_start();
	ofw_memmap(&bootinfo.memmap);

	if (arch == ARCH_SUN4V)
		sun4v_fixups();

	void *bootinfo_pa = ofw_translate(&bootinfo);
	void *kernel_address_pa = ofw_translate((void *) KERNEL_ADDRESS);
	void *loader_address_pa = ofw_translate((void *) LOADER_ADDRESS);

	printf("\nMemory statistics (total %" PRIu64 " MB, starting at %p)\n",
	    bootinfo.memmap.total >> 20, (void *) bootinfo.physmem_start);
	printf(" %p|%p: boot info structure\n", &bootinfo, (void *) bootinfo_pa);
	printf(" %p|%p: kernel entry point\n",
	    (void *) KERNEL_ADDRESS, (void *) kernel_address_pa);
	printf(" %p|%p: loader entry point\n",
	    (void *) LOADER_ADDRESS, (void *) loader_address_pa);

	/*
	 * At this point, we claim and map the physical memory that we
	 * are going to use. We should be safe in case of the virtual
	 * address space because the OpenFirmware, according to its
	 * SPARC binding, should restrict its use of virtual memory to
	 * addresses from [0xffd00000; 0xffefffff] and [0xfe000000;
	 * 0xfeffffff].
	 */

	size_t sz = ALIGN_UP(payload_unpacked_size(), PAGE_SIZE);
	ofw_claim_phys((void *) (bootinfo.physmem_start + KERNEL_ADDRESS), sz);
	ofw_map((void *) (bootinfo.physmem_start + KERNEL_ADDRESS),
	    (void *) KERNEL_ADDRESS, sz, -1);

	/* Extract components. */

	// TODO: Cache-coherence callback?
	extract_payload(&bootinfo.taskmap, (void *) KERNEL_ADDRESS,
	    (void *) KERNEL_ADDRESS + sz, KERNEL_ADDRESS, NULL);

	/*
	 * Claim and map the physical memory for the boot allocator.
	 * Initialize the boot allocator.
	 */
	printf("Setting up boot allocator ...\n");
	void *balloc_base = (void *) KERNEL_ADDRESS + sz;
	ofw_claim_phys(bootinfo.physmem_start + balloc_base, BALLOC_MAX_SIZE);
	ofw_map(bootinfo.physmem_start + balloc_base, balloc_base,
	    BALLOC_MAX_SIZE, -1);
	balloc_init(&bootinfo.ballocs, balloc_base, (uintptr_t) balloc_base,
	    BALLOC_MAX_SIZE);

	printf("Setting up screens ...\n");
	ofw_setup_screens();

	printf("Canonizing OpenFirmware device tree ...\n");
	bootinfo.ofw_root = ofw_tree_build();

	if (arch == ARCH_SUN4U)
		sun4u_smp();

	uintptr_t entry = check_kernel((void *) KERNEL_ADDRESS);

	printf("Booting the kernel ...\n");
	jump_to_kernel(bootinfo.physmem_start | BSP_PROCESSOR, &bootinfo,
	    subarch, (void *) entry);
}
