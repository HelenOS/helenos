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

.section K_TEXT_START
.global k_text_start
k_text_start:

.global kernel_image_start

stack0:
kernel_image_start:
	# initialize register stack
	alloc r1 = ar.pfs, 0, 0, 0, 0	;;

	# initialize memory stack to some sane value
	movl r12=stack0

	# initialize gp (Global Pointer) register
	movl r1=k_text_start

	#
	# Initialize hardcoded_* variables.
	# (IA-64 port doesn't get these values from linker.)
	#
	movl r8 = k_text_start	;;
	movl r9 = k_text_end	;;
	sub r8 = r9, r8		
	addl r10 = @gprel(hardcoded_ktext_size), gp;;
	st4 [r10] = r8		;;

	movl r8 = k_data_start	;;
	movl r9 = k_data_end	;;
	sub r8 = r9, r8		
	addl r10 = @gprel(hardcoded_kdata_size), gp;;
	st4 [r10] = r8		;;
	
	addl r10 = @gprel(hardcoded_load_address), gp;;
	st8 [r10] = r1
	
	br.call.sptk.many b0=main_bsp

0:
	br 0b

.section K_TEXT_END
.global k_text_end
k_text_end:

.section K_DATA_START
.global k_data_start
k_data_start:

.section K_DATA_END
.global k_data_end
k_data_end:
