#
# Copyright (C) 2005 Jakub Jermar
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

/**
 * This file contains two trap tables.
 * First, trap_table, is the one wich contains handlers implemented by
 * kernel. During initialization, these handlers are copied out to
 * the second trap table, trap_table_save, and the first table is
 * overwritten with copy of OFW's own trap table. The copy is then patched
 * from the trap_table_save.
 *
 * This arrangement is beneficial because kernel handlers stay on their
 * link-time addresses which is good for debugging.
 */
 
.text

#include <arch/trap/trap_table.h>
#include <arch/trap/regwin.h>
#include <arch/trap/interrupt.h>

#define TABLE_SIZE	TRAP_TABLE_SIZE
#define ENTRY_SIZE	TRAP_TABLE_ENTRY_SIZE

/*
 * Kernel trap table.
 */
.align TABLE_SIZE
.global trap_table
trap_table:

/* TT = 0x24, TL = 0, clean_window handler */
.org trap_table + TT_CLEAN_WINDOW*ENTRY_SIZE
.global clean_window_handler
clean_window_handler:
	CLEAN_WINDOW_HANDLER

/* TT = 0x60, TL = 0, interrupt_vector_trap handler */
.org trap_table + TT_INTERRUPT_VECTOR_TRAP*ENTRY_SIZE
.global interrupt_vector_trap_handler
interrupt_vector_trap_handler:
	INTERRUPT_VECTOR_TRAP_HANDLER

/* TT = 0x80, TL = 0, spill_0_normal handler */
.org trap_table + TT_SPILL_0_NORMAL*ENTRY_SIZE
.global spill_0_normal
spill_0_normal:
	SPILL_NORMAL_HANDLER

/* TT = 0xc0, TL = 0, fill_0_normal handler */
.org trap_table + TT_FILL_0_NORMAL*ENTRY_SIZE
.global fill_0_normal
fill_0_normal:
	FILL_NORMAL_HANDLER

/*
 * Handlers for TL>0.
 */

/* TT = 0x24, TL > 0, clean_window handler */
.org trap_table + (TT_CLEAN_WINDOW+512)*ENTRY_SIZE
.global clean_window_handler_high
clean_window_handler_high:
	CLEAN_WINDOW_HANDLER


/* TT = 0x80, TL > 0, spill_0_normal handler */

.org trap_table + (TT_SPILL_0_NORMAL+512)*ENTRY_SIZE
.global spill_0_normal_high
spill_0_normal_high:
	SPILL_NORMAL_HANDLER


/* TT = 0xc0, TL > 0, fill_0_normal handler */
.org trap_table + (TT_FILL_0_NORMAL+512)*ENTRY_SIZE
.global fill_0_normal_high
fill_0_normal_high:
	FILL_NORMAL_HANDLER



/*
 * Save trap table.
 */
.align TABLE_SIZE
.global trap_table_save
trap_table_save:
	.space TABLE_SIZE, 0
