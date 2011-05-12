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
# This script is intended to be called indirectly from the Makefile.
# Given information from Makefile.common (e.g. CFLAGS), this shell
# script generates false toolchain consisting of a short shell script
# for each tool. Therefore, generated false tool already contains some
# filtered information from the HelenOS side. When the actual call is
# intercepted, information passed from the binutils side is filtered
# and stored. Information from both sides is then joined together
# and passed to the real tool.
#
# Basic idea behind filtering the information is that binutils is
# dominant provider for the compilation, whereas HelenOS is dominant
# for the linkage. In that way, the compilation command from binutils
# is left untouched and only extended with a couple of flags that are
# specific for HelenOS (i.e. include directories, preprocessor 
# directives, preprocessor flags, standard libc exclusion). Similarly,
# the linkage command is taken from HelenOS and extended with the
# filtered information from binutils (i.e. output binary, objects,
# binutils libraries).
#
# Most of the false tools are straightforward. However, gcc needs a 
# special approach because binutils use it for both compilation and
# linkage. In case the linking usage is detected, the call is redirected
# to the linker.
#

case "$1" in
	"gcc")
		echo '#! /bin/bash' > 'gcc'
		echo 'AS_LINK="`echo $* | grep '\'as-new\''`"' >> 'gcc'
		echo 'LD_LINK="`echo $* | grep '\'ld-new\''`"' >> 'gcc'
		echo 'LINK="`echo -n "$AS_LINK""$LD_LINK"`"' >> 'gcc'
		echo 'if [ -n "$LINK" ]; then' >> 'gcc'
		echo '	LD_ARGS="`echo $* | \' >> 'gcc'
		echo '		sed '\'s/-O[^ ]*//g\'' | \' >> 'gcc'
		echo '		sed '\'s/-W[^ ]*//g\'' | \' >> 'gcc'
		echo '		sed '\'s/-g//g\'' | \' >> 'gcc'
		echo '		sed '\'s/-l[^ ]*//g\'' | \' >> 'gcc'
		echo '		sed '\'s/ [ ]*/ /g\''`"' >> 'gcc'
		echo '	ld "$LD_ARGS"' >> 'gcc'
		echo 'else' >> 'gcc'
		CFLAGS="`echo "$3" | \
			sed 's/-O[^ ]*//g' | \
			sed 's/-W[^ ]*//g' | \
			sed 's/-pipe//g' | \
			sed 's/-g//g' | \
			sed 's/ [ ]*/ /g'`"
		echo "	$2" "$CFLAGS" '$@' >> 'gcc'
		echo 'fi' >> 'gcc'
		chmod a+x 'gcc'
		;;
	"as")
		echo '#! /bin/bash' > 'as'
		echo "$2" '$@' >> 'as'
		chmod a+x 'as'
		;;
	"ar")
		echo '#! /bin/bash' > 'ar'
		echo "$2" '$@' >> 'ar'
		chmod a+x 'ar'
		;;
	"ranlib")
		echo '#! /bin/bash' > 'ranlib'
		echo "ar -s" '$@' >> 'ranlib'
		chmod a+x 'ranlib'
		;;
	"ld")
		echo '#! /bin/bash' > 'ld'
		echo "$2 -n $3 -T $4" '$@' "$5" >> 'ld'
		chmod a+x 'ld'
		;;
	"objdump")
		echo '#! /bin/bash' > 'objdump'
		echo "$2" '$@' >> 'objdump'
		chmod a+x 'objdump'
		;;
	"objcopy")
		echo '#! /bin/bash' > 'objcopy'
		echo "$2" '$@' >> 'objcopy'
		chmod a+x 'objcopy'
		;;
	"strip")
		echo '#! /bin/bash' > 'strip'
		echo "$2" '$@' >> 'strip'
		chmod a+x 'strip'
		;;
	*)
		;;
esac

