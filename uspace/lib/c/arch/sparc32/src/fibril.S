#
# Copyright (c) 2005 Jakub Jermar
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

#include <libarch/context_offset.h>

.text

.global flush_windows
.global context_save
.global context_restore

context_save:
	#
	# We rely on the kernel to flush our active register windows to memory
	# should a thread switch occur.
	#
	
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
	mov 1, %o0              ! context_save_arch returns 1

context_restore:
	#
	# Flush all active windows.
	# This is essential, because CONTEXT_RESTORE_ARCH_CORE overwrites %sp of
	# CWP - 1 with the value written to %fp of CWP.  Flushing all active
	# windows mitigates this problem as CWP - 1 becomes the overlap window.
	#
	
	## XXX
	## flushw
	## ta 0x4f
	## nop
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
	xor %o0, %o0, %o0       ! context_restore_arch returns 0
