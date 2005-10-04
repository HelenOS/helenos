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

#ifndef __mips32_CP0_H__
#define __mips32_CP0_H__

#include <arch/types.h>
#include <arch/mm/tlb.h>

#define cp0_status_ie_enabled_bit	(1<<0)
#define cp0_status_exl_exception_bit	(1<<1)
#define cp0_status_erl_error_bit	(1<<2)
#define cp0_status_um_bit	        (1<<4)
#define cp0_status_bev_bootstrap_bit	(1<<22)
#define cp0_status_fpu_bit              (1<<29)

#define cp0_status_im_shift		8
#define cp0_status_im_mask              0xff00

#define cp0_cause_excno(cause) ((cause >> 2) & 0x1f)
#define cp0_cause_coperr(cause) ((cause >> 28) & 0x3)

#define fpu_cop_id 1

/*
 * Magic value for use in msim.
 * On AMD Duron 800Mhz, this roughly seems like one us.
 */
#define cp0_compare_value 		10000

#define cp0_mask_all_int() cp0_status_write(cp0_status_read() & ~(cp0_status_im_mask))
#define cp0_unmask_all_int() cp0_status_write(cp0_status_read() | cp0_status_im_mask)
#define cp0_mask_int(it) cp0_status_write(cp0_status_read() & ~(1<<(cp0_status_im_shift+(it))))
#define cp0_unmask_int(it) cp0_status_write(cp0_status_read() | (1<<(cp0_status_im_shift+(it))))

extern  __u32 cp0_index_read(void);
extern void cp0_index_write(__u32 val);

extern __u32 cp0_random_read(void);

extern __u32 cp0_entry_lo0_read(void);
extern void cp0_entry_lo0_write(__u32 val);

extern __u32 cp0_entry_lo1_read(void);
extern void cp0_entry_lo1_write(__u32 val);

extern __u32 cp0_context_read(void);
extern void cp0_context_write(__u32 val);

extern __u32 cp0_pagemask_read(void);
extern void cp0_pagemask_write(__u32 val);

extern __u32 cp0_wired_read(void);
extern void cp0_wired_write(__u32 val);

extern __u32 cp0_badvaddr_read(void);

extern volatile __u32 cp0_count_read(void);
extern void cp0_count_write(__u32 val);

extern volatile __u32 cp0_entry_hi_read(void);
extern void cp0_entry_hi_write(__u32 val);

extern volatile __u32 cp0_compare_read(void);
extern void cp0_compare_write(__u32 val);

extern __u32 cp0_status_read(void);
extern void cp0_status_write(__u32 val);

extern __u32 cp0_cause_read(void);
extern void cp0_cause_write(__u32 val);

extern __u32 cp0_epc_read(void);
extern void cp0_epc_write(__u32 val);

extern __u32 cp0_prid_read(void);

#endif
