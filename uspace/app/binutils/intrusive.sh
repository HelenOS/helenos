#! /bin/bash

#
# Copyright (c) 2011 Petr Koupy
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

#
# This shell script is reserved for intrusive patches (hacks) to binutils
# that are not possible to be done in a clean and isolated way or for
# which doing so would require too much complexity.
#

case "$1" in
	"do")
		# Binutils plugin support is dependent on libdl.so library.
		# By default, the plugin support is switched of for all
		# configure scripts of binutils. The only exception is configure
		# script of ld 2.21 (and possibly above), where plugin support
		# became mandatory (although not really needed). 
		mv -f "$2/ld/configure" "$2/ld/configure.backup"
		sed 's/enable_plugins=yes/enable_plugins=no/g' \
			< "$2/ld/configure.backup" > "$2/ld/configure"
		;;
	"undo")
		mv -f "$2/ld/configure.backup" "$2/ld/configure"
		;;
	*)
		;;
esac

