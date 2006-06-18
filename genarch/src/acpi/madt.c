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

 /** @addtogroup genarch	
 * @{
 */
/**
 * @file
 * @brief	Multiple APIC Description Table (MADT) parsing.
 */

#include <arch/types.h>
#include <typedefs.h>
#include <genarch/acpi/acpi.h>
#include <genarch/acpi/madt.h>
#include <arch/smp/apic.h>
#include <arch/smp/smp.h>
#include <panic.h>
#include <debug.h>
#include <config.h>
#include <print.h>
#include <mm/slab.h>
#include <memstr.h>
#include <sort.h>

struct acpi_madt *acpi_madt = NULL;

#ifdef CONFIG_SMP

/** Standard ISA IRQ map; can be overriden by Interrupt Source Override entries of MADT. */
int isa_irq_map[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

static void madt_l_apic_entry(struct madt_l_apic *la, __u32 index);
static void madt_io_apic_entry(struct madt_io_apic *ioa, __u32 index);
static void madt_intr_src_ovrd_entry(struct madt_intr_src_ovrd *override, __u32 index);
static int madt_cmp(void * a, void * b);

struct madt_l_apic *madt_l_apic_entries = NULL;
struct madt_io_apic *madt_io_apic_entries = NULL;

index_t madt_l_apic_entry_index = 0;
index_t madt_io_apic_entry_index = 0;
count_t madt_l_apic_entry_cnt = 0;
count_t madt_io_apic_entry_cnt = 0;
count_t cpu_count = 0;

struct madt_apic_header * * madt_entries_index = NULL;
int madt_entries_index_cnt = 0;

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
static int madt_irq_to_pin(int irq);

struct smp_config_operations madt_config_operations = {
	.cpu_count = madt_cpu_count,
	.cpu_enabled = madt_cpu_enabled,
	.cpu_bootstrap = madt_cpu_bootstrap,
	.cpu_apic_id = madt_cpu_apic_id,
	.irq_to_pin = madt_irq_to_pin
};

count_t madt_cpu_count(void)
{
	return madt_l_apic_entry_cnt;
}

bool madt_cpu_enabled(index_t i)
{
	ASSERT(i < madt_l_apic_entry_cnt);
	return ((struct madt_l_apic *) madt_entries_index[madt_l_apic_entry_index + i])->flags & 0x1;

}

bool madt_cpu_bootstrap(index_t i)
{
	ASSERT(i < madt_l_apic_entry_cnt);
	return ((struct madt_l_apic *) madt_entries_index[madt_l_apic_entry_index + i])->apic_id == l_apic_id();
}

__u8 madt_cpu_apic_id(index_t i)
{
	ASSERT(i < madt_l_apic_entry_cnt);
	return ((struct madt_l_apic *) madt_entries_index[madt_l_apic_entry_index + i])->apic_id;
}

int madt_irq_to_pin(int irq)
{
	ASSERT(irq < sizeof(isa_irq_map)/sizeof(int));
        return isa_irq_map[irq];
}

int madt_cmp(void * a, void * b) 
{
	return 
		(((struct madt_apic_header *) a)->type > ((struct madt_apic_header *) b)->type) ?
		1 : 
		((((struct madt_apic_header *) a)->type < ((struct madt_apic_header *) b)->type) ? -1 : 0);
}
	
void acpi_madt_parse(void)
{
	struct madt_apic_header *end = (struct madt_apic_header *) (((__u8 *) acpi_madt) + acpi_madt->header.length);
	struct madt_apic_header *h;
	
        l_apic = (__u32 *) (__native) acpi_madt->l_apic_address;

	/* calculate madt entries */
	for (h = &acpi_madt->apic_header[0]; h < end; h = (struct madt_apic_header *) (((__u8 *) h) + h->length)) {
		madt_entries_index_cnt++;
	}

	/* create madt apic entries index array */
	madt_entries_index = (struct madt_apic_header * *) malloc(madt_entries_index_cnt * sizeof(struct madt_apic_header * *), FRAME_ATOMIC);
	if (!madt_entries_index)
		panic("Memory allocation error.");

	__u32 index = 0;

	for (h = &acpi_madt->apic_header[0]; h < end; h = (struct madt_apic_header *) (((__u8 *) h) + h->length)) {
		madt_entries_index[index++] = h;
	}

	/* Quicksort MADT index structure */
	qsort(madt_entries_index, madt_entries_index_cnt, sizeof(__address), &madt_cmp);

	/* Parse MADT entries */	
	for (index = 0; index < madt_entries_index_cnt - 1; index++) {
		h = madt_entries_index[index];
		switch (h->type) {
			case MADT_L_APIC:
				madt_l_apic_entry((struct madt_l_apic *) h, index);
				break;
			case MADT_IO_APIC:
				madt_io_apic_entry((struct madt_io_apic *) h, index);
				break;
			case MADT_INTR_SRC_OVRD:
				madt_intr_src_ovrd_entry((struct madt_intr_src_ovrd *) h, index);
				break;
			case MADT_NMI_SRC:
			case MADT_L_APIC_NMI:
			case MADT_L_APIC_ADDR_OVRD:
			case MADT_IO_SAPIC:
			case MADT_L_SAPIC:
			case MADT_PLATFORM_INTR_SRC:
				printf("MADT: skipping %s entry (type=%zd)\n", entry[h->type], h->type);
				break;

			default:
				if (h->type >= MADT_RESERVED_SKIP_BEGIN && h->type <= MADT_RESERVED_SKIP_END) {
					printf("MADT: skipping reserved entry (type=%zd)\n", h->type);
				}
				if (h->type >= MADT_RESERVED_OEM_BEGIN) {
					printf("MADT: skipping OEM entry (type=%zd)\n", h->type);
				}
				break;
		}
	
	
	}
	

	if (cpu_count)
		config.cpu_count = cpu_count;
}
 

void madt_l_apic_entry(struct madt_l_apic *la, __u32 index)
{
	if (!madt_l_apic_entry_cnt++) {
		madt_l_apic_entry_index = index;
	}
		
	if (!(la->flags & 0x1)) {
		/* Processor is unusable, skip it. */
		return;
	}
	
	cpu_count++;	
	apic_id_mask |= 1<<la->apic_id;
}

void madt_io_apic_entry(struct madt_io_apic *ioa, __u32 index)
{
	if (!madt_io_apic_entry_cnt++) {
		/* remember index of the first io apic entry */
		madt_io_apic_entry_index = index;
		io_apic = (__u32 *) (__native) ioa->io_apic_address;
	} else {
		/* currently not supported */
		return;
	}
}

void madt_intr_src_ovrd_entry(struct madt_intr_src_ovrd *override, __u32 index)
{
	ASSERT(override->source < sizeof(isa_irq_map)/sizeof(int));
	printf("MADT: ignoring %s entry: bus=%zd, source=%zd, global_int=%zd, flags=%#hx\n",
		entry[override->header.type], override->bus, override->source,
		override->global_int, override->flags);
}

#endif /* CONFIG_SMP */

 /** @}
 */

