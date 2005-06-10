/*
 * Copyright (C) 2001-2005 Jakub Jermar
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

#ifdef __SMP__

#include <arch/pm.h>
#include <config.h>
#include <print.h>
#include <panic.h>
#include <arch/smp/mps.h>
#include <arch/smp/ap.h>
#include <arch/smp/apic.h>
#include <func.h>
#include <arch/types.h>
#include <typedefs.h>
#include <synch/waitq.h>
#include <time/delay.h>
#include <mm/heap.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <cpu.h>
#include <arch/i8259.h>
#include <arch/asm.h>
#include <arch/bios/bios.h>
#include <arch/acpi/madt.h>

/*
 * MultiProcessor Specification detection code.
 */

#define	FS_SIGNATURE	0x5f504d5f
#define CT_SIGNATURE 	0x504d4350

int mps_fs_check(__u8 *base);
int mps_ct_check(void);

int configure_via_ct(void);
int configure_via_default(__u8 n);

int ct_processor_entry(struct __processor_entry *pr);
void ct_bus_entry(struct __bus_entry *bus);
void ct_io_apic_entry(struct __io_apic_entry *ioa);
void ct_io_intr_entry(struct __io_intr_entry *iointr);
void ct_l_intr_entry(struct __l_intr_entry *lintr);

void ct_extended_entries(void);

static struct mps_fs *fs;
static struct mps_ct *ct;

struct __processor_entry *processor_entries = NULL;
struct __bus_entry *bus_entries = NULL;
struct __io_apic_entry *io_apic_entries = NULL;
struct __io_intr_entry *io_intr_entries = NULL;
struct __l_intr_entry *l_intr_entries = NULL;

int processor_entry_cnt = 0;
int bus_entry_cnt = 0;
int io_apic_entry_cnt = 0;
int io_intr_entry_cnt = 0;
int l_intr_entry_cnt = 0;

waitq_t ap_completion_wq;
waitq_t kmp_completion_wq;

/*
 * Used to check the integrity of the MP Floating Structure.
 */
int mps_fs_check(__u8 *base)
{
	int i;
	__u8 sum;
	
	for (i = 0, sum = 0; i < 16; i++)
		sum += base[i];
	
	return !sum;
}

/*
 * Used to check the integrity of the MP Configuration Table.
 */
int mps_ct_check(void)
{
	__u8 *base = (__u8 *) ct;
	__u8 *ext = base + ct->base_table_length;
	__u8 sum;
	int i;	
	
	/* count the checksum for the base table */
	for (i=0,sum=0; i < ct->base_table_length; i++)
		sum += base[i];
		
	if (sum)
		return 0;
		
	/* count the checksum for the extended table */
	for (i=0,sum=0; i < ct->ext_table_length; i++)
		sum += ext[i];
		
	return sum == ct->ext_table_checksum;
}

void mps_init(void)
{
	__u8 *addr[2] = { NULL, (__u8 *) 0xf0000 };
	int i, j, length[2] = { 1024, 64*1024 };
	

	/*
	 * Find MP Floating Pointer Structure
	 * 1a. search first 1K of EBDA
	 * 1b. if EBDA is undefined, search last 1K of base memory
	 *  2. search 64K starting at 0xf0000
	 */

	addr[0] = (__u8 *) (ebda ? ebda : 639 * 1024);
	for (i = 0; i < 2; i++) {
		for (j = 0; j < length[i]; j += 16) {
			if (*((__u32 *) &addr[i][j]) == FS_SIGNATURE && mps_fs_check(&addr[i][j])) {
				fs = (struct mps_fs *) &addr[i][j];
				goto fs_found;
			}
		}
	}

	return;
	
fs_found:
	printf("%L: MPS Floating Pointer Structure\n", fs);

	frame_not_free((__address) fs);

	if (fs->config_type == 0 && fs->configuration_table) {
		if (fs->mpfib2 >> 7) {
			printf("%s: PIC mode not supported\n", __FUNCTION__);
			return;
		}

		ct = fs->configuration_table;
		frame_not_free((__address) ct);
		config.cpu_count = configure_via_ct();
	} 
	else
		config.cpu_count = configure_via_default(fs->config_type);

	if (config.cpu_count > 1) {
		map_page_to_frame((__address) l_apic, (__address) l_apic, PAGE_NOT_CACHEABLE, 0);
 	}		
	
	
	/*
	 * Must be initialized outside the kmp thread, since it is waited
	 * on before the kmp thread is created.
	 */
	waitq_initialize(&kmp_completion_wq);
	return;
}

