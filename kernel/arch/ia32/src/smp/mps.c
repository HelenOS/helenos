/*
 * Copyright (c) 2008 Jakub Jermar
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

/** @addtogroup ia32
 * @{
 */
/** @file
 */

#ifdef CONFIG_SMP

#include <config.h>
#include <log.h>
#include <arch/smp/mps.h>
#include <arch/smp/apic.h>
#include <arch/smp/smp.h>
#include <assert.h>
#include <halt.h>
#include <typedefs.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch/bios/bios.h>
#include <mm/frame.h>

/*
 * MultiProcessor Specification detection code.
 */

#define FS_SIGNATURE  UINT32_C(0x5f504d5f)
#define CT_SIGNATURE  UINT32_C(0x504d4350)

static struct mps_fs *fs;
static struct mps_ct *ct;

static struct __processor_entry *processor_entries = NULL;
static struct __bus_entry *bus_entries = NULL;
static struct __io_apic_entry *io_apic_entries = NULL;
static struct __io_intr_entry *io_intr_entries = NULL;
static struct __l_intr_entry *l_intr_entries = NULL;

static size_t io_apic_cnt = 0;

static size_t processor_entry_cnt = 0;
static size_t bus_entry_cnt = 0;
static size_t io_apic_entry_cnt = 0;
static size_t io_intr_entry_cnt = 0;
static size_t l_intr_entry_cnt = 0;

static uint8_t mps_cpu_apic_id(size_t i)
{
	assert(i < processor_entry_cnt);

	return processor_entries[i].l_apic_id;
}

static bool mps_cpu_enabled(size_t i)
{
	assert(i < processor_entry_cnt);

	/*
	 * FIXME: The current local APIC driver limits usable
	 * CPU IDs to 8.
	 *
	 */
	if (i > 7)
		return false;

	return ((processor_entries[i].cpu_flags & 0x01) == 0x01);
}

static bool mps_cpu_bootstrap(size_t i)
{
	assert(i < processor_entry_cnt);

	return ((processor_entries[i].cpu_flags & 0x02) == 0x02);
}

static int mps_irq_to_pin(unsigned int irq)
{
	size_t i;

	for (i = 0; i < io_intr_entry_cnt; i++) {
		if (io_intr_entries[i].src_bus_irq == irq &&
		    io_intr_entries[i].intr_type == 0)
			return io_intr_entries[i].dst_io_apic_pin;
	}

	return -1;
}

/** Implementation of IA-32 SMP configuration interface.
 *
 */
struct smp_config_operations mps_config_operations = {
	.cpu_enabled = mps_cpu_enabled,
	.cpu_bootstrap = mps_cpu_bootstrap,
	.cpu_apic_id = mps_cpu_apic_id,
	.irq_to_pin = mps_irq_to_pin
};

/** Check the integrity of the MP Floating Structure.
 *
 */
static bool mps_fs_check(uint8_t *base)
{
	unsigned int i;
	uint8_t sum;

	for (i = 0, sum = 0; i < 16; i++)
		sum = (uint8_t) (sum + base[i]);

	return (sum == 0);
}

/** Check the integrity of the MP Configuration Table.
 *
 */
static bool mps_ct_check(void)
{
	uint8_t *base = (uint8_t *) ct;
	uint8_t *ext = base + ct->base_table_length;
	uint8_t sum;
	uint16_t i;

	/* Compute the checksum for the base table */
	for (i = 0, sum = 0; i < ct->base_table_length; i++)
		sum = (uint8_t) (sum + base[i]);

	if (sum)
		return false;

	/* Compute the checksum for the extended table */
	for (i = 0, sum = 0; i < ct->ext_table_length; i++)
		sum = (uint8_t) (sum + ext[i]);

	return (sum == ct->ext_table_checksum);
}

static void ct_processor_entry(struct __processor_entry *pr)
{
	/*
	 * Ignore processors which are not marked enabled.
	 */
	if ((pr->cpu_flags & (1 << 0)) == 0)
		return;

	apic_id_mask |= (1 << pr->l_apic_id);
}

