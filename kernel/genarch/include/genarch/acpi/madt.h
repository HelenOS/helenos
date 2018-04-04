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

/** @addtogroup genarch
 * @{
 */
/** @file
 */

#ifndef KERN_MADT_H_
#define KERN_MADT_H_

#include <genarch/acpi/acpi.h>
#include <arch/smp/apic.h>
#include <arch/smp/smp.h>

#define MADT_L_APIC               0
#define MADT_IO_APIC              1
#define MADT_INTR_SRC_OVRD        2
#define MADT_NMI_SRC              3
#define MADT_L_APIC_NMI           4
#define MADT_L_APIC_ADDR_OVRD     5
#define MADT_IO_SAPIC             6
#define MADT_L_SAPIC              7
#define MADT_PLATFORM_INTR_SRC    8
#define MADT_RESERVED_SKIP_BEGIN  9
#define MADT_RESERVED_SKIP_END    127
#define MADT_RESERVED_OEM_BEGIN   128

struct madt_apic_header {
	uint8_t type;
	uint8_t length;
} __attribute__((packed));

/* Multiple APIC Description Table */
struct acpi_madt {
	struct acpi_sdt_header header;
	uint32_t l_apic_address;
	uint32_t flags;
	struct madt_apic_header apic_header[];
} __attribute__((packed));

struct madt_l_apic {
	struct madt_apic_header header;
	uint8_t acpi_id;
	uint8_t apic_id;
	uint32_t flags;
} __attribute__((packed));

struct madt_io_apic {
	struct madt_apic_header header;
	uint8_t io_apic_id;
	uint8_t reserved;
	uint32_t io_apic_address;
	uint32_t global_intr_base;
} __attribute__((packed));

struct madt_intr_src_ovrd {
	struct madt_apic_header header;
	uint8_t bus;
	uint8_t source;
	uint32_t global_int;
	uint16_t flags;
} __attribute__((packed));

struct madt_nmi_src {
	struct madt_apic_header header;
	uint16_t flags;
	uint32_t global_intr;
} __attribute__((packed));

struct madt_l_apic_nmi {
	struct madt_apic_header header;
	uint8_t acpi_id;
	uint16_t flags;
	uint8_t l_apic_lint;
} __attribute__((packed));

struct madt_l_apic_addr_ovrd {
	struct madt_apic_header header;
	uint16_t reserved;
	uint64_t l_apic_address;
} __attribute__((packed));

struct madt_io_sapic {
	struct madt_apic_header header;
	uint8_t io_apic_id;
	uint8_t reserved;
	uint32_t global_intr_base;
	uint64_t io_apic_address;
} __attribute__((packed));

struct madt_l_sapic {
	struct madt_apic_header header;
	uint8_t acpi_id;
	uint8_t sapic_id;
	uint8_t sapic_eid;
	uint8_t reserved[3];
	uint32_t flags;
	uint32_t acpi_processor_uid_value;
	uint8_t acpi_processor_uid_str[1];
} __attribute__((packed));

struct madt_platform_intr_src {
	struct madt_apic_header header;
	uint16_t flags;
	uint8_t intr_type;
	uint8_t processor_id;
	uint8_t processor_eid;
	uint8_t io_sapic_vector;
	uint32_t global_intr;
	uint32_t platform_intr_src_flags;
} __attribute__((packed));

extern struct acpi_madt *acpi_madt;
extern struct smp_config_operations madt_config_operations;

extern void acpi_madt_parse(void);

#endif /* KERN_MADT_H_ */

/** @}
 */
