/*
 * Copyright (c) 2003-2004 Jakub Jermar
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

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_CP0_H_
#define KERN_mips32_CP0_H_

#include <stdint.h>

#define cp0_status_ie_enabled_bit     (1 << 0)
#define cp0_status_exl_exception_bit  (1 << 1)
#define cp0_status_erl_error_bit      (1 << 2)
#define cp0_status_um_bit             (1 << 4)
#define cp0_status_bev_bootstrap_bit  (1 << 22)
#define cp0_status_fpu_bit            (1 << 29)

#define cp0_status_im_shift  8
#define cp0_status_im_mask   0xff00

#define cp0_cause_ip_shift  8
#define cp0_cause_ip_mask   0xff00

#define cp0_cause_excno(cause)   ((cause >> 2) & 0x1f)
#define cp0_cause_coperr(cause)  ((cause >> 28) & 0x3)

#define fpu_cop_id  1

/*
 * Magic value for use in msim.
 */
#define cp0_compare_value  100000

#define cp0_mask_all_int() \
	cp0_status_write(cp0_status_read() & ~(cp0_status_im_mask))

#define cp0_unmask_all_int() \
	cp0_status_write(cp0_status_read() | cp0_status_im_mask)

#define cp0_mask_int(it) \
	cp0_status_write(cp0_status_read() & ~(1 << (cp0_status_im_shift + (it))))

#define cp0_unmask_int(it) \
	cp0_status_write(cp0_status_read() | (1 << (cp0_status_im_shift + (it))))

#define GEN_READ_CP0(nm, reg) \
	static inline uint32_t cp0_ ##nm##_read(void) \
	{ \
		uint32_t retval; \
		\
		asm volatile ( \
			"mfc0 %0, $" #reg \
			: "=r"(retval) \
		); \
		\
		return retval; \
	}

#define GEN_WRITE_CP0(nm, reg) \
	static inline void cp0_ ##nm##_write(uint32_t val) \
	{ \
		asm volatile ( \
			"mtc0 %0, $" #reg \
			:: "r"(val) \
		); \
	}

GEN_READ_CP0(index, 0);
GEN_WRITE_CP0(index, 0);

GEN_READ_CP0(random, 1);

GEN_READ_CP0(entry_lo0, 2);
GEN_WRITE_CP0(entry_lo0, 2);

GEN_READ_CP0(entry_lo1, 3);
GEN_WRITE_CP0(entry_lo1, 3);

GEN_READ_CP0(context, 4);
GEN_WRITE_CP0(context, 4);

GEN_READ_CP0(pagemask, 5);
GEN_WRITE_CP0(pagemask, 5);

GEN_READ_CP0(wired, 6);
GEN_WRITE_CP0(wired, 6);

GEN_READ_CP0(badvaddr, 8);

GEN_READ_CP0(count, 9);
GEN_WRITE_CP0(count, 9);

GEN_READ_CP0(entry_hi, 10);
GEN_WRITE_CP0(entry_hi, 10);

GEN_READ_CP0(compare, 11);
GEN_WRITE_CP0(compare, 11);

GEN_READ_CP0(status, 12);
GEN_WRITE_CP0(status, 12);

GEN_READ_CP0(cause, 13);
GEN_WRITE_CP0(cause, 13);

GEN_READ_CP0(epc, 14);
GEN_WRITE_CP0(epc, 14);

GEN_READ_CP0(prid, 15);

#endif

/** @}
 */
