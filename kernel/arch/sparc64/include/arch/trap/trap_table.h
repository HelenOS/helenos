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
/** @file
 */

#ifndef KERN_sparc64_TRAP_TABLE_H_
#define KERN_sparc64_TRAP_TABLE_H_

#include <arch/stack.h>
#include <arch/istate_struct.h>

#define TRAP_TABLE_ENTRY_COUNT	1024
#define TRAP_TABLE_ENTRY_SIZE	32
#define TRAP_TABLE_SIZE		(TRAP_TABLE_ENTRY_COUNT * TRAP_TABLE_ENTRY_SIZE)

#define ISTATE_END_OFFSET(o)	((o) - ISTATE_SIZE)

/*
 * The one STACK_ITEM_SIZE is counted for space holding the 7th
 * argument to syscall_handler (i.e. syscall number) and the other
 * STACK_ITEM_SIZE is counted because of the required alignment.
 */
#define PREEMPTIBLE_HANDLER_STACK_FRAME_SIZE \
    (STACK_WINDOW_SAVE_AREA_SIZE + STACK_ARG_SAVE_AREA_SIZE + \
    (2 * STACK_ITEM_SIZE) + (ISTATE_SIZE + 9 * 8))
/* <-- istate_t ends here */
#define SAVED_TSTATE	ISTATE_END_OFFSET(ISTATE_OFFSET_TSTATE)	
#define SAVED_TPC	ISTATE_END_OFFSET(ISTATE_OFFSET_TPC)
#define SAVED_TNPC	ISTATE_END_OFFSET(ISTATE_OFFSET_TNPC)
/* <-- istate_t begins here */
#define SAVED_Y		-(1 * 8 + ISTATE_SIZE)
#define SAVED_I0	-(2 * 8 + ISTATE_SIZE)
#define SAVED_I1	-(3 * 8 + ISTATE_SIZE)
#define SAVED_I2	-(4 * 8 + ISTATE_SIZE)
#define SAVED_I3	-(5 * 8 + ISTATE_SIZE)
#define SAVED_I4	-(6 * 8 + ISTATE_SIZE)
#define SAVED_I5	-(7 * 8 + ISTATE_SIZE)
#define SAVED_I6	-(8 * 8 + ISTATE_SIZE)
#define SAVED_I7	-(9 * 8 + ISTATE_SIZE)

#ifndef __ASM__

#include <typedefs.h>

struct trap_table_entry {
	uint8_t octets[TRAP_TABLE_ENTRY_SIZE];
} __attribute__ ((packed));

typedef struct trap_table_entry trap_table_entry_t;

extern trap_table_entry_t trap_table[TRAP_TABLE_ENTRY_COUNT];
extern trap_table_entry_t trap_table_save[TRAP_TABLE_ENTRY_COUNT];
#endif /* !__ASM__ */

#ifdef __ASM__
.macro SAVE_GLOBALS
	mov %g1, %l1
	mov %g2, %l2
	mov %g3, %l3
	mov %g4, %l4
	mov %g5, %l5
	mov %g6, %l6
	mov %g7, %l7
.endm

.macro RESTORE_GLOBALS
	mov %l1, %g1
	mov %l2, %g2
	mov %l3, %g3
	mov %l4, %g4
	mov %l5, %g5
	mov %l6, %g6
	mov %l7, %g7
.endm

.macro PREEMPTIBLE_HANDLER f
	sethi %hi(\f), %g1
	ba %xcc, preemptible_handler
	or %g1, %lo(\f), %g1
.endm

#endif /* __ASM__ */

#endif

/** @}
 */
