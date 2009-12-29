#
# Copyright (c) 2009 Jakub Jermar
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

.text

.global frame_pointer_get
.global frame_pointer_prev
.global frame_pointer_validate
.global return_address_get

frame_pointer_get:
	# Add the stack bias to %sp to get the actual address.
	retl
	add %sp, 0x7ff, %o0

frame_pointer_prev:
	save %sp, -176, %sp
	# Flush all other windows to memory so that we can read their contents.
	flushw
	# Read the %fp from the window save area.
	ldx [%i0 + 14 * 8], %i0
	# Add the stack bias to the %fp read from the window save area.
	add %i0, 0x7ff, %i0
	ret
	restore

frame_pointer_validate:
	# Just return the frame address.
	retl
	nop

return_address_get:
	save %sp, -176, %sp
	# Flush all other windows to memory so that we can read their contents.
	flushw
	# Read the %i7 from the window save area.
	ldx [%i0 + 15 * 8], %i0
	ret
	restore
