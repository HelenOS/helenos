/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32
 * @{
 */
/** @file
 */

#include <arch/cpu.h>
#include <cpu.h>
#include <arch.h>
#include <stdio.h>
#include <fpu_context.h>

void cpu_arch_init(void)
{
#ifdef CONFIG_FPU
	fpu_enable();
#endif
}

void cpu_identify(void)
{
	cpu_version(&CPU->arch);
}

void cpu_print_report(cpu_t *cpu)
{
	const char *name;

	switch (cpu->arch.version) {
	case 8:
		name = "PowerPC 750";
		break;
	case 9:
		name = "PowerPC 604e";
		break;
	case 0x81:
		name = "PowerPC 8260";
		break;
	case 0x8081:
		name = "PowerPC 826xA";
		break;
	default:
		name = "unknown";
	}

	printf("cpu%u: version=%" PRIu16 " (%s), revision=%" PRIu16 "\n", cpu->id,
	    cpu->arch.version, name, cpu->arch.revision);
}

/** @}
 */
