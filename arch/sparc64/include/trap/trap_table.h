/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __sparc64_TRAP_TABLE_H__
#define __sparc64_TRAP_TABLE_H__

#ifndef __ASM__
#include <arch/types.h>
#endif /* __ASM__ */

#define TRAP_TABLE_ENTRY_COUNT	1024
#define TRAP_TABLE_ENTRY_SIZE	32
#define TRAP_TABLE_SIZE		(TRAP_TABLE_ENTRY_COUNT*TRAP_TABLE_ENTRY_SIZE)

#ifndef __ASM__
struct trap_table_entry {
	__u8 octets[TRAP_TABLE_ENTRY_SIZE];
} __attribute__ ((packed));

typedef struct trap_table_entry trap_table_entry_t;

extern trap_table_entry_t trap_table[TRAP_TABLE_ENTRY_COUNT];
extern trap_table_entry_t trap_table_save[TRAP_TABLE_ENTRY_COUNT];
#endif /* !__ASM__ */

#endif