static void ct_bus_entry(struct __bus_entry *bus __attribute__((unused)))
{
#ifdef MPSCT_VERBOSE
	char buf[7];

	memcpy((void *) buf, (void *) bus->bus_type, 6);
	buf[6] = 0;

	log(LF_ARCH, LVL_DEBUG, "MPS: bus=%" PRIu8 " (%s)", bus->bus_id, buf);
#endif
}

static void ct_io_apic_entry(struct __io_apic_entry *ioa)
{
	/* This I/O APIC is marked unusable */
	if ((ioa->io_apic_flags & 1) == 0)
		return;

	if (io_apic_cnt++ > 0) {
		/*
		 * Multiple I/O APICs are currently not supported.
		 */
		return;
	}

	io_apic = (uint32_t *) (uintptr_t) ioa->io_apic;
}

static void ct_io_intr_entry(struct __io_intr_entry *iointr
    __attribute__((unused)))
{
#ifdef MPSCT_VERBOSE
	log_begin(LF_ARCH, LVL_DEBUG);
	log_printf("MPS: ");

	switch (iointr->intr_type) {
	case 0:
		log_printf("INT");
		break;
	case 1:
		log_printf("NMI");
		break;
	case 2:
		log_printf("SMI");
		break;
	case 3:
		log_printf("ExtINT");
		break;
	}

	log_printf(", ");

	switch (iointr->poel & 3) {
	case 0:
		log_printf("bus-like");
		break;
	case 1:
		log_printf("active high");
		break;
	case 2:
		log_printf("reserved");
		break;
	case 3:
		log_printf("active low");
		break;
	}

	log_printf(", ");

	switch ((iointr->poel >> 2) & 3) {
	case 0:
		log_printf("bus-like");
		break;
	case 1:
		log_printf("edge-triggered");
		break;
	case 2:
		log_printf("reserved");
		break;
	case 3:
		log_printf("level-triggered");
		break;
	}

	log_printf(", bus=%" PRIu8 " irq=%" PRIu8 " io_apic=%" PRIu8 " pin=%"
	    PRIu8, iointr->src_bus_id, iointr->src_bus_irq,
	    iointr->dst_io_apic_id, iointr->dst_io_apic_pin);
	log_end();
#endif
}

static void ct_l_intr_entry(struct __l_intr_entry *lintr
    __attribute__((unused)))
{
#ifdef MPSCT_VERBOSE
	log_begin(LF_ARCH, LVL_DEBUG);
	log_printf("MPS: ");

	switch (lintr->intr_type) {
	case 0:
		log_printf("INT");
		break;
	case 1:
		log_printf("NMI");
		break;
	case 2:
		log_printf("SMI");
		break;
	case 3:
		log_printf("ExtINT");
		break;
	}

	log_printf(", ");

	switch (lintr->poel & 3) {
	case 0:
		log_printf("bus-like");
		break;
	case 1:
		log_printf("active high");
		break;
	case 2:
		log_printf("reserved");
		break;
	case 3:
		log_printf("active low");
		break;
	}

	log_printf(", ");

	switch ((lintr->poel >> 2) & 3) {
	case 0:
		log_printf("bus-like");
		break;
	case 1:
		log_printf("edge-triggered");
		break;
	case 2:
		log_printf("reserved");
		break;
	case 3:
		log_printf("level-triggered");
		break;
	}

	log_printf(", bus=%" PRIu8 " irq=%" PRIu8 " l_apic=%" PRIu8 " pin=%"
	    PRIu8, lintr->src_bus_id, lintr->src_bus_irq,
	    lintr->dst_l_apic_id, lintr->dst_l_apic_pin);
	log_end();
#endif
}

static void ct_extended_entries(void)
{
	uint8_t *ext = (uint8_t *) ct + ct->base_table_length;
	uint8_t *cur;

	for (cur = ext; cur < ext + ct->ext_table_length;
	    cur += cur[CT_EXT_ENTRY_LEN]) {
		switch (cur[CT_EXT_ENTRY_TYPE]) {
		default:
			log(LF_ARCH, LVL_NOTE, "MPS: Skipping MP Configuration"
			    " Table extended entry type %" PRIu8,
			    cur[CT_EXT_ENTRY_TYPE]);
		}
	}
}

