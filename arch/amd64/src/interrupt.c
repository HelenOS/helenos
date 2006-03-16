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

#include <arch/interrupt.h>
#include <print.h>
#include <debug.h>
#include <panic.h>
#include <arch/i8259.h>
#include <func.h>
#include <cpu.h>
#include <arch/asm.h>
#include <mm/tlb.h>
#include <mm/as.h>
#include <arch.h>
#include <symtab.h>
#include <arch/asm.h>
#include <proc/scheduler.h>
#include <proc/thread.h>

static void print_info_errcode(int n, istate_t *istate)
{
	char *symbol;
	__u64 *x = &istate->stack[0];

	if (!(symbol=get_symtab_entry(istate->rip)))
		symbol = "";

	printf("-----EXCEPTION(%d) OCCURED----- ( %s )\n",n, __FUNCTION__);
	printf("%%rip: %Q (%s)\n",istate->rip, symbol);
	printf("ERROR_WORD=%Q\n", istate->error_word);
	printf("%%rcs=%Q, flags=%Q, %%cr0=%Q\n", istate->cs, istate->rflags,read_cr0());
	printf("%%rax=%Q, %%rbx=%Q, %%rcx=%Q\n",istate->rax,istate->rbx,istate->rcx);
	printf("%%rdx=%Q, %%rsi=%Q, %%rdi=%Q\n",istate->rdx,istate->rsi,istate->rdi);
	printf("%%r8 =%Q, %%r9 =%Q, %%r10=%Q\n",istate->r8,istate->r9,istate->r10);
	printf("%%r11=%Q, %%r12=%Q, %%r13=%Q\n",istate->r11,istate->r12,istate->r13);
	printf("%%r14=%Q, %%r15=%Q, %%rsp=%Q\n",istate->r14,istate->r15,&istate->stack[0]);
	printf("%%rbp=%Q\n",istate->rbp);
	printf("stack: %Q, %Q, %Q\n", x[5], x[6], x[7]);
	printf("       %Q, %Q, %Q\n", x[8], x[9], x[10]);
	printf("       %Q, %Q, %Q\n", x[11], x[12], x[13]);
	printf("       %Q, %Q, %Q\n", x[14], x[15], x[16]);
}

/*
 * Interrupt and exception dispatching.
 */

void (* disable_irqs_function)(__u16 irqmask) = NULL;
void (* enable_irqs_function)(__u16 irqmask) = NULL;
void (* eoi_function)(void) = NULL;

void null_interrupt(int n, istate_t *istate)
{
	print_info_errcode(n, istate);
	panic("unserviced interrupt\n");
}

void gp_fault(int n, istate_t *istate)
{
	print_info_errcode(n, istate);
	panic("general protection fault\n");
}

void ss_fault(int n, istate_t *istate)
{
	print_info_errcode(n, istate);
	panic("stack fault\n");
}


void nm_fault(int n, istate_t *istate)
{
#ifdef CONFIG_FPU_LAZY     
	scheduler_fpu_lazy_request();
#else
	panic("fpu fault");
#endif
}


/* Definitions for identic page mapper */
pte_t helper_ptl1[512] __attribute__((aligned (PAGE_SIZE)));
pte_t helper_ptl2[512] __attribute__((aligned (PAGE_SIZE)));
pte_t helper_ptl3[512] __attribute__((aligned (PAGE_SIZE)));
extern pte_t ptl_0; /* From boot.S */

#define PTL1_PRESENT(ptl0, page) (!(GET_PTL1_FLAGS_ARCH(ptl0, PTL0_INDEX_ARCH(page)) & PAGE_NOT_PRESENT))
#define PTL2_PRESENT(ptl1, page) (!(GET_PTL2_FLAGS_ARCH(ptl1, PTL1_INDEX_ARCH(page)) & PAGE_NOT_PRESENT))
#define PTL3_PRESENT(ptl2, page) (!(GET_PTL3_FLAGS_ARCH(ptl2, PTL2_INDEX_ARCH(page)) & PAGE_NOT_PRESENT))

#define PTL1_ADDR(ptl0, page) ((pte_t *)PA2KA(GET_PTL1_ADDRESS_ARCH(ptl0, PTL0_INDEX_ARCH(page))))
#define PTL2_ADDR(ptl1, page) ((pte_t *)PA2KA(GET_PTL2_ADDRESS_ARCH(ptl1, PTL1_INDEX_ARCH(page))))
#define PTL3_ADDR(ptl2, page) ((pte_t *)PA2KA(GET_PTL3_ADDRESS_ARCH(ptl2, PTL2_INDEX_ARCH(page))))

