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

#include <arch/acpi/acpi.h>
#include <arch/acpi/madt.h>
#include <arch/bios/bios.h>

#include <mm/page.h>
#include <print.h>

#define RSDP_SIGNATURE		"RSD PTR "
#define RSDP_REVISION_OFFS	15

struct acpi_rsdp *acpi_rsdp = NULL;
struct acpi_rsdt *acpi_rsdt = NULL;
struct acpi_xsdt *acpi_xsdt = NULL;

struct acpi_signature_map signature_map[] = { 
	{ (__u8 *)"APIC", (struct acpi_sdt_header **) &acpi_madt, "Multiple APIC Description Table" }
};

int rsdp_check(__u8 *rsdp) {
	struct acpi_rsdp *r = (struct acpi_rsdp *) rsdp;
	__u8 sum = 0;
	int i;
	
	for (i=0; i<20; i++)
		sum += rsdp[i];
		
	if (sum)	
		return 0; /* bad checksum */

	if (r->revision == 0)
		return 1; /* ACPI 1.0 */
		
	for (; i<r->length; i++)
		sum += rsdp[i];
		
	return !sum;
	
}

int acpi_sdt_check(__u8 *sdt)
{
	struct acpi_sdt_header *h = (struct acpi_sdt_header *) sdt;
	__u8 sum = 0;
	int i;

	for (i=0; i<h->length; i++)
		sum += sdt[i];
		
	return !sum;
}

void map_sdt(struct acpi_sdt_header *sdt)
{
	map_page_to_frame((__address) sdt, (__address) sdt, PAGE_NOT_CACHEABLE, 0);
	map_structure((__address) sdt, sdt->length);
}

void acpi_init(void)
{
	__u8 *addr[2] = { NULL, (__u8 *) PA2KA(0xe0000) };
	int i, j, length[2] = { 1024, 128*1024 };
	__u64 *sig = (__u64 *) RSDP_SIGNATURE;

	/*
	 * Find Root System Description Pointer
	 * 1. search first 1K of EBDA
	 * 2. search 128K starting at 0xe0000
	 */

	addr[0] = (__u8 *) PA2KA(ebda);
	for (i = (ebda ? 0 : 1); i < 2; i++) {
		for (j = 0; j < length[i]; j += 16) {
			if (*((__u64 *) &addr[i][j]) == *sig && rsdp_check(&addr[i][j])) {
				acpi_rsdp = (struct acpi_rsdp *) &addr[i][j];
				goto rsdp_found;
			}
		}
	}

	return;

rsdp_found:
	printf("%L: ACPI Root System Description Pointer\n", acpi_rsdp);

	acpi_rsdt = (struct acpi_rsdt *) (__native) acpi_rsdp->rsdt_address;
	if (acpi_rsdp->revision) acpi_xsdt = (struct acpi_xsdt *) ((__address) acpi_rsdp->xsdt_address);

	if (acpi_rsdt) map_sdt((struct acpi_sdt_header *) acpi_rsdt);
	if (acpi_xsdt) map_sdt((struct acpi_sdt_header *) acpi_xsdt);	

	if (acpi_rsdt && !acpi_sdt_check((__u8 *) acpi_rsdt)) {
		printf("RSDT: %s\n", "bad checksum");
		return;
	}
	if (acpi_xsdt && !acpi_sdt_check((__u8 *) acpi_xsdt)) {
		printf("XSDT: %s\n", "bad checksum");
		return;
	}

	if (acpi_xsdt) configure_via_xsdt();
	else if (acpi_rsdt) configure_via_rsdt();

}

void configure_via_rsdt(void)
{
	int i, j, cnt = (acpi_rsdt->header.length - sizeof(struct acpi_sdt_header))/sizeof(__u32);
	
	for (i=0; i<cnt; i++) {
		for (j=0; j<sizeof(signature_map)/sizeof(struct acpi_signature_map); j++) {
			struct acpi_sdt_header *h = (struct acpi_sdt_header *) (__native) acpi_rsdt->entry[i];
		
			map_sdt(h);	
			if (*((__u32 *) &h->signature[0])==*((__u32 *) &signature_map[j].signature[0])) {
				if (!acpi_sdt_check((__u8 *) h))
					goto next;
				*signature_map[j].sdt_ptr = h;
				printf("%L: ACPI %s\n", *signature_map[j].sdt_ptr, signature_map[j].description);
			}
		}
next:
		;
	}
}

void configure_via_xsdt(void)
{
	int i, j, cnt = (acpi_xsdt->header.length - sizeof(struct acpi_sdt_header))/sizeof(__u64);
	
	for (i=0; i<cnt; i++) {
		for (j=0; j<sizeof(signature_map)/sizeof(struct acpi_signature_map); j++) {
			struct acpi_sdt_header *h = (struct acpi_sdt_header *) ((__address) acpi_rsdt->entry[i]);

			map_sdt(h);
			if (*((__u32 *) &h->signature[0])==*((__u32 *) &signature_map[j].signature[0])) {
				if (!acpi_sdt_check((__u8 *) h))
					goto next;
				*signature_map[j].sdt_ptr = h;
				printf("%L: ACPI %s\n", *signature_map[j].sdt_ptr, signature_map[j].description);
			}
		}
next:
		;
	}

}
