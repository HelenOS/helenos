/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */

/**
 * @file
 * @brief Advanced Configuration and Power Interface (ACPI) initialization.
 */

#include <genarch/acpi/acpi.h>
#include <genarch/acpi/madt.h>
#include <arch/bios/bios.h>
#include <debug.h>
#include <mm/page.h>
#include <mm/km.h>
#include <log.h>
#include <mem.h>

#define RSDP_SIGNATURE      "RSD PTR "
#define RSDP_REVISION_OFFS  15

#define CMP_SIGNATURE(left, right) \
	(((left)[0] == (right)[0]) && \
	((left)[1] == (right)[1]) && \
	((left)[2] == (right)[2]) && \
	((left)[3] == (right)[3]))

struct acpi_rsdp *acpi_rsdp = NULL;
struct acpi_rsdt *acpi_rsdt = NULL;
struct acpi_xsdt *acpi_xsdt = NULL;

struct acpi_signature_map signature_map[] = {
	{
		(uint8_t *) "APIC",
		(void *) &acpi_madt,
		"Multiple APIC Description Table"
	}
};

static int rsdp_check(uint8_t *_rsdp)
{
	struct acpi_rsdp *rsdp = (struct acpi_rsdp *) _rsdp;
	uint8_t sum = 0;
	uint32_t i;

	for (i = 0; i < 20; i++)
		sum = (uint8_t) (sum + _rsdp[i]);

	if (sum)
		return 0; /* bad checksum */

	if (rsdp->revision == 0)
		return 1; /* ACPI 1.0 */

	for (; i < rsdp->length; i++)
		sum = (uint8_t) (sum + _rsdp[i]);

	return !sum;
}

int acpi_sdt_check(uint8_t *sdt)
{
	struct acpi_sdt_header *hdr = (struct acpi_sdt_header *) sdt;
	uint8_t sum = 0;
	unsigned int i;

	for (i = 0; i < hdr->length; i++)
		sum = (uint8_t) (sum + sdt[i]);

	return !sum;
}

static struct acpi_sdt_header *map_sdt(struct acpi_sdt_header *psdt)
{
	struct acpi_sdt_header *vhdr;
	struct acpi_sdt_header *vsdt;

	/* Start with mapping the header only. */
	vhdr = (struct acpi_sdt_header *) km_map((uintptr_t) psdt,
	    sizeof(struct acpi_sdt_header), KM_NATURAL_ALIGNMENT,
	    PAGE_READ | PAGE_NOT_CACHEABLE);

	/* Now we can map the entire structure. */
	vsdt = (struct acpi_sdt_header *) km_map((uintptr_t) psdt,
	    vhdr->length, KM_NATURAL_ALIGNMENT,
	    PAGE_WRITE | PAGE_NOT_CACHEABLE);

	// TODO: do not leak vtmp

	return vsdt;
}

static void configure_via_rsdt(void)
{
	size_t i;
	size_t j;
	size_t cnt = (acpi_rsdt->header.length - sizeof(struct acpi_sdt_header)) /
	    sizeof(uint32_t);

	for (i = 0; i < cnt; i++) {
		for (j = 0; j < sizeof(signature_map) /
		    sizeof(struct acpi_signature_map); j++) {
			struct acpi_sdt_header *hdr =
			    (struct acpi_sdt_header *) (sysarg_t) acpi_rsdt->entry[i];

			struct acpi_sdt_header *vhdr = map_sdt(hdr);
			if (CMP_SIGNATURE(vhdr->signature, signature_map[j].signature)) {
				if (!acpi_sdt_check((uint8_t *) vhdr))
					break;

				*signature_map[j].sdt_ptr = vhdr;
				LOG("%p: ACPI %s", *signature_map[j].sdt_ptr,
				    signature_map[j].description);
			}
		}
	}
}

static void configure_via_xsdt(void)
{
	size_t i;
	size_t j;
	size_t cnt = (acpi_xsdt->header.length - sizeof(struct acpi_sdt_header)) /
	    sizeof(uint64_t);

	for (i = 0; i < cnt; i++) {
		for (j = 0; j < sizeof(signature_map) /
		    sizeof(struct acpi_signature_map); j++) {
			struct acpi_sdt_header *hdr =
			    (struct acpi_sdt_header *) ((uintptr_t) acpi_xsdt->entry[i]);

			struct acpi_sdt_header *vhdr = map_sdt(hdr);
			if (CMP_SIGNATURE(vhdr->signature, signature_map[j].signature)) {
				if (!acpi_sdt_check((uint8_t *) vhdr))
					break;

				*signature_map[j].sdt_ptr = vhdr;
				LOG("%p: ACPI %s", *signature_map[j].sdt_ptr,
				    signature_map[j].description);
			}
		}
	}
}

static uint8_t *search_rsdp(uint8_t *base, size_t len)
{
	for (size_t i = 0; i < len; i += 16) {
		if (!__builtin_memcmp(&base[i], RSDP_SIGNATURE, 8) &&
		    rsdp_check(&base[i]))
			return &base[i];
	}

	return NULL;
}

void acpi_init(void)
{
	/*
	 * Find Root System Description Pointer
	 * 1. search first 1K of EBDA
	 * 2. search 128K starting at 0xe0000
	 */

	uint8_t *rsdp = NULL;

	if (ebda)
		rsdp = search_rsdp((uint8_t *) PA2KA(ebda), 1024);

	if (!rsdp)
		rsdp = search_rsdp((uint8_t *) PA2KA(0xe0000), 128 * 1024);

	if (!rsdp)
		return;

	acpi_rsdp = (struct acpi_rsdp *) rsdp;

	LOG("%p: ACPI Root System Description Pointer", acpi_rsdp);

	uintptr_t acpi_rsdt_p = (uintptr_t) acpi_rsdp->rsdt_address;
	uintptr_t acpi_xsdt_p = 0;

	if (acpi_rsdp->revision)
		acpi_xsdt_p = (uintptr_t) acpi_rsdp->xsdt_address;

	if (acpi_rsdt_p)
		acpi_rsdt = (struct acpi_rsdt *) map_sdt(
		    (struct acpi_sdt_header *) acpi_rsdt_p);

	if (acpi_xsdt_p)
		acpi_xsdt = (struct acpi_xsdt *) map_sdt(
		    (struct acpi_sdt_header *) acpi_xsdt_p);

	if ((acpi_rsdt) && (!acpi_sdt_check((uint8_t *) acpi_rsdt))) {
		log(LF_ARCH, LVL_ERROR, "RSDT: bad checksum");
		return;
	}

	if ((acpi_xsdt) && (!acpi_sdt_check((uint8_t *) acpi_xsdt))) {
		log(LF_ARCH, LVL_ERROR, "XSDT: bad checksum");
		return;
	}

	if (acpi_xsdt)
		configure_via_xsdt();
	else if (acpi_rsdt)
		configure_via_rsdt();
}

/** @}
 */
