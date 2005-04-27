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

#ifndef __ACPI_H__
#define __ACPI_H__

#include <arch/types.h>

/* Root System Description Pointer */
struct acpi_rsdp {
	__u8 signature[8];
	__u8 checksum;
	__u8 oemid[6];
	__u8 revision;
	__u32 rsdt_address;
	__u32 length;
	__u64 xsdt_address;
	__u32 ext_checksum;
	__u8 reserved[3];
} __attribute__ ((packed));

/* System Description Table Header */
struct acpi_sdt_header {
	__u8 signature[4];
	__u32 length;
	__u8 revision;
	__u8 checksum;
	__u8 oemid[6];
	__u8 oem_table_id[8];
	__u32 oem_revision;
	__u32 creator_id;
	__u32 creator_revision;
} __attribute__ ((packed));;

/* Root System Description Table */
struct acpi_rsdt {
	struct acpi_sdt_header header;
	__u32 entry[];
} __attribute__ ((packed));;

/* Extended System Description Table */
struct acpi_xsdt {
	struct acpi_sdt_header header;
	__u64 entry[];
} __attribute__ ((packed));;

extern struct acpi_rsdp *acpi_rsdp;

extern void acpi_init(void);

static int rsdp_check(__u8 *rsdp);

#endif /* __ACPI_H__ */