#define SETUP_PTL1(ptl0, page, tgt)  {	\
	SET_PTL1_ADDRESS_ARCH(ptl0, PTL0_INDEX_ARCH(page), (__address)KA2PA(tgt)); \
        SET_PTL1_FLAGS_ARCH(ptl0, PTL0_INDEX_ARCH(page), PAGE_WRITE | PAGE_EXEC); \
    }
#define SETUP_PTL2(ptl1, page, tgt)  {	\
	SET_PTL2_ADDRESS_ARCH(ptl1, PTL1_INDEX_ARCH(page), (__address)KA2PA(tgt)); \
        SET_PTL2_FLAGS_ARCH(ptl1, PTL1_INDEX_ARCH(page), PAGE_WRITE | PAGE_EXEC); \
    }
#define SETUP_PTL3(ptl2, page, tgt)  {	\
	SET_PTL3_ADDRESS_ARCH(ptl2, PTL2_INDEX_ARCH(page), (__address)KA2PA(tgt)); \
        SET_PTL3_FLAGS_ARCH(ptl2, PTL2_INDEX_ARCH(page), PAGE_WRITE | PAGE_EXEC); \
    }
#define SETUP_FRAME(ptl3, page, tgt)  {	\
	SET_FRAME_ADDRESS_ARCH(ptl3, PTL3_INDEX_ARCH(page), (__address)KA2PA(tgt)); \
        SET_FRAME_FLAGS_ARCH(ptl3, PTL3_INDEX_ARCH(page), PAGE_WRITE | PAGE_EXEC); \
    }

/** Identic page mapper
 *
 * We need to map whole physical memory identically before the page subsystem
 * is initializaed. This thing clears page table and fills in the specific
 * items.
 */
void ident_page_fault(int n, istate_t *istate)
{
	__address page;
	static __address oldpage = 0;
	pte_t *aptl_1, *aptl_2, *aptl_3;

	page = read_cr2();
	if (oldpage) {
		/* Unmap old address */
		aptl_1 = PTL1_ADDR(&ptl_0, oldpage);
		aptl_2 = PTL2_ADDR(aptl_1, oldpage);
		aptl_3 = PTL3_ADDR(aptl_2, oldpage);

		SET_FRAME_FLAGS_ARCH(aptl_3, PTL3_INDEX_ARCH(oldpage), PAGE_NOT_PRESENT);
		if (aptl_3 == helper_ptl3)
			SET_PTL3_FLAGS_ARCH(aptl_2, PTL2_INDEX_ARCH(oldpage), PAGE_NOT_PRESENT);
		if (aptl_2 == helper_ptl2)
			SET_PTL2_FLAGS_ARCH(aptl_1, PTL1_INDEX_ARCH(oldpage), PAGE_NOT_PRESENT);
		if (aptl_1 == helper_ptl1)
			SET_PTL1_FLAGS_ARCH(&ptl_0, PTL0_INDEX_ARCH(oldpage), PAGE_NOT_PRESENT);
	}
	if (PTL1_PRESENT(&ptl_0, page))
		aptl_1 = PTL1_ADDR(&ptl_0, page);
	else {
		SETUP_PTL1(&ptl_0, page, helper_ptl1);
		aptl_1 = helper_ptl1;
	}
	    
	if (PTL2_PRESENT(aptl_1, page)) 
		aptl_2 = PTL2_ADDR(aptl_1, page);
	else {
		SETUP_PTL2(aptl_1, page, helper_ptl2);
		aptl_2 = helper_ptl2;
	}

	if (PTL3_PRESENT(aptl_2, page))
		aptl_3 = PTL3_ADDR(aptl_2, page);
	else {
		SETUP_PTL3(aptl_2, page, helper_ptl3);
		aptl_3 = helper_ptl3;
	}
	
	SETUP_FRAME(aptl_3, page, page);

	oldpage = page;
}

void page_fault(int n, istate_t *istate)
{
	__address page;
	
	page = read_cr2();
	if (!as_page_fault(page)) {
		print_info_errcode(n, istate);
		printf("Page fault address: %Q\n", page);
		panic("page fault\n");
	}
}

void tlb_shootdown_ipi(int n, istate_t *istate)
{
	trap_virtual_eoi();
	tlb_shootdown_ipi_recv();
}

void trap_virtual_enable_irqs(__u16 irqmask)
{
	if (enable_irqs_function)
		enable_irqs_function(irqmask);
	else
		panic("no enable_irqs_function\n");
}

void trap_virtual_disable_irqs(__u16 irqmask)
{
	if (disable_irqs_function)
		disable_irqs_function(irqmask);
	else
		panic("no disable_irqs_function\n");
}

void trap_virtual_eoi(void)
{
	if (eoi_function)
		eoi_function();
	else
		panic("no eoi_function\n");

}
