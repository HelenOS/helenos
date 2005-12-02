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
 * This file contains two trap tables. First, trap_table, is the one
 * to wich kernel copies OFW's own trap table and patches it with
 * kernel-provided handlers from the second trap table, trap_table_kernel.
 */
 
.text

#include <arch/trap_table.h>

#define TABLE_SIZE	TRAP_TABLE_SIZE
#define ENTRY_SIZE	TRAP_TABLE_ENTRY_SIZE


/*
 * Trap table for copy of OFW's own trap table.
 */
.align TABLE_SIZE
.global trap_table
trap_table:
	.space TABLE_SIZE, 0


/*
 * Kernel-provided trap handlers.
 */
.global trap_table_kernel
trap_table_kernel:

/*
 * Hardware interrupts for TL=0.
 */
.org trap_table_kernel + 0*ENTRY_SIZE
tl_0_hardware_traps:

/*
 * Register window spill/fill/clean traps for TL=0.
 */
.org trap_table_kernel + 128*ENTRY_SIZE
tl_0_regwin_traps:

/*
 * Software traps for TL=0.
 */
.org trap_table_kernel + 256*ENTRY_SIZE
tl_0_software_traps:

/* Reserved area. */
.org trap_table_kernel + 384*ENTRY_SIZE
.space 128*ENTRY_SIZE, 0

/*
 * Hardware interrupts for TL>0.
 */
.org trap_table_kernel + 512*ENTRY_SIZE
tl_non_0_hardware_traps:

/*
 * Register window spill/fill/clean traps for TL>0.
 */
.org trap_table_kernel + 640*ENTRY_SIZE
tl_non_0_regwin_traps:

/*
 * Software traps for TL>0.
 */
.org trap_table_kernel + 768*ENTRY_SIZE
tl_non_0_software_traps:

/* Reserved area. */
.org trap_table_kernel + 896*ENTRY_SIZE
.space 128*ENTRY_SIZE, 0
