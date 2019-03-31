/*
 * Copyright (c) 2015 Petr Pavlu
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

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief CPU identification.
 */

#include <arch/regutils.h>
#include <cpu.h>
#include <typedefs.h>

/** Decode implementer (vendor) name. */
static const char *implementer(uint32_t id)
{
	switch (id) {
	case 0x41:
		return "ARM Limited";
	case 0x42:
		return "Broadcom Corporation";
	case 0x43:
		return "Cavium Inc.";
	case 0x44:
		return "Digital Equipment Corporation";
	case 0x49:
		return "Infineon Technologies AG";
	case 0x4d:
		return "Motorola or Freescale Semiconductor Inc.";
	case 0x4e:
		return "NVIDIA Corporation";
	case 0x50:
		return "Applied Micro Circuits Corporation";
	case 0x51:
		return "Qualcomm Inc.";
	case 0x56:
		return "Marvell International Ltd.";
	case 0x69:
		return "Intel Corporation";
	}
	return "Unknown implementer";
}

/** Perform ARM64-specific tasks needed for CPU initialization. */
void cpu_arch_init(void)
{
}

/** Retrieve processor identification and stores it to #CPU.arch */
void cpu_identify(void)
{
	uint64_t midr = MIDR_EL1_read();

	CPU->arch.implementer =
	    (midr & MIDR_IMPLEMENTER_MASK) >> MIDR_IMPLEMENTER_SHIFT;
	CPU->arch.variant = (midr & MIDR_VARIANT_MASK) >> MIDR_VARIANT_SHIFT;
	CPU->arch.partnum = (midr & MIDR_PARTNUM_MASK) >> MIDR_PARTNUM_SHIFT;
	CPU->arch.revision = (midr & MIDR_REVISION_MASK) >> MIDR_REVISION_SHIFT;
}

/** Print CPU identification. */
void cpu_print_report(cpu_t *m)
{
	printf("cpu%d: vendor=%s, variant=%" PRIx32 ", part number=%" PRIx32
	    ", revision=%" PRIx32 "\n",
	    m->id, implementer(m->arch.implementer), m->arch.variant,
	    m->arch.partnum, m->arch.revision);
}

/** @}
 */
