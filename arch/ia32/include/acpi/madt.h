/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __MADT_H__
#define __MADT_H__

#include <arch/acpi/acpi.h>

/* Multiple APIC Description Table */
struct acpi_madt {
	struct acpi_sdt_header header;
	__u32 l_apic_address;
	__u32 flags;
	__u8  apic_strucure[];
} __attribute__ ((packed));

#define	MADT_L_APIC			0
#define MADT_IO_APIC			1
#define MADT_INTR_SRC_OVRD		2
#define MADT_NMI_SRC			3
#define MADT_L_APIC_NMI			4
#define MADT_L_APIC_ADDR_OVRD		5
#define MADT_IO_SAPIC			6
#define MADT_L_SAPIC			7
#define MADT_PLATFORM_INTR_SRC		8
#define MADT_RESERVED_SKIP_BEGIN	9
#define MADT_RESERVED_SKIP_END		127
#define MADT_RESERVED_OEM_BEGIN		128
#define MADT_RESERVED_OEM_END		255

struct madt_l_apic {
	__u8 type;
	__u8 length;
	__u8 acpi_id;
	__u8 apic_id;
	__u32 flags;	
} __attribute__ ((packed));

struct madt_io_apic {
	__u8 type;
	__u8 length;
	__u8 io_apic_id;
	__u8 reserved;
	__u32 io_apic_address;	
	__u32 global_intr_base;
} __attribute__ ((packed));

struct madt_intr_src_ovrd {
	__u8 type;
	__u8 length;
	__u8 bus;
	__u8 source;
	__u32 global_intr;
	__u16 flags;
} __attribute__ ((packed));

struct madt_nmi_src {
	__u8 type;
	__u8 length;
	__u16 flags;
	__u32 global_intr;
} __attribute__ ((packed));

struct madt_l_apic_nmi {
	__u8 type;
	__u8 length;
	__u8 acpi_id;
	__u16 flags;
	__u8 l_apic_lint;
} __attribute__ ((packed));

struct madt_l_apic_addr_ovrd {
	__u8 type;
	__u8 length;
	__u16 reserved;
	__u64 l_apic_address;
} __attribute__ ((packed));

struct madt_io_sapic {
	__u8 type;
	__u8 length;
	__u8 io_apic_id;
	__u8 reserved;
	__u32 global_intr_base;
	__u64 io_apic_address;		
} __attribute__ ((packed));

struct madt_l_sapic {
	__u8 type;
	__u8 length;
	__u8 acpi_id;
	__u8 sapic_id;
	__u8 sapic_eid;
	__u8 reserved[3];
	__u32 flags;
	__u32 acpi_processor_uid_value;
	__u8 acpi_processor_uid_str[1];
} __attribute__ ((packed));

struct madt_platform_intr_src {
	__u8 type;
	__u8 length;
	__u16 flags;
	__u8 intr_type;
	__u8 processor_id;
	__u8 processor_eid;
	__u8 io_sapic_vector;
	__u32 global_intr;
	__u32 platform_intr_src_flags;
} __attribute__ ((packed));

extern struct acpi_madt *acpi_madt;

#endif /* __MADT_H__ */
