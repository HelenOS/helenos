/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#include <cpu.h>
#include <arch.h>
#include <arch/register.h>
#include <stdio.h>
#include <mem.h>

void cpu_arch_init(void)
{
}

void cpu_identify(void)
{
	CPU->arch.cpuid0 = cpuid_read(0);
	CPU->arch.cpuid1 = cpuid_read(1);
	CPU->arch.cpuid3.value = cpuid_read(3);
}

void cpu_print_report(cpu_t *m)
{
	const char *family_str;
	char vendor[2 * sizeof(uint64_t) + 1];

	memcpy(vendor, &CPU->arch.cpuid0, 8);
	memcpy(vendor + 8, &CPU->arch.cpuid1, 8);
	vendor[sizeof(vendor) - 1] = 0;

	switch (m->arch.cpuid3.family) {
	case FAMILY_ITANIUM:
		family_str = "Itanium";
		break;
	case FAMILY_ITANIUM2:
		family_str = "Itanium 2";
		break;
	default:
		family_str = "Unknown";
		break;
	}

	printf("cpu%d: %s (%s), archrev=%d, model=%d, revision=%d\n", CPU->id,
	    family_str, vendor, CPU->arch.cpuid3.archrev,
	    CPU->arch.cpuid3.model, CPU->arch.cpuid3.revision);
}

/** @}
 */
