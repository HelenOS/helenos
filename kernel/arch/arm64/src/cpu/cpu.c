/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
