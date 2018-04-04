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

#ifndef KERN_ACPI_H_
#define KERN_ACPI_H_

#include <typedefs.h>

/* Root System Description Pointer */
struct acpi_rsdp {
	uint8_t signature[8];
	uint8_t checksum;
	uint8_t oemid[6];
	uint8_t revision;
	uint32_t rsdt_address;
	uint32_t length;
	uint64_t xsdt_address;
	uint32_t ext_checksum;
	uint8_t reserved[3];
} __attribute__((packed));

/* System Description Table Header */
struct acpi_sdt_header {
	uint8_t signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	uint8_t oemid[6];
	uint8_t oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} __attribute__((packed));

struct acpi_signature_map {
	uint8_t *signature;
	struct acpi_sdt_header **sdt_ptr;
	const char *description;
};

/* Root System Description Table */
struct acpi_rsdt {
	struct acpi_sdt_header header;
	uint32_t entry[];
} __attribute__((packed));

/* Extended System Description Table */
struct acpi_xsdt {
	struct acpi_sdt_header header;
	uint64_t entry[];
} __attribute__((packed));

extern struct acpi_rsdp *acpi_rsdp;
extern struct acpi_rsdt *acpi_rsdt;
extern struct acpi_xsdt *acpi_xsdt;

extern void acpi_init(void);
extern int acpi_sdt_check(uint8_t *sdt);

#endif /* KERN_ACPI_H_ */

/** @}
 */
