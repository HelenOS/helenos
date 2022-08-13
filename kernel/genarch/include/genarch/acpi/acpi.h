/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
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
