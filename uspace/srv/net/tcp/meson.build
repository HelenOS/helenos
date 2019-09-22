#
# Copyright (c) 2011 Jiri Svoboda
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

deps = [ 'nettl' ]

_common_src = files(
	'conn.c',
	'inet.c',
	'iqueue.c',
	'ncsim.c',
	'pdu.c',
	'rqueue.c',
	'segment.c',
	'seq_no.c',
	'test.c',
	'tqueue.c',
	'ucall.c',
)

src = files(
	'service.c',
	'tcp.c',
)

test_src = files(
	'test/conn.c',
	'test/iqueue.c',
	'test/main.c',
	'test/pdu.c',
	'test/rqueue.c',
	'test/segment.c',
	'test/seq_no.c',
	'test/tqueue.c',
	'test/ucall.c',
)

src = [ _common_src, src ]
test_src = [ _common_src, test_src ]
