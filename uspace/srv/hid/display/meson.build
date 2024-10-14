#
# Copyright (c) 2024 Jiri Svoboda
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

deps = [ 'ipcgfx', 'memgfx', 'display', 'ddev', 'dispcfg', 'wndmgt', 'input',
    'sif' ]

src = files(
	'cfgclient.c',
	'cfgops.c',
	'client.c',
	'clonegc.c',
	'cursor.c',
	'cursimg.c',
	'ddev.c',
	'display.c',
	'dsops.c',
	'idevcfg.c',
	'ievent.c',
	'input.c',
	'main.c',
	'output.c',
	'seat.c',
	'window.c',
	'wmclient.c',
	'wmops.c',
)

test_src = files(
	'cfgclient.c',
	'client.c',
	'clonegc.c',
	'cursimg.c',
	'cursor.c',
	'ddev.c',
	'display.c',
	'idevcfg.c',
	'ievent.c',
	'seat.c',
	'window.c',
	'wmclient.c',
	'test/cfgclient.c',
	'test/client.c',
	'test/clonegc.c',
	'test/cursor.c',
	'test/display.c',
	'test/ievent.c',
	'test/main.c',
	'test/seat.c',
	'test/window.c',
	'test/wmclient.c',
)
