/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#include <cpu.h>
#include <arch.h>
#include <arch/register.h>
#include <stdio.h>
#include <memw.h>

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
