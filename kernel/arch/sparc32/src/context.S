#
# Copyright (c) 2005 Jakub Jermar
# Copyright (c) 2013 Jakub Klama
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

#include <arch/context_offset.h>
#include <arch/arch.h>

.text

.global context_save_arch
.global context_restore_arch

/*
 * context_save_arch() is required not to create its own stack frame. See the
 * generic context.h for explanation.
 */
context_save_arch:
	#
	# Force all our active register windows to memory so that we can find
	# them there even if e.g. the thread is migrated to another processor.
	#
	
	## flushw
	mov 7, %g1
	1:
		subcc %g1, 1, %g1
		bg 1b
		save %sp, -64, %sp
	
	mov 7, %g1
	1:
		subcc %g1, 1, %g1
		bg 1b
		restore
	
	CONTEXT_SAVE_ARCH_CORE %o0
	retl
	mov 1, %o0		! context_save_arch returns 1

context_restore_arch:
	#
	# Forget all previous windows, they are not going to be needed again.
	# Enforce a window fill on the next RESTORE instruction by setting
	# CANRESTORE to zero and other window configuration registers
	# accordingly. Note that the same can be achieved by executing the
	# FLUSHW instruction, but since we don't need to remember the previous
	# windows, we do the former and save thus some unnecessary window
	# spills.
	#
	
	## rdpr %pstate, %l0
	## andn %l0, PSTATE_IE_BIT, %l1
	## wrpr %l1, %pstate
	## wrpr %g0, 0, %canrestore
	## wrpr %g0, 0, %otherwin
	## wrpr %g0, NWINDOWS - 2, %cansave
	## wrpr %l0, %pstate
	
	mov 7, %g1
	1:
		subcc %g1, 1, %g1
		bg 1b
		save %sp, -64, %sp
	
	mov 7, %g1
	1:
		subcc %g1, 1, %g1
		bg 1b
		restore
	
	CONTEXT_RESTORE_ARCH_CORE %o0
	retl
	xor %o0, %o0, %o0	! context_restore_arch returns 0
