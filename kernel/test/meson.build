#
# Copyright (c) 2019 Jiří Zárevúcky
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

## Test sources
#

test_src = []

if CONFIG_TEST
	test_src += files(
		'test.c',
		'atomic/atomic1.c',
		'fault/fault1.c',
		'mm/falloc1.c',
		'mm/falloc2.c',
		'mm/mapping1.c',
		'mm/slab1.c',
		'mm/slab2.c',
		'synch/semaphore1.c',
		'synch/semaphore2.c',
		'print/print1.c',
		'print/print2.c',
		'print/print3.c',
		'print/print4.c',
		'print/print5.c',
		'thread/thread1.c',
	)

	if KARCH == 'mips32'
		test_src += files('debug/mips1.c')
	else
		test_src += files('debug/mips1_skip.c')
	endif

	if KARCH == 'ia64'
		test_src += files('mm/purge1.c')
	else
		test_src += files('mm/purge1_skip.c')
	endif
endif
