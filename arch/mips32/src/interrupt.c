/*
 * Copyright (C) 2003-2004 Jakub Jermar
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
#include <arch/types.h>
#include <arch.h>
#include <arch/cp0.h>
#include <time/clock.h>
#include <panic.h>
#include <print.h>
#include <symtab.h>
#include <arch/drivers/arc.h>

static void print_regdump(struct exception_regdump *pstate)
{
	char *pcsymbol = "";
	char *rasymbol = "";

	char *s = get_symtab_entry(pstate->epc);
	if (s)
		pcsymbol = s;
	s = get_symtab_entry(pstate->ra);
	if (s)
		rasymbol = s;
	
	printf("PC: %X(%s) RA: %X(%s)\n",pstate->epc,pcsymbol,
	       pstate->ra,rasymbol);
}

/** Disable interrupts.
 *
 * @return Old interrupt priority level.
 */
ipl_t interrupts_disable(void)
{
	ipl_t ipl = (ipl_t) cp0_status_read();
	cp0_status_write(ipl & ~cp0_status_ie_enabled_bit);
	return ipl;
}

/** Enable interrupts.
 *
 * @return Old interrupt priority level.
 */
ipl_t interrupts_enable(void)
{
	ipl_t ipl = (ipl_t) cp0_status_read();
	cp0_status_write(ipl | cp0_status_ie_enabled_bit);
	return ipl;
}

/** Restore interrupt priority level.
 *
 * @param ipl Saved interrupt priority level.
 */
void interrupts_restore(ipl_t ipl)
{
	cp0_status_write(cp0_status_read() | (ipl & cp0_status_ie_enabled_bit));
}

/** Read interrupt priority level.
 *
 * @return Current interrupt priority level.
 */
ipl_t interrupts_read(void)
{
	return cp0_status_read();
}

void interrupt(struct exception_regdump *pstate)
{
	__u32 cause;
	int i;
	
	/* decode interrupt number and process the interrupt */
	cause = (cp0_cause_read() >> 8) &0xff;
	
	for (i = 0; i < 8; i++) {
		if (cause & (1 << i)) {
			switch (i) {
				case 0: /* SW0 - Software interrupt 0 */
					cp0_cause_write(cp0_cause_read() & ~(1 << 8)); /* clear SW0 interrupt */
					break;
				case 1: /* SW1 - Software interrupt 1 */
					cp0_cause_write(cp0_cause_read() & ~(1 << 9)); /* clear SW1 interrupt */
					break;
				case 2: /* IRQ0 */
				case 3: /* IRQ1 */
				case 4: /* IRQ2 */
				case 5: /* IRQ3 */
				case 6: /* IRQ4 */
				default:
					print_regdump(pstate);
					panic("unhandled interrupt %d\n", i);
					break;
				case TIMER_INTERRUPT:
					/* clear timer interrupt & set new */
					cp0_compare_write(cp0_count_read() + cp0_compare_value); 
					clock();
					break;
			}
		}
	}

}
