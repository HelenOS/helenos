#
# Copyright (C) 2001-2004 Jakub Jermar
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

.global atomic_inc
atomic_inc:
	pushl %ebx
	movl 8(%esp),%ebx
	lock incl (%ebx)
	popl %ebx
	ret

.global atomic_dec
atomic_dec:
	pushl %ebx
	movl 8(%esp),%ebx
	lock decl (%ebx)
	popl %ebx
	ret
	

#ifdef __SMP__


.global test_and_set
.global spinlock_arch

test_and_set:
	pushl %ebx
    
	movl 8(%esp),%ebx
	movl $1,%eax
	xchgl %eax,(%ebx)	# xchg implicitly turns on the LOCK signal
    
	popl %ebx
	ret


#
# This is a bus-and-hyperthreading-friendly implementation of spinlock 
#
spinlock_arch:
	pushl %eax
	pushl %ebx
	
	movl 12(%esp),%ebx

0:	
	#ifdef __HT__
	pause			# Pentium 4's with HT love this instruction
	#endif
	movl (%ebx),%eax
	testl %eax,%eax
	jnz 0b			# lightweight looping while it is locked
	incl %eax
	xchgl %eax,(%ebx)	# now use the atomic operation
	testl %eax,%eax
	jnz 0b

	popl %ebx
	popl %eax
	ret
	
#endif
