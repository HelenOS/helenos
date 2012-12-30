/*
 * Copyright (c) 2007 Michal Kebrt
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

/** @addtogroup arm32
 * @{
 */
/** @file
 *  @brief CPU identification.
 */

#include <arch/cpu.h>
#include <cpu.h>
#include <arch.h>
#include <print.h>

/** Implementers (vendor) names */
static const char * implementer(unsigned id)
{
	switch (id)
	{
	case 0x41: return "ARM Limited";
	case 0x44: return "Digital Equipment Corporation";
	case 0x4d: return "Motorola, Freescale Semiconductor Inc.";
	case 0x51: return "Qualcomm Inc.";
	case 0x56: return "Marvell Semiconductor Inc.";
	case 0x69: return "Intel Corporation";
	}
	return "Unknown implementer";
}

/** Architecture names */
static const char * architecture_string(cpu_arch_t *arch)
{
	static const char *arch_data[] = {
		"ARM",       /* 0x0 */
		"ARMv4",       /* 0x1 */
		"ARMv4T",      /* 0x2 */
		"ARMv5",       /* 0x3 */
		"ARMv5T",      /* 0x4 */
		"ARMv5TE",     /* 0x5 */
		"ARMv5TEJ",    /* 0x6 */
		"ARMv6"        /* 0x7 */
	};
	if (arch->arch_num < (sizeof(arch_data) / sizeof(arch_data[0])))
		return arch_data[arch->arch_num];
	else
		return arch_data[0];
}


/** Retrieves processor identification from CP15 register 0.
 *
 * @param cpu Structure for storing CPU identification.
 * See page B4-1630 of ARM Architecture Reference Manual.
 */
static void arch_cpu_identify(cpu_arch_t *cpu)
{
	uint32_t ident;
	asm volatile (
		"mrc p15, 0, %[ident], c0, c0, 0\n"
		: [ident] "=r" (ident)
	);
	
	cpu->imp_num = ident >> 24;
	cpu->variant_num = (ident << 8) >> 28;
	cpu->arch_num = (ident << 12) >> 28;
	cpu->prim_part_num = (ident << 16) >> 20;
	cpu->rev_num = (ident << 28) >> 28;
	// TODO CPUs with arch_num == 0xf use CPUID scheme for identification
}

/** Enables unaligned access and caching for armv6+ */
void cpu_arch_init(void)
{
	uint32_t control_reg = 0;
	asm volatile (
		"mrc p15, 0, %[control_reg], c1, c0"
		: [control_reg] "=r" (control_reg)
	);
	
	/* Turn off tex remap, RAZ/WI prior to armv7 */
	control_reg &= ~CP15_R1_TEX_REMAP_EN;
	/* Turn off accessed flag, RAZ/WI prior to armv7 */
	control_reg &= ~(CP15_R1_ACCESS_FLAG_EN | CP15_R1_HW_ACCESS_FLAG_EN);
	/* Enable branch prediction RAZ/WI if not supported */
	control_reg |= CP15_R1_BRANCH_PREDICT_EN;

	/* Unaligned access is supported on armv6+ */
#if defined(PROCESSOR_ARCH_armv7_a) | defined(PROCESSOR_ARCH_armv6)
	/* Enable unaligned access, RAZ/WI prior to armv6
	 * switchable on armv6, RAO/WI writes on armv7,
	 * see ARM Architecture Reference Manual ARMv7-A and ARMv7-R edition
	 * L.3.1 (p. 2456) */
	control_reg |= CP15_R1_UNALIGNED_EN;
	/* Disable alignment checks, this turns unaligned access to undefined,
	 * unless U bit is set. */
	control_reg &= ~CP15_R1_ALIGN_CHECK_EN;
	/* Enable caching, On arm prior to armv7 there is only one level
	 * of caches. Data cache is coherent.
	 * "This means that the behavior of accesses from the same observer to
	 * different VAs, that are translated to the same PA
	 * with the same memory attributes, is fully coherent."
	 *    ARM Architecture Reference Manual ARMv7-A and ARMv7-R Edition
	 *    B3.11.1 (p. 1383)
	 * We are safe to turn this on. For arm v6 see ch L.6.2 (p. 2469)
	 * L2 Cache for armv7 was enabled in boot code.
	 */
	control_reg |= CP15_R1_CACHE_EN;
#endif
#ifdef PROCESSOR_cortex_a8
	 /* ICache coherency is elaborate on in barrier.h.
	  * Cortex-A8 implements IVIPT extension.
	  * Cortex-A8 TRM ch. 7.2.6 p. 7-4 (PDF 245) */
	control_reg |= CP15_R1_INST_CACHE_EN;
#endif
	
	asm volatile (
		"mcr p15, 0, %[control_reg], c1, c0"
		:: [control_reg] "r" (control_reg)
	);
#ifdef CONFIG_FPU
	fpu_setup();
#endif
}

/** Retrieves processor identification and stores it to #CPU.arch */
void cpu_identify(void)
{
	arch_cpu_identify(&CPU->arch);
}

/** Prints CPU identification. */
void cpu_print_report(cpu_t *m)
{
	printf("cpu%d: vendor=%s, architecture=%s, part number=%x, "
	    "variant=%x, revision=%x\n",
	    m->id, implementer(m->arch.imp_num),
	    architecture_string(&m->arch), m->arch.prim_part_num,
	    m->arch.variant_num, m->arch.rev_num);
}

/** @}
 */
