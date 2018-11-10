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

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Multiple APIC Description Table (MADT) parsing.
 */

#include <assert.h>
#include <typedefs.h>
#include <genarch/acpi/acpi.h>
#include <genarch/acpi/madt.h>
#include <arch/smp/apic.h>
#include <arch/smp/smp.h>
#include <panic.h>
#include <config.h>
#include <log.h>
#include <stdlib.h>
#include <gsort.h>

struct acpi_madt *acpi_madt = NULL;

#ifdef CONFIG_SMP

/**
 * Standard ISA IRQ map; can be overriden by
 * Interrupt Source Override entries of MADT.
 */
static int isa_irq_map[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

struct madt_l_apic *madt_l_apic_entries = NULL;
struct madt_io_apic *madt_io_apic_entries = NULL;

static size_t madt_l_apic_entry_index = 0;
static size_t madt_io_apic_entry_index = 0;
static size_t madt_l_apic_entry_cnt = 0;
static size_t madt_io_apic_entry_cnt = 0;

static struct madt_apic_header **madt_entries_index = NULL;

const char *entry[] = {
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

static uint8_t madt_cpu_apic_id(size_t i)
{
	assert(i < madt_l_apic_entry_cnt);

	return ((struct madt_l_apic *)
	    madt_entries_index[madt_l_apic_entry_index + i])->apic_id;
}

static bool madt_cpu_enabled(size_t i)
{
	assert(i < madt_l_apic_entry_cnt);

	/*
	 * FIXME: The current local APIC driver limits usable
	 * CPU IDs to 8.
	 *
	 */
	if (i > 7)
		return false;

	return ((struct madt_l_apic *)
	    madt_entries_index[madt_l_apic_entry_index + i])->flags & 0x1;
}

static bool madt_cpu_bootstrap(size_t i)
{
	assert(i < madt_l_apic_entry_cnt);

	return ((struct madt_l_apic *)
	    madt_entries_index[madt_l_apic_entry_index + i])->apic_id ==
	    bsp_l_apic;
}

static int madt_irq_to_pin(unsigned int irq)
{
	if (irq >= sizeof(isa_irq_map) / sizeof(int))
		return (int) irq;

	return isa_irq_map[irq];
}

/** ACPI MADT Implementation of SMP configuration interface.
 *
 */
struct smp_config_operations madt_config_operations = {
	.cpu_enabled = madt_cpu_enabled,
	.cpu_bootstrap = madt_cpu_bootstrap,
	.cpu_apic_id = madt_cpu_apic_id,
	.irq_to_pin = madt_irq_to_pin
};

static int madt_cmp(void *a, void *b, void *arg)
{
	uint8_t typea = (*((struct madt_apic_header **) a))->type;
	uint8_t typeb = (*((struct madt_apic_header **) b))->type;

	if (typea > typeb)
		return 1;

	if (typea < typeb)
		return -1;

	return 0;
}

static void madt_l_apic_entry(struct madt_l_apic *la, size_t i)
{
	if (madt_l_apic_entry_cnt == 0)
		madt_l_apic_entry_index = i;

	madt_l_apic_entry_cnt++;

	if (!(la->flags & 0x1)) {
		/* Processor is unusable, skip it. */
		return;
	}

	apic_id_mask |= 1 << la->apic_id;
}

static void madt_io_apic_entry(struct madt_io_apic *ioa, size_t i)
{
	if (madt_io_apic_entry_cnt == 0) {
		/* Remember index of the first io apic entry */
		madt_io_apic_entry_index = i;
		io_apic = (uint32_t *) (sysarg_t) ioa->io_apic_address;
	} else {
		/* Currently not supported */
	}

	madt_io_apic_entry_cnt++;
}

static void madt_intr_src_ovrd_entry(struct madt_intr_src_ovrd *override,
    size_t i)
{
	assert(override->source < sizeof(isa_irq_map) / sizeof(int));

	isa_irq_map[override->source] = override->global_int;
}

void acpi_madt_parse(void)
{
	struct madt_apic_header *end = (struct madt_apic_header *)
	    (((uint8_t *) acpi_madt) + acpi_madt->header.length);
	struct madt_apic_header *hdr;

	l_apic = (uint32_t *) (sysarg_t) acpi_madt->l_apic_address;

	/* Count MADT entries */
	unsigned int madt_entries_index_cnt = 0;
	for (hdr = acpi_madt->apic_header; hdr < end;
	    hdr = (struct madt_apic_header *) (((uint8_t *) hdr) + hdr->length))
		madt_entries_index_cnt++;

	/* Create MADT APIC entries index array */
	madt_entries_index = (struct madt_apic_header **)
	    malloc(madt_entries_index_cnt * sizeof(struct madt_apic_header *));
	if (!madt_entries_index)
		panic("Memory allocation error.");

	size_t i = 0;

	for (hdr = acpi_madt->apic_header; hdr < end;
	    hdr = (struct madt_apic_header *) (((uint8_t *) hdr) + hdr->length)) {
		madt_entries_index[i] = hdr;
		i++;
	}

	/* Sort MADT index structure */
	if (!gsort(madt_entries_index, madt_entries_index_cnt,
	    sizeof(struct madt_apic_header *), madt_cmp, NULL))
		panic("Sorting error.");

	/* Parse MADT entries */
	for (i = 0; i < madt_entries_index_cnt; i++) {
		hdr = madt_entries_index[i];

		switch (hdr->type) {
		case MADT_L_APIC:
			madt_l_apic_entry((struct madt_l_apic *) hdr, i);
			break;
		case MADT_IO_APIC:
			madt_io_apic_entry((struct madt_io_apic *) hdr, i);
			break;
		case MADT_INTR_SRC_OVRD:
			madt_intr_src_ovrd_entry((struct madt_intr_src_ovrd *) hdr, i);
			break;
		case MADT_NMI_SRC:
		case MADT_L_APIC_NMI:
		case MADT_L_APIC_ADDR_OVRD:
		case MADT_IO_SAPIC:
		case MADT_L_SAPIC:
		case MADT_PLATFORM_INTR_SRC:
			log(LF_ARCH, LVL_WARN,
			    "MADT: Skipping %s entry (type=%" PRIu8 ")",
			    entry[hdr->type], hdr->type);
			break;
		default:
			if ((hdr->type >= MADT_RESERVED_SKIP_BEGIN) &&
			    (hdr->type <= MADT_RESERVED_SKIP_END))
				log(LF_ARCH, LVL_NOTE,
				    "MADT: Skipping reserved entry (type=%" PRIu8 ")",
				    hdr->type);

			if (hdr->type >= MADT_RESERVED_OEM_BEGIN)
				log(LF_ARCH, LVL_NOTE,
				    "MADT: Skipping OEM entry (type=%" PRIu8 ")",
				    hdr->type);

			break;
		}
	}

	if (madt_l_apic_entry_cnt > 0)
		config.cpu_count = madt_l_apic_entry_cnt;
}

#endif /* CONFIG_SMP */

/** @}
 */
