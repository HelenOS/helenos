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

/** Number of indexes left out in the #imp_data array */
#define IMP_DATA_START_OFFSET 0x40

/** Implementators (vendor) names */
static const char *imp_data[] = {
	"?",					/* IMP_DATA_START_OFFSET */
	"ARM Ltd",				/* 0x41 */
	"",					/* 0x42 */
	"",                             	/* 0x43 */
	"Digital Equipment Corporation",	/* 0x44 */
	"", "", "", "", "", "", "", "", "", "",	/* 0x45 - 0x4e */
	"", "", "", "", "", "", "", "", "", "", /* 0x4f - 0x58 */
	"", "", "", "", "", "", "", "", "", "", /* 0x59 - 0x62 */
	"", "", "", "", "", "",			/* 0x63 - 0x68 */
	"Intel Corporation"			/* 0x69 */
};

/** Length of the #imp_data array */
static unsigned int imp_data_length = sizeof(imp_data) / sizeof(char *);

/** Architecture names */
static const char *arch_data[] = {
	"?",       /* 0x0 */
	"4",       /* 0x1 */
	"4T",      /* 0x2 */
	"5",       /* 0x3 */
	"5T",      /* 0x4 */
	"5TE",     /* 0x5 */
	"5TEJ",    /* 0x6 */
	"6"        /* 0x7 */
};

/** Length of the #arch_data array */
static unsigned int arch_data_length = sizeof(arch_data) / sizeof(char *);


/** Retrieves processor identification from CP15 register 0.
 * 
 * @param cpu Structure for storing CPU identification.
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
}

/** Does nothing on ARM. */
void cpu_arch_init(void)
{
}

/** Retrieves processor identification and stores it to #CPU.arch */
void cpu_identify(void) 
{
	arch_cpu_identify(&CPU->arch);
}

/** Prints CPU identification. */
void cpu_print_report(cpu_t *m)
{
	const char *vendor = imp_data[0];
	const char *architecture = arch_data[0];
	cpu_arch_t * cpu_arch = &m->arch;

	if ((cpu_arch->imp_num) > 0 &&
	    (cpu_arch->imp_num < (imp_data_length + IMP_DATA_START_OFFSET))) {
		vendor = imp_data[cpu_arch->imp_num - IMP_DATA_START_OFFSET];
	}

	if ((cpu_arch->arch_num) > 0 &&
	    (cpu_arch->arch_num < arch_data_length)) {
		architecture = arch_data[cpu_arch->arch_num];
	}

	printf("cpu%d: vendor=%s, architecture=ARM%s, part number=%x, "
	    "variant=%x, revision=%x\n",
	    m->id, vendor, architecture, cpu_arch->prim_part_num,
	    cpu_arch->variant_num, cpu_arch->rev_num);
}

/** @}
 */
