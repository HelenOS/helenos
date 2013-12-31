/*
 * Copyright (c) 2005 Jakub Jermar
 * Copyright (c) 2013 Jakub Klama
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

/** @addtogroup sparc64interrupt
 * @{
 */
/**
 * @file
 */

#ifndef KERN_sparc32_EXCEPTION_H_
#define KERN_sparc32_EXCEPTION_H_

#define TT_INSTRUCTION_ACCESS_EXCEPTION  0x01
#define TT_ILLEGAL_INSTRUCTION           0x02
#define TT_PRIVILEGED_INSTRUCTION        0x03
#define TT_MEM_ADDRESS_NOT_ALIGNED       0x07
#define TT_FP_DISABLED                   0x08
#define TT_DATA_ACCESS_EXCEPTION         0x09
#define TT_INSTRUCTION_ACCESS_ERROR      0x21
#define TT_DATA_ACCESS_ERROR             0x29
#define TT_DIVISION_BY_ZERO              0x2a
#define TT_DATA_ACCESS_MMU_MISS          0x2c
#define TT_INSTRUCTION_ACCESS_MMU_MISS   0x3c

#ifndef __ASM__

extern void instruction_access_exception(int, istate_t *);
extern void instruction_access_error(int, istate_t *);
extern void illegal_instruction(int, istate_t *);
extern void privileged_instruction(int, istate_t *);
extern void fp_disabled(int, istate_t *);
extern void fp_exception(int, istate_t *);
extern void tag_overflow(int, istate_t *);
extern void division_by_zero(int, istate_t *);
extern void data_access_exception(int, istate_t *);
extern void data_access_error(int, istate_t *);
extern void data_access_mmu_miss(int, istate_t *);
extern void data_store_error(int, istate_t *);
extern void mem_address_not_aligned(int, istate_t *);

extern sysarg_t syscall(sysarg_t, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t, sysarg_t);
extern void irq_exception(unsigned int, istate_t *);

#endif /* !__ASM__ */

#endif

/** @}
 */
