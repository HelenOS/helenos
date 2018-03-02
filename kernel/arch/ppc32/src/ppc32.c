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

/** @addtogroup ppc32
 * @{
 */
/** @file
 */

#include <arch.h>
#include <arch/arch.h>
#include <config.h>
#include <arch/boot/boot.h>
#include <genarch/drivers/via-cuda/cuda.h>
#include <genarch/kbrd/kbrd.h>
#include <arch/interrupt.h>
#include <interrupt.h>
#include <genarch/fb/fb.h>
#include <abi/fb/visuals.h>
#include <genarch/ofw/ofw_tree.h>
#include <genarch/ofw/pci.h>
#include <userspace.h>
#include <mm/page.h>
#include <mm/km.h>
#include <time/clock.h>
#include <abi/proc/uarg.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <ddi/irq.h>
#include <arch/drivers/pic.h>
#include <align.h>
#include <macros.h>
#include <str.h>
#include <print.h>

#define IRQ_COUNT  64
#define IRQ_CUDA   10

static void ppc32_pre_mm_init(void);
static void ppc32_post_mm_init(void);
static void ppc32_post_smp_init(void);

arch_ops_t ppc32_ops = {
	.pre_mm_init = ppc32_pre_mm_init,
	.post_mm_init = ppc32_post_mm_init,
	.post_smp_init = ppc32_post_smp_init,
};

arch_ops_t *arch_ops = &ppc32_ops;

bootinfo_t bootinfo;

static cir_t pic_cir;
static void *pic_cir_arg;

/** Performs ppc32-specific initialization before main_bsp() is called. */
void ppc32_pre_main(bootinfo_t *bootinfo)
{
	/* Copy tasks map. */
	init.cnt = min3(bootinfo->taskmap.cnt, TASKMAP_MAX_RECORDS, CONFIG_INIT_TASKS);
	size_t i;
	for (i = 0; i < init.cnt; i++) {
		init.tasks[i].paddr = KA2PA(bootinfo->taskmap.tasks[i].addr);
		init.tasks[i].size = bootinfo->taskmap.tasks[i].size;
		str_cpy(init.tasks[i].name, CONFIG_TASK_NAME_BUFLEN,
		    bootinfo->taskmap.tasks[i].name);
	}

	/* Copy physical memory map. */
	memmap.total = bootinfo->memmap.total;
	memmap.cnt = min(bootinfo->memmap.cnt, MEMMAP_MAX_RECORDS);
	for (i = 0; i < memmap.cnt; i++) {
		memmap.zones[i].start = bootinfo->memmap.zones[i].start;
		memmap.zones[i].size = bootinfo->memmap.zones[i].size;
	}

	/* Copy boot allocations info. */
	ballocs.base = bootinfo->ballocs.base;
	ballocs.size = bootinfo->ballocs.size;

	/* Copy OFW tree. */
	ofw_tree_init(bootinfo->ofw_root);
}

void ppc32_pre_mm_init(void)
{
	/* Initialize dispatch table */
	interrupt_init();

	ofw_tree_node_t *cpus_node;
	ofw_tree_node_t *cpu_node;
	ofw_tree_property_t *freq_prop;

	cpus_node = ofw_tree_lookup("/cpus");
	if (!cpus_node)
		panic("Could not find cpus node.");

	cpu_node = cpus_node->child;
	if (!cpu_node)
		panic("Could not find first cpu.");

	freq_prop = ofw_tree_getprop(cpu_node, "timebase-frequency");
	if (!freq_prop)
		panic("Could not get frequency property.");

	uint32_t freq;
	freq = *((uint32_t *) freq_prop->value);

	/* Start decrementer */
	decrementer_start(freq / HZ);
}

#ifdef CONFIG_FB
static bool display_register(ofw_tree_node_t *node, void *arg)
{
	uintptr_t fb_addr = 0;
	uint32_t fb_width = 0;
	uint32_t fb_height = 0;
	uint32_t fb_scanline = 0;
	unsigned int visual = VISUAL_UNKNOWN;

	ofw_tree_property_t *prop = ofw_tree_getprop(node, "address");
	if ((prop) && (prop->value))
		fb_addr = *((uintptr_t *) prop->value);

	prop = ofw_tree_getprop(node, "width");
	if ((prop) && (prop->value))
		fb_width = *((uint32_t *) prop->value);

	prop = ofw_tree_getprop(node, "height");
	if ((prop) && (prop->value))
		fb_height = *((uint32_t *) prop->value);

	prop = ofw_tree_getprop(node, "depth");
	if ((prop) && (prop->value)) {
		uint32_t fb_bpp = *((uint32_t *) prop->value);
		switch (fb_bpp) {
		case 8:
			visual = VISUAL_INDIRECT_8;
			break;
		case 15:
			visual = VISUAL_RGB_5_5_5_BE;
			break;
		case 16:
			visual = VISUAL_RGB_5_6_5_BE;
			break;
		case 24:
			visual = VISUAL_BGR_8_8_8;
			break;
		case 32:
			visual = VISUAL_RGB_0_8_8_8;
			break;
		default:
			visual = VISUAL_UNKNOWN;
		}
	}

	prop = ofw_tree_getprop(node, "linebytes");
	if ((prop) && (prop->value))
		fb_scanline = *((uint32_t *) prop->value);

	if ((fb_addr) && (fb_width > 0) && (fb_height > 0)
	    && (fb_scanline > 0) && (visual != VISUAL_UNKNOWN)) {
		fb_properties_t fb_prop = {
			.addr = fb_addr,
			.offset = 0,
			.x = fb_width,
			.y = fb_height,
			.scan = fb_scanline,
			.visual = visual,
		};

		outdev_t *fbdev = fb_init(&fb_prop);
		if (fbdev)
			stdout_wire(fbdev);
	}

	return true;
}
#endif

