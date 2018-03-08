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

/** @addtogroup sparc64interrupt
 * @{
 */
/**
 * @file
 */

#ifndef KERN_sparc64_EXCEPTION_H_
#define KERN_sparc64_EXCEPTION_H_

#define TT_INSTRUCTION_ACCESS_EXCEPTION		0x08
#define TT_INSTRUCTION_ACCESS_MMU_MISS		0x09
#define TT_INSTRUCTION_ACCESS_ERROR		0x0a
#define	TT_IAE_UNAUTH_ACCESS			0x0b
#define	TT_IAE_NFO_PAGE				0x0c
#define TT_ILLEGAL_INSTRUCTION			0x10
#define TT_PRIVILEGED_OPCODE			0x11
#define TT_UNIMPLEMENTED_LDD			0x12
#define TT_UNIMPLEMENTED_STD			0x13
#define TT_DAE_INVALID_ASI			0x14
#define TT_DAE_PRIVILEGE_VIOLATION		0x15
#define TT_DAE_NC_PAGE				0x16
#define TT_DAE_NFO_PAGE				0x17
#define TT_FP_DISABLED				0x20
#define TT_FP_EXCEPTION_IEEE_754		0x21
#define TT_FP_EXCEPTION_OTHER			0x22
#define TT_TAG_OVERFLOW				0x23
#define TT_DIVISION_BY_ZERO			0x28
#define TT_DATA_ACCESS_EXCEPTION		0x30
#define TT_DATA_ACCESS_MMU_MISS			0x31
#define TT_DATA_ACCESS_ERROR			0x32
#define TT_MEM_ADDRESS_NOT_ALIGNED		0x34
#define TT_LDDF_MEM_ADDRESS_NOT_ALIGNED		0x35
#define TT_STDF_MEM_ADDRESS_NOT_ALIGNED		0x36
#define TT_PRIVILEGED_ACTION			0x37
#define TT_LDQF_MEM_ADDRESS_NOT_ALIGNED		0x38
#define TT_STQF_MEM_ADDRESS_NOT_ALIGNED		0x39

#ifndef __ASSEMBLER__

#include <arch/interrupt.h>

extern void dump_istate(istate_t *istate);

extern void instruction_access_exception(unsigned int, istate_t *);
extern void instruction_access_error(unsigned int, istate_t *);
extern void illegal_instruction(unsigned int, istate_t *);
extern void privileged_opcode(unsigned int, istate_t *);
extern void unimplemented_LDD(unsigned int, istate_t *);
extern void unimplemented_STD(unsigned int, istate_t *);
extern void fp_disabled(unsigned int, istate_t *);
extern void fp_exception_ieee_754(unsigned int, istate_t *);
extern void fp_exception_other(unsigned int, istate_t *);
extern void tag_overflow(unsigned int, istate_t *);
extern void division_by_zero(unsigned int, istate_t *);
extern void data_access_exception(unsigned int, istate_t *);
extern void data_access_error(unsigned int, istate_t *);
extern void mem_address_not_aligned(unsigned int, istate_t *);
extern void LDDF_mem_address_not_aligned(unsigned int, istate_t *);
extern void STDF_mem_address_not_aligned(unsigned int, istate_t *);
extern void privileged_action(unsigned int, istate_t *);
extern void LDQF_mem_address_not_aligned(unsigned int, istate_t *);
extern void STQF_mem_address_not_aligned(unsigned int, istate_t *);

#endif /* !__ASSEMBLER__ */

#endif

/** @}
 */
