/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2009 Jiri Svoboda
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup ia32
 * @{
 */
/** @file
 */

#include <arch.h>

#include <arch/types.h>

#include <arch/pm.h>

#include <genarch/drivers/legacy/ia32/io.h>
#include <genarch/drivers/ega/ega.h>
#include <arch/drivers/vesa.h>
#include <genarch/kbd/i8042.h>
#include <arch/drivers/i8254.h>
#include <arch/drivers/i8259.h>

#include <arch/context.h>

#include <config.h>

#include <arch/interrupt.h>
#include <arch/asm.h>
#include <genarch/acpi/acpi.h>

#include <arch/bios/bios.h>

#include <interrupt.h>
#include <ddi/irq.h>
#include <arch/debugger.h>
#include <proc/thread.h>
#include <syscall/syscall.h>
#include <console/console.h>
#include <ddi/device.h>
#include <sysinfo/sysinfo.h>
#include <arch/boot/boot.h>
#include <string.h>
#include <macros.h>

#ifdef CONFIG_SMP
#include <arch/smp/apic.h>
#endif

/** Extract command name from the multiboot module command line.
 *
 * @param buf      Destination buffer (will always null-terminate).
 * @param n        Size of destination buffer.
 * @param cmd_line Input string (the command line).
 *
 */
static void extract_command(char *buf, size_t n, const char *cmd_line)
{
	const char *start, *end, *cp;
	size_t max_len;
	
	/* Find the first space. */
	end = strchr(cmd_line, ' ');
	if (end == NULL)
		end = cmd_line + strlen(cmd_line);
	
	/*
	 * Find last occurence of '/' before 'end'. If found, place start at
	 * next character. Otherwise, place start at beginning of buffer.
	 */
	cp = end;
	start = buf;
	while (cp != start) {
		if (*cp == '/') {
			start = cp + 1;
			break;
		}
		--cp;
	}
	
	/* Copy the command and null-terminate the string. */
	max_len = min(n - 1, (size_t) (end - start));
	strncpy(buf, start, max_len + 1);
	buf[max_len] = '\0';
}

/** C part of ia32 boot sequence.
 *
 * @param signature Should contain the multiboot signature.
 * @param mi        Pointer to the multiboot information structure.
 */
void arch_pre_main(uint32_t signature, const mb_info_t *mi)
{
	uint32_t flags;
	mb_mod_t *mods;
	uint32_t i;
	
	if (signature == MULTIBOOT_LOADER_MAGIC)
		flags = mi->flags;
	else {
		/* No multiboot info available. */
		flags = 0;
	}
	
	/* Copy module information. */
	
	if ((flags & MBINFO_FLAGS_MODS) != 0) {
		init.cnt = mi->mods_count;
		mods = mi->mods_addr;
		
		for (i = 0; i < init.cnt; i++) {
			init.tasks[i].addr = mods[i].start + 0x80000000;
			init.tasks[i].size = mods[i].end - mods[i].start;
			
			/* Copy command line, if available. */
			if (mods[i].string) {
				extract_command(init.tasks[i].name,
				    CONFIG_TASK_NAME_BUFLEN,
				    mods[i].string);
			} else
				init.tasks[i].name[0] = '\0';
		}
	} else
		init.cnt = 0;
	
	/* Copy memory map. */
	
	int32_t mmap_length;
	mb_mmap_t *mme;
	uint32_t size;
	
	if ((flags & MBINFO_FLAGS_MMAP) != 0) {
		mmap_length = mi->mmap_length;
		mme = mi->mmap_addr;
		e820counter = 0;
		
		i = 0;
		while (mmap_length > 0) {
			e820table[i++] = mme->mm_info;
			
			/* Compute address of next structure. */
			size = sizeof(mme->size) + mme->size;
			mme = ((void *) mme) + size;
			mmap_length -= size;
		}
		
		e820counter = i;
	} else
		e820counter = 0;
	
#ifdef CONFIG_SMP
	/* Copy AP bootstrap routines below 1 MB. */
	memcpy((void *) AP_BOOT_OFFSET, (void *) BOOT_OFFSET,
	    (size_t) &_hardcoded_unmapped_size);
#endif
}

void arch_pre_mm_init(void)
{
	pm_init();

	if (config.cpu_active == 1) {
		interrupt_init();
		bios_init();
		
		/* PIC */
		i8259_init();
	}
}

void arch_post_mm_init(void)
{
	if (config.cpu_active == 1) {
		/* Initialize IRQ routing */
		irq_init(IRQ_COUNT, IRQ_COUNT);
		
		/* hard clock */
		i8254_init();

#ifdef CONFIG_FB
		if (vesa_present()) 
			vesa_init();
		else
#endif
			ega_init(EGA_BASE, EGA_VIDEORAM);	/* video */
		
		/* Enable debugger */
		debugger_init();
		/* Merge all memory zones to 1 big zone */
		zone_merge_all();
	}
}

void arch_post_cpu_init()
{
#ifdef CONFIG_SMP
        if (config.cpu_active > 1) {
		l_apic_init();
		l_apic_debug();
	}
#endif
}

void arch_pre_smp_init(void)
{
	if (config.cpu_active == 1) {
#ifdef CONFIG_SMP
		acpi_init();
#endif /* CONFIG_SMP */
	}
}

void arch_post_smp_init(void)
{
	devno_t devno = device_assign_devno();
	/* keyboard controller */
	(void) i8042_init((i8042_t *) I8042_BASE, devno, IRQ_KBD);

	/*
	 * This is the necessary evil until the userspace driver is entirely
	 * self-sufficient.
	 */
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.devno", NULL, devno);
	sysinfo_set_item_val("kbd.inr", NULL, IRQ_KBD);
}

void calibrate_delay_loop(void)
{
	i8254_calibrate_delay_loop();
	if (config.cpu_active == 1) {
		/*
		 * This has to be done only on UP.
		 * On SMP, i8254 is not used for time keeping and its interrupt pin remains masked.
		 */
		i8254_normal_operation();
	}
}

/** Set thread-local-storage pointer
 *
 * TLS pointer is set in GS register. That means, the GS contains
 * selector, and the descriptor->base is the correct address.
 */
unative_t sys_tls_set(unative_t addr)
{
	THREAD->arch.tls = addr;
	set_tls_desc(addr);

	return 0;
}

/** Acquire console back for kernel
 *
 */
void arch_grab_console(void)
{
#ifdef CONFIG_FB
	vesa_redraw();
#else
	ega_redraw();
#endif
}

/** Return console to userspace
 *
 */
void arch_release_console(void)
{
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

/** @}
 */
