/*
 * Copyright (C) 2001-2004 Jakub Jermar
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
#include <arch/smp/mp.h>
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

/*
 * Multi-Processor Specification detection code.
 */

#define	FS_SIGNATURE	0x5f504d5f
#define CT_SIGNATURE 	0x504d4350

int mp_fs_check(__u8 *base);
int mp_ct_check(void);

int configure_via_ct(void);
int configure_via_default(__u8 n);

int ct_processor_entry(struct __processor_entry *pr);
void ct_bus_entry(struct __bus_entry *bus);
void ct_io_apic_entry(struct __io_apic_entry *ioa);
void ct_io_intr_entry(struct __io_intr_entry *iointr);
void ct_l_intr_entry(struct __l_intr_entry *lintr);

void ct_extended_entries(void);

static struct __mpfs *fs;
static struct __mpct *ct;

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
int mp_fs_check(__u8 *base)
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
int mp_ct_check(void)
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
		
	return !sum;
}

void mp_init(void)
{
	__address addr, frame;
	int cnt, n;
	

	/*
	 * First place to search the MP Floating Pointer Structure is the Extended
	 * BIOS Data Area. We have to read EBDA segment address from the BIOS Data
	 * Area. Unfortunatelly, this memory is in page 0, which has intentionally no
	 * mapping.
	 */
	frame = frame_alloc(FRAME_KA);
	map_page_to_frame(frame,0,PAGE_CACHEABLE,0);
	addr = *((__u16 *) (frame + 0x40e)) * 16;
	map_page_to_frame(frame,frame,PAGE_CACHEABLE,0);
	frame_free(frame);	

	/*
	 * EBDA can be undefined. In that case addr would be 0. 
	 */
	if (addr >= 0x1000) {
		cnt = 1024;
		while (addr = __u32_search(addr,cnt,FS_SIGNATURE)) {
			if (mp_fs_check((__u8 *) addr))
				goto fs_found;
			addr++;
			cnt--;
		}
	}
	
	/*
	 * Second place where the MP Floating Pointer Structure may live is the last
	 * kilobyte of base memory.
	 */
	addr = 639*1024;
	cnt = 1024;
	while (addr = __u32_search(addr,cnt,FS_SIGNATURE)) {
		if (mp_fs_check((__u8 *) addr))
			goto fs_found;
		addr++;
		cnt--;
	}

	/*
	 * As the last resort, MP Floating Pointer Structure is searched in the BIOS
	 * ROM.
	 */
	addr = 0xf0000;
	cnt = 64*1024;
	while (addr = __u32_search(addr,cnt,FS_SIGNATURE)) {
		if (mp_fs_check((__u8 *) addr))
			goto fs_found;
		addr++;
		cnt--;
	}

	return;
	
fs_found:
	printf("%L: MP Floating Pointer Structure\n", addr);

	fs = (struct __mpfs *) addr;
	frame_not_free((__address) fs);
	
	if (fs->config_type == 0 && fs->configuration_table) {
		if (fs->mpfib2 >> 7) {
			printf("mp_init: PIC mode not supported\n");
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
		printf("configure_via_ct: bad ct->signature\n");
		return 1;
	}
	if (!mp_ct_check()) {
		printf("configure_via_ct: bad ct checksum\n");
		return 1;
	}
	if (ct->oem_table) {
		printf("configure_via_ct: ct->oem_table not supported\n");
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
				 
				printf("configure_via_ct: ct badness\n");
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
	printf("configure_via_default: not supported\n");
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
#ifdef MPCT_VERBOSE
	char buf[7];
	memcopy((__address) bus->bus_type, (__address) buf,6);
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

//#define MPCT_VERBOSE
void ct_io_intr_entry(struct __io_intr_entry *iointr)
{
#ifdef MPCT_VERBOSE
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
#ifdef MPCT_VERBOSE
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
	/*
	 * Not yet implemented.
	 */
	if (ct->ext_table_length)
		panic("ct_extended_entries: not supported\n");
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
	__address frame;
	int i;

	waitq_initialize(&ap_completion_wq);

	/*
	 * Processor entries immediately follow the configuration table header.
	 */
	pr = processor_entries;

	/*
	 * Grab a frame and map its address to page 0. This is a hack which
	 * accesses data in frame 0. Note that page 0 is not present because
	 * of nil reference bug catching.
	 */
	frame = frame_alloc(FRAME_KA);
	map_page_to_frame(frame,0,PAGE_CACHEABLE,0);

	/*
	 * Set the warm-reset vector to the real-mode address of 4K-aligned ap_boot()
	 */
	*((__u16 *) (frame + 0x467+0)) =  ((__address) ap_boot) >> 4;	/* segment */
	*((__u16 *) (frame + 0x467+2)) =  0x0;	/* offset */
	
	/*
	 * Give back the borrowed frame and restore identity mapping for it.
	 */
	map_page_to_frame(frame,frame,PAGE_CACHEABLE,0);
	frame_free(frame);

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
		
		/*
		 * Prepare new GDT for CPU in question.
		 */
		if (!(gdt_new = (struct descriptor *) malloc(GDT_ITEMS*sizeof(struct descriptor))))
			panic(PANIC "couldn't allocate memory for GDT\n");

		memcopy(gdt, gdt_new, GDT_ITEMS*sizeof(struct descriptor));
		gdtr.base = (__address) gdt_new;
		
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

int mp_irq_to_pin(int irq)
{
	int i;
	
	for(i=0;i<io_intr_entry_cnt;i++) {
		if (io_intr_entries[i].src_bus_irq == irq && io_intr_entries[i].intr_type == 0)
			return io_intr_entries[i].dst_io_apic_pin;
	}
	
	return -1;
}

#endif /* __SMP__ */