static void configure_via_ct(void)
{
	if (ct->signature != CT_SIGNATURE) {
		log(LF_ARCH, LVL_WARN, "MPS: Wrong ct->signature");
		return;
	}

	if (!mps_ct_check()) {
		log(LF_ARCH, LVL_WARN, "MPS: Wrong ct checksum");
		return;
	}

	if (ct->oem_table) {
		log(LF_ARCH, LVL_WARN, "MPS: ct->oem_table not supported");
		return;
	}

	l_apic = (uint32_t *) (uintptr_t) ct->l_apic;

	uint8_t *cur = &ct->base_table[0];
	uint16_t i;

	for (i = 0; i < ct->entry_count; i++) {
		switch (*cur) {
		case 0:  /* Processor entry */
			processor_entries = processor_entries ?
			    processor_entries :
			    (struct __processor_entry *) cur;
			processor_entry_cnt++;
			ct_processor_entry((struct __processor_entry *) cur);
			cur += 20;
			break;
		case 1:  /* Bus entry */
			bus_entries = bus_entries ?
			    bus_entries : (struct __bus_entry *) cur;
			bus_entry_cnt++;
			ct_bus_entry((struct __bus_entry *) cur);
			cur += 8;
			break;
		case 2:  /* I/O APIC */
			io_apic_entries = io_apic_entries ?
			    io_apic_entries : (struct __io_apic_entry *) cur;
			io_apic_entry_cnt++;
			ct_io_apic_entry((struct __io_apic_entry *) cur);
			cur += 8;
			break;
		case 3:  /* I/O Interrupt Assignment */
			io_intr_entries = io_intr_entries ?
			    io_intr_entries : (struct __io_intr_entry *) cur;
			io_intr_entry_cnt++;
			ct_io_intr_entry((struct __io_intr_entry *) cur);
			cur += 8;
			break;
		case 4:  /* Local Interrupt Assignment */
			l_intr_entries = l_intr_entries ?
			    l_intr_entries : (struct __l_intr_entry *) cur;
			l_intr_entry_cnt++;
			ct_l_intr_entry((struct __l_intr_entry *) cur);
			cur += 8;
			break;
		default:
			/*
			 * Something is wrong. Fallback to UP mode.
			 */
			log(LF_ARCH, LVL_WARN, "MPS: ct badness %" PRIu8, *cur);
			return;
		}
	}

	/*
	 * Process extended entries.
	 */
	ct_extended_entries();
}

static void configure_via_default(uint8_t n __attribute__((unused)))
{
	/*
	 * Not yet implemented.
	 */
	log(LF_ARCH, LVL_WARN, "MPS: Default configuration not supported");
}

void mps_init(void)
{
	uint8_t *addr[2] = { NULL, (uint8_t *) PA2KA(0xf0000) };
	unsigned int i;
	unsigned int j;
	unsigned int length[2] = { 1024, 64 * 1024 };

	/*
	 * Find MP Floating Pointer Structure
	 *  1a. search first 1K of EBDA
	 *  1b. if EBDA is undefined, search last 1K of base memory
	 *  2.  search 64K starting at 0xf0000
	 */

	addr[0] = (uint8_t *) PA2KA(ebda ? ebda : 639 * 1024);
	for (i = 0; i < 2; i++) {
		for (j = 0; j < length[i]; j += 16) {
			if ((*((uint32_t *) &addr[i][j]) ==
			    FS_SIGNATURE) && (mps_fs_check(&addr[i][j]))) {
				fs = (struct mps_fs *) &addr[i][j];
				goto fs_found;
			}
		}
	}

	return;

fs_found:
	log(LF_ARCH, LVL_NOTE, "%p: MPS Floating Pointer Structure", fs);

	if ((fs->config_type == 0) && (fs->configuration_table)) {
		if (fs->mpfib2 >> 7) {
			log(LF_ARCH, LVL_WARN, "MPS: PIC mode not supported\n");
			return;
		}

		ct = (struct mps_ct *) PA2KA((uintptr_t) fs->configuration_table);
		configure_via_ct();
	} else
		configure_via_default(fs->config_type);

	if (processor_entry_cnt > 0)
		config.cpu_count = processor_entry_cnt;
}

#endif /* CONFIG_SMP */

/** @}
 */