int configure_via_ct(void)
{
	__u8 *cur;
	int i, cnt;
		
	if (ct->signature != CT_SIGNATURE) {
		printf("%s: bad ct->signature\n", __FUNCTION__);
		return 1;
	}
	if (!mps_ct_check()) {
		printf("%s: bad ct checksum\n", __FUNCTION__);
		return 1;
	}
	if (ct->oem_table) {
		printf("%s: ct->oem_table not supported\n", __FUNCTION__);
		return 1;
	}
	
	l_apic = ct->l_apic;

	cnt = 0;
	cur = &ct->base_table[0];
	for (i=0; i < ct->entry_count; i++) {
		switch (*cur) {
			/* Processor entry */
			case 0:	
				processor_entries = processor_entries ? processor_entries : (struct __processor_entry *) cur;
				processor_entry_cnt++;
				cnt += ct_processor_entry((struct __processor_entry *) cur);
				cur += 20;
				break;

			/* Bus entry */
			case 1:
				bus_entries = bus_entries ? bus_entries : (struct __bus_entry *) cur;
				bus_entry_cnt++;
				ct_bus_entry((struct __bus_entry *) cur);
				cur += 8;
				break;
				
			/* I/O Apic */
			case 2:
				io_apic_entries = io_apic_entries ? io_apic_entries : (struct __io_apic_entry *) cur;
				io_apic_entry_cnt++;
				ct_io_apic_entry((struct __io_apic_entry *) cur);
				cur += 8;
				break;
				
			/* I/O Interrupt Assignment */
			case 3:
				io_intr_entries = io_intr_entries ? io_intr_entries : (struct __io_intr_entry *) cur;
				io_intr_entry_cnt++;
				ct_io_intr_entry((struct __io_intr_entry *) cur);
				cur += 8;
				break;

			/* Local Interrupt Assignment */
			case 4:
				l_intr_entries = l_intr_entries ? l_intr_entries : (struct __l_intr_entry *) cur;
				l_intr_entry_cnt++;
				ct_l_intr_entry((struct __l_intr_entry *) cur);
		    		cur += 8;
				break;
	    
			default:
				/*
				 * Something is wrong. Fallback to UP mode.
				 */
				 
				printf("%s: ct badness\n", __FUNCTION__);
				return 1;
		}
	}
	
	/*
	 * Process extended entries.
	 */
	ct_extended_entries();
	return cnt;
}

int configure_via_default(__u8 n)
{
	/*
	 * Not yet implemented.
	 */
	printf("%s: not supported\n", __FUNCTION__);
	return 1;
}


int ct_processor_entry(struct __processor_entry *pr)
{
	/*
	 * Ignore processors which are not marked enabled.
	 */
	if ((pr->cpu_flags & (1<<0)) == 0)
		return 0;
	
	apic_id_mask |= (1<<pr->l_apic_id); 
	return 1;
}

void ct_bus_entry(struct __bus_entry *bus)
{
#ifdef MPSCT_VERBOSE
	char buf[7];
	memcopy((__address) bus->bus_type, (__address) buf, 6);
	buf[6] = 0;
	printf("bus%d: %s\n", bus->bus_id, buf);
#endif
}

void ct_io_apic_entry(struct __io_apic_entry *ioa)
{
	static int io_apic_count = 0;

	/* this ioapic is marked unusable */
	if (ioa->io_apic_flags & 1 == 0)
		return;
	
	if (io_apic_count++ > 0) {
		/*
		 * Multiple IO APIC's are currently not supported.
		 */
		return;
	}
	
	map_page_to_frame((__address) ioa->io_apic, (__address) ioa->io_apic, PAGE_NOT_CACHEABLE, 0);
	
	io_apic = ioa->io_apic;
}

//#define MPSCT_VERBOSE
void ct_io_intr_entry(struct __io_intr_entry *iointr)
{
#ifdef MPSCT_VERBOSE
	switch (iointr->intr_type) {
	    case 0: printf("INT"); break;
	    case 1: printf("NMI"); break;
	    case 2: printf("SMI"); break;
	    case 3: printf("ExtINT"); break;
	}
	putchar(',');
	switch (iointr->poel&3) {
	    case 0: printf("bus-like"); break;
	    case 1: printf("active high"); break;
	    case 2: printf("reserved"); break;
	    case 3: printf("active low"); break;
	}
	putchar(',');
	switch ((iointr->poel>>2)&3) {
	    case 0: printf("bus-like"); break;
	    case 1: printf("edge-triggered"); break;
	    case 2: printf("reserved"); break;
	    case 3: printf("level-triggered"); break;
	}
	putchar(',');
	printf("bus%d,irq%d", iointr->src_bus_id, iointr->src_bus_irq);
	putchar(',');
	printf("io_apic%d,pin%d", iointr->dst_io_apic_id, iointr->dst_io_apic_pin);
	putchar('\n');	
#endif
}

