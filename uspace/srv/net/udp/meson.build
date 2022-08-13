#
# SPDX-FileCopyrightText: 2012 Jiri Svoboda
#
# SPDX-License-Identifier: BSD-3-Clause
#

deps = [ 'nettl' ]

_common_src = files(
	'assoc.c',
	'cassoc.c',
	'msg.c',
	'pdu.c',
)

src = files(
	'service.c',
	'udp.c',
	'udp_inet.c',
)

test_src = files(
	'test/assoc.c',
	'test/msg.c',
	'test/main.c',
	'test/pdu.c',
)

src = [ _common_src, src ]
test_src = [ _common_src, test_src ]
