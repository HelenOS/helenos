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

#include <arch/types.h>
#include <typedefs.h>
#include <arch/acpi/acpi.h>
#include <arch/acpi/madt.h>
#include <arch/smp/apic.h>
#include <arch/smp/smp.h>
#include <mm/page.h>
#include <panic.h>
#include <debug.h>
#include <config.h>

struct acpi_madt *acpi_madt = NULL;

#ifdef __SMP__

/*
 * NOTE: it is currently not completely clear to the authors of SPARTAN whether
 * MADT can exist in such a form that entries of the same type are not consecutive.
 * Because of this uncertainity, some entry types are explicitly checked for
 * being consecutive with other entries of the same kind. 
 */

static void madt_l_apic_entry(struct madt_l_apic *la, __u8 prev_type);
static void madt_io_apic_entry(struct madt_io_apic *ioa, __u8 prev_type);

struct madt_l_apic *madt_l_apic_entries = NULL;
struct madt_io_apic *madt_io_apic_entries = NULL;

int madt_l_apic_entry_cnt = 0;
int madt_io_apic_entry_cnt = 0;

char *entry[] = {
	"L_APIC",
	"IO_APIC",
	"INTR_SRC_OVRD",
	"NMI_SRC",
	"L_APIC_NMI",
	"L_APIC_ADDR_OVRD",
	"IO_SAPIC",
	"L_SAPIC",
	"PLATFORM_INTR_SRC"
};

/*
 * ACPI MADT Implementation of SMP configuration interface.
 */
static count_t madt_cpu_count(void);
static bool madt_cpu_enabled(index_t i);
static bool madt_cpu_bootstrap(index_t i);
static __u8 madt_cpu_apic_id(index_t i);

struct smp_config_operations madt_config_operations = {
	.cpu_count = madt_cpu_count,
	.cpu_enabled = madt_cpu_enabled,
	.cpu_bootstrap = madt_cpu_bootstrap,
	.cpu_apic_id = madt_cpu_apic_id
};

static count_t madt_cpu_count(void)
{
	return madt_l_apic_entry_cnt;
}

static bool madt_cpu_enabled(index_t i)
{
	ASSERT(i < madt_l_apic_entry_cnt);
	return madt_l_apic_entries[i].flags & 0x1;
}

static bool madt_cpu_bootstrap(index_t i)
{
	ASSERT(i < madt_l_apic_entry_cnt);
	return madt_l_apic_entries[i].apic_id == l_apic_id();
}

static __u8 madt_cpu_apic_id(index_t i)
{
	ASSERT(i < madt_l_apic_entry_cnt);
	return madt_l_apic_entries[i].apic_id;
}

void acpi_madt_parse(void)
{
	struct madt_apic_header *end = (struct madt_apic_header *) (((__u8 *) acpi_madt) + acpi_madt->header.length);
	struct madt_apic_header *h = &acpi_madt->apic_header[0];
	__u8 prev_type = 0; /* used to detect incosecutive entries */


	l_apic = (__u32 *) acpi_madt->l_apic_address;

	while (h < end) {
		switch (h->type) {
			case MADT_L_APIC:
				madt_l_apic_entry((struct madt_l_apic *) h, prev_type);
				break;
			case MADT_IO_APIC:
				madt_io_apic_entry((struct madt_io_apic *) h, prev_type);
				break;
			case MADT_INTR_SRC_OVRD:
			case MADT_NMI_SRC:
			case MADT_L_APIC_NMI:
			case MADT_L_APIC_ADDR_OVRD:
			case MADT_IO_SAPIC:
			case MADT_L_SAPIC:
			case MADT_PLATFORM_INTR_SRC:
				printf("MADT: skipping %s entry (type=%d)\n", entry[h->type], h->type);
				break;

			default:
				if (h->type >= MADT_RESERVED_SKIP_BEGIN && h->type <= MADT_RESERVED_SKIP_END) {
					printf("MADT: skipping reserved entry (type=%d)\n", h->type);
				}
				if (h->type >= MADT_RESERVED_OEM_BEGIN) {
					printf("MADT: skipping OEM entry (type=%d)\n", h->type);
				}
				break;
		}
		prev_type = h->type;
		h = (struct madt_apic_header *) (((__u8 *) h) + h->length);
	}

	if (madt_l_apic_entry_cnt)
		config.cpu_count = madt_l_apic_entry_cnt;
}
 
void madt_l_apic_entry(struct madt_l_apic *la, __u8 prev_type)
{
	/* check for consecutiveness */
	if (madt_l_apic_entry_cnt && prev_type != MADT_L_APIC)
	panic("%s entries are not consecuitve\n", entry[MADT_L_APIC]);

	if (!madt_l_apic_entry_cnt++)
		madt_l_apic_entries = la;
		
	if (!(la->flags & 0x1)) {
		/* Processor is unusable, skip it. */
		return;
	}
		
	apic_id_mask |= 1<<la->apic_id;
}

void madt_io_apic_entry(struct madt_io_apic *ioa, __u8 prev_type)
{
	/* check for consecutiveness */
	if (madt_io_apic_entry_cnt && prev_type != MADT_IO_APIC)
	panic("%s entries are not consecuitve\n", entry[MADT_IO_APIC]);

	if (!madt_io_apic_entry_cnt++) {
		madt_io_apic_entries = ioa;
		io_apic = (__u32 *) ioa->io_apic_address;
		map_page_to_frame((__address) io_apic, (__address) io_apic, PAGE_NOT_CACHEABLE, 0);
	}
	else {
		/* currently not supported */
		return;
	}
}


#endif /* __SMP__ */
