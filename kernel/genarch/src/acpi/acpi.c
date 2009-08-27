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

/**
 * @file
 * @brief	Advanced Configuration and Power Interface (ACPI) initialization.
 */
 
#include <genarch/acpi/acpi.h>
#include <genarch/acpi/madt.h>
#include <arch/bios/bios.h>
#include <mm/as.h>
#include <mm/page.h>
#include <print.h>

#define RSDP_SIGNATURE		"RSD PTR "
#define RSDP_REVISION_OFFS	15

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

static int rsdp_check(uint8_t *rsdp) {
	struct acpi_rsdp *r = (struct acpi_rsdp *) rsdp;
	uint8_t sum = 0;
	unsigned int i;
	
	for (i = 0; i < 20; i++)
		sum = (uint8_t) (sum + rsdp[i]);
		
	if (sum)	
		return 0; /* bad checksum */

	if (r->revision == 0)
		return 1; /* ACPI 1.0 */
		
	for (; i < r->length; i++)
		sum = (uint8_t) (sum + rsdp[i]);
		
	return !sum;
	
}

int acpi_sdt_check(uint8_t *sdt)
{
	struct acpi_sdt_header *h = (struct acpi_sdt_header *) sdt;
	uint8_t sum = 0;
	unsigned int i;

	for (i = 0; i < h->length; i++)
		sum = (uint8_t) (sum + sdt[i]);
		
	return !sum;
}

static void map_sdt(struct acpi_sdt_header *sdt)
{
	page_mapping_insert(AS_KERNEL, (uintptr_t) sdt, (uintptr_t) sdt, PAGE_NOT_CACHEABLE | PAGE_WRITE);
	map_structure((uintptr_t) sdt, sdt->length);
}

static void configure_via_rsdt(void)
{
	unsigned int i, j, cnt = (acpi_rsdt->header.length - sizeof(struct acpi_sdt_header)) / sizeof(uint32_t);
	
	for (i = 0; i < cnt; i++) {
		for (j = 0; j < sizeof(signature_map) / sizeof(struct acpi_signature_map); j++) {
			struct acpi_sdt_header *h = (struct acpi_sdt_header *) (unative_t) acpi_rsdt->entry[i];
		
			map_sdt(h);
			if (CMP_SIGNATURE(h->signature, signature_map[j].signature)) {
				if (!acpi_sdt_check((uint8_t *) h))
					goto next;
				*signature_map[j].sdt_ptr = h;
				LOG("%p: ACPI %s\n", *signature_map[j].sdt_ptr, signature_map[j].description);
			}
		}
next:
		;
	}
}

static void configure_via_xsdt(void)
{
	unsigned int i, j, cnt = (acpi_xsdt->header.length - sizeof(struct acpi_sdt_header)) / sizeof(uint64_t);
	
	for (i = 0; i < cnt; i++) {
		for (j = 0; j < sizeof(signature_map) / sizeof(struct acpi_signature_map); j++) {
			struct acpi_sdt_header *h = (struct acpi_sdt_header *) ((uintptr_t) acpi_rsdt->entry[i]);

			map_sdt(h);
			if (CMP_SIGNATURE(h->signature, signature_map[j].signature)) {
				if (!acpi_sdt_check((uint8_t *) h))
					goto next;
				*signature_map[j].sdt_ptr = h;
				LOG("%p: ACPI %s\n", *signature_map[j].sdt_ptr, signature_map[j].description);
			}
		}
next:
		;
	}

}

void acpi_init(void)
{
	uint8_t *addr[2] = { NULL, (uint8_t *) PA2KA(0xe0000) };
	int i, j, length[2] = { 1024, 128*1024 };
	uint64_t *sig = (uint64_t *) RSDP_SIGNATURE;

	/*
	 * Find Root System Description Pointer
	 * 1. search first 1K of EBDA
	 * 2. search 128K starting at 0xe0000
	 */

	addr[0] = (uint8_t *) PA2KA(ebda);
	for (i = (ebda ? 0 : 1); i < 2; i++) {
		for (j = 0; j < length[i]; j += 16) {
			if (*((uint64_t *) &addr[i][j]) == *sig && rsdp_check(&addr[i][j])) {
				acpi_rsdp = (struct acpi_rsdp *) &addr[i][j];
				goto rsdp_found;
			}
		}
	}

	return;

rsdp_found:
	LOG("%p: ACPI Root System Description Pointer\n", acpi_rsdp);

	acpi_rsdt = (struct acpi_rsdt *) (unative_t) acpi_rsdp->rsdt_address;
	if (acpi_rsdp->revision)
		acpi_xsdt = (struct acpi_xsdt *) ((uintptr_t) acpi_rsdp->xsdt_address);

	if (acpi_rsdt)
		map_sdt((struct acpi_sdt_header *) acpi_rsdt);
	if (acpi_xsdt)
		map_sdt((struct acpi_sdt_header *) acpi_xsdt);

	if (acpi_rsdt && !acpi_sdt_check((uint8_t *) acpi_rsdt)) {
		printf("RSDT: bad checksum\n");
		return;
	}
	if (acpi_xsdt && !acpi_sdt_check((uint8_t *) acpi_xsdt)) {
		printf("XSDT: bad checksum\n");
		return;
	}

	if (acpi_xsdt)
		configure_via_xsdt();
	else if (acpi_rsdt)
		configure_via_rsdt();

}

/** @}
 */