void ct_l_intr_entry(struct __l_intr_entry *lintr)
{
#ifdef MPSCT_VERBOSE
	switch (lintr->intr_type) {
	    case 0: printf("INT"); break;
	    case 1: printf("NMI"); break;
	    case 2: printf("SMI"); break;
	    case 3: printf("ExtINT"); break;
	}
	putchar(',');
	switch (lintr->poel&3) {
	    case 0: printf("bus-like"); break;
	    case 1: printf("active high"); break;
	    case 2: printf("reserved"); break;
	    case 3: printf("active low"); break;
	}
	putchar(',');
	switch ((lintr->poel>>2)&3) {
	    case 0: printf("bus-like"); break;
	    case 1: printf("edge-triggered"); break;
	    case 2: printf("reserved"); break;
	    case 3: printf("level-triggered"); break;
	}
	putchar(',');
	printf("bus%d,irq%d", lintr->src_bus_id, lintr->src_bus_irq);
	putchar(',');
	printf("l_apic%d,pin%d", lintr->dst_l_apic_id, lintr->dst_l_apic_pin);
	putchar('\n');
#endif
}

void ct_extended_entries(void)
{
	__u8 *ext = (__u8 *) ct + ct->base_table_length;
	__u8 *cur;

	for (cur = ext; cur < ext + ct->ext_table_length; cur += cur[CT_EXT_ENTRY_LEN]) {
		switch (cur[CT_EXT_ENTRY_TYPE]) {
			default:
				printf("%L: skipping MP Configuration Table extended entry type %d\n", cur, cur[CT_EXT_ENTRY_TYPE]);
				break;
		}
	}
}


/*
 * Kernel thread for bringing up application processors. It becomes clear
 * that we need an arrangement like this (AP's being initialized by a kernel
 * thread), for a thread has its dedicated stack. (The stack used during the
 * BSP initialization (prior the very first call to scheduler()) will be used
 * as an initialization stack for each AP.)
 */
void kmp(void *arg)
{
	struct __processor_entry *pr;
	__address src, dst;
	int i;

	waitq_initialize(&ap_completion_wq);

	/*
	 * Processor entries immediately follow the configuration table header.
	 */
	pr = processor_entries;

	/*
	 * We need to access data in frame 0.
	 * We boldly make use of kernel address space mapping.
	 */

	/*
	 * Set the warm-reset vector to the real-mode address of 4K-aligned ap_boot()
	 */
	*((__u16 *) (PA2KA(0x467+0))) =  ((__address) ap_boot) >> 4;	/* segment */
	*((__u16 *) (PA2KA(0x467+2))) =  0;				/* offset */
	
	/*
	 * Save 0xa to address 0xf of the CMOS RAM.
	 * BIOS will not do the POST after the INIT signal.
	 */
	outb(0x70,0xf);
	outb(0x71,0xa);

	cpu_priority_high();

	pic_disable_irqs(0xffff);
	apic_init();

	for (i = 0; i < processor_entry_cnt; i++) {
		struct descriptor *gdt_new;
	
		/*
		 * Skip processors marked unusable.
		 */
		if (pr[i].cpu_flags & (1<<0) == 0)
			continue;

		/*
		 * The bootstrap processor is already up.
		 */
		if (pr[i].cpu_flags & (1<<1))
			continue;

		if (pr[i].l_apic_id == l_apic_id()) {
			printf("%L: bad processor entry #%d, will not send IPI to myself\n", &pr[i], i);
			continue;
		}
		
		/*
		 * Prepare new GDT for CPU in question.
		 */
		if (!(gdt_new = (struct descriptor *) malloc(GDT_ITEMS*sizeof(struct descriptor))))
			panic("couldn't allocate memory for GDT\n");

		memcopy(gdt, gdt_new, GDT_ITEMS*sizeof(struct descriptor));
		memsetb(&gdt_new[TSS_DES], sizeof(struct descriptor), 0);
		gdtr.base = KA2PA((__address) gdt_new);

		if (l_apic_send_init_ipi(pr[i].l_apic_id)) {
			/*
		         * There may be just one AP being initialized at
			 * the time. After it comes completely up, it is
			 * supposed to wake us up.
		         */
			waitq_sleep(&ap_completion_wq);
			cpu_priority_high();
		}
		else {
			printf("INIT IPI for l_apic%d failed\n", pr[i].l_apic_id);
		}
	}

	/*
	 * Wakeup the kinit thread so that
	 * system initialization can go on.
	 */
	waitq_wakeup(&kmp_completion_wq, WAKEUP_FIRST);
}

int mps_irq_to_pin(int irq)
{
	int i;
	
	for(i=0;i<io_intr_entry_cnt;i++) {
		if (io_intr_entries[i].src_bus_irq == irq && io_intr_entries[i].intr_type == 0)
			return io_intr_entries[i].dst_io_apic_pin;
	}
	
	return -1;
}

#endif /* __SMP__ */