void ppc32_post_mm_init(void)
{
	if (config.cpu_active == 1) {
#ifdef CONFIG_FB
		ofw_tree_walk_by_device_type("display", display_register, NULL);
#endif
		/* Map OFW information into sysinfo */
		ofw_sysinfo_map();

		/* Initialize IRQ routing */
		irq_init(IRQ_COUNT, IRQ_COUNT);

		/* Merge all zones to 1 big zone */
		zone_merge_all();
	}
}

static bool macio_register(ofw_tree_node_t *node, void *arg)
{
	ofw_pci_reg_t *assigned_address = NULL;

	ofw_tree_property_t *prop = ofw_tree_getprop(node, "assigned-addresses");
	if ((prop) && (prop->value))
		assigned_address = ((ofw_pci_reg_t *) prop->value);

	if (assigned_address) {
		/* Initialize PIC */
		pic_init(assigned_address[0].addr, PAGE_SIZE, &pic_cir,
		    &pic_cir_arg);

#ifdef CONFIG_MAC_KBD
		uintptr_t pa = assigned_address[0].addr + 0x16000;
		uintptr_t aligned_addr = ALIGN_DOWN(pa, PAGE_SIZE);
		size_t offset = pa - aligned_addr;
		size_t size = 2 * PAGE_SIZE;

		cuda_t *cuda = (cuda_t *) (km_map(aligned_addr, offset + size,
		    PAGE_WRITE | PAGE_NOT_CACHEABLE) + offset);

		/* Initialize I/O controller */
		cuda_instance_t *cuda_instance =
		    cuda_init(cuda, IRQ_CUDA, pic_cir, pic_cir_arg);
		if (cuda_instance) {
			kbrd_instance_t *kbrd_instance = kbrd_init();
			if (kbrd_instance) {
				indev_t *sink = stdin_wire();
				indev_t *kbrd = kbrd_wire(kbrd_instance, sink);
				cuda_wire(cuda_instance, kbrd);
				pic_enable_interrupt(IRQ_CUDA);
			}
		}

		/*
		 * This is the necessary evil until the userspace driver is entirely
		 * self-sufficient.
		 */
		sysinfo_set_item_val("cuda", NULL, true);
		sysinfo_set_item_val("cuda.inr", NULL, IRQ_CUDA);
		sysinfo_set_item_val("cuda.address.physical", NULL, pa);
#endif
	}

	/* Consider only a single device for now */
	return false;
}

void irq_initialize_arch(irq_t *irq)
{
	irq->cir = pic_cir;
	irq->cir_arg = pic_cir_arg;
	irq->preack = true;
}

void ppc32_post_smp_init(void)
{
	/* Currently the only supported platform for ppc32 is 'mac'. */
	static const char *platform = "mac";

	sysinfo_set_item_data("platform", NULL, (void *) platform,
	    str_size(platform));

	ofw_tree_walk_by_device_type("mac-io", macio_register, NULL);
}

void calibrate_delay_loop(void)
{
}

void userspace(uspace_arg_t *kernel_uarg)
{
	userspace_asm((uintptr_t) kernel_uarg->uspace_uarg,
	    (uintptr_t) kernel_uarg->uspace_stack +
	    kernel_uarg->uspace_stack_size - SP_DELTA,
	    (uintptr_t) kernel_uarg->uspace_entry);

	/* Unreachable */
	while (true);
}

/** Construct function pointer
 *
 * @param fptr   function pointer structure
 * @param addr   function address
 * @param caller calling function address
 *
 * @return address of the function pointer
 *
 */
void *arch_construct_function(fncptr_t *fptr, void *addr, void *caller)
{
	return addr;
}

void arch_reboot(void)
{
	// TODO
	while (true);
}

/** @}
 */
