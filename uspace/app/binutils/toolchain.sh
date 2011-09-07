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
# to the linker. There is also special case for configure script tests
# that require compiler link step.
#

case "$1" in
	"gcc")
		(
		echo '#! /bin/bash'
		echo 'AS_LINK="`echo \"$*\" | grep '\'as-new\''`"'
		echo 'LD_LINK="`echo \"$*\" | grep '\'ld-new\''`"'
		echo 'LINK="`echo -n "$AS_LINK""$LD_LINK"`"'
		echo 'if [ -n "$LINK" ]; then'
		echo '	LD_ARGS="`echo \" $*\" | \'
		echo '		sed '\'s/ -O[^ ]*//g\'' | \'
		echo '		sed '\'s/ -W[^ ]*//g\'' | \'
		echo '		sed '\'s/ -g//g\'' | \'
		echo '		sed '\'s/ -l[^ ]*//g\'' | \'
		echo '		sed '\'s/ [ ]*/ /g\''`"'
		echo '	ld $LD_ARGS'
		echo 'else'
		CFLAGS="`echo " $3" | \
			sed 's/ -O[^ ]*//g' | \
			sed 's/ -W[^ ]*//g' | \
			sed 's/ -pipe//g' | \
			sed 's/ -g//g' | \
			sed 's/ [ ]*/ /g'`"
		echo '	CONFTEST="`echo \"$*\" | grep '\' conftest \''`"'
		echo '	if [ -n "$CONFTEST" ]; then'
		echo '		LFLAGS="-Xlinker -z -Xlinker muldefs"'
		echo '		echo' \'"$2"\' '"$@"' \'"$CFLAGS -T $4"\' '"$LFLAGS"' \'"$5"\'
		echo "		$2" '$@' "$CFLAGS -T $4" '$LFLAGS' "$5"
		echo '	else'
					# Remove flags:
					# -Werror
					#		Avoid build failure due to some harmless bugs
					#		(e.g. unused parameter) in the HelenOS.
					# -Wc++-compat
					#		Required just for gold linker.
		echo '		GCC_ARGS="`echo \" $*\" | \'
		echo '			sed '\'s/ -Werror//g\'' | \'
		echo '			sed '\'s/ -Wc++-compat//g\'' | \'
		echo '			sed '\'s/ [ ]*/ /g\''`"'
					# Add flags:
					# -example
					#		Flag description.
#		echo '		GCC_ARGS="$GCC_ARGS -example"'
		echo '		echo' \'"$2"\' '"$GCC_ARGS"' \'"$CFLAGS"\'
		echo "		$2" '$GCC_ARGS' "$CFLAGS"
		echo '	fi'
		echo 'fi'
		) > 'gcc'
		chmod a+x 'gcc'
		;;
	"as")
		(
		echo '#! /bin/bash'
		echo 'echo' \'"$2"\' '"$@"'
		echo "$2" '$@'
		) > 'as'
		chmod a+x 'as'
		;;
	"ar")
		(
		echo '#! /bin/bash'
		echo 'echo' \'"$2"\' '"$@"'
		echo "$2" '$@'
		) > 'ar'
		chmod a+x 'ar'
		;;
	"ranlib")
		(
		echo '#! /bin/bash'
		echo 'echo' 'ar -s' '"$@"'
		echo 'ar -s' '$@'
		) > 'ranlib'
		chmod a+x 'ranlib'
		;;
	"ld")
		(
		echo '#! /bin/bash'
		echo 'echo' \'"$2 -n $3 -T $4"\' '"$@"' \'"$5"\'
		echo "$2 -n $3 -T $4" '$@' "$5"
		) > 'ld'
		chmod a+x 'ld'
		;;
	"objdump")
		(
		echo '#! /bin/bash'
		echo 'echo' \'"$2"\' '"$@"'
		echo "$2" '$@'
		) > 'objdump'
		chmod a+x 'objdump'
		;;
	"objcopy")
		(
		echo '#! /bin/bash'
		echo 'echo' \'"$2"\' '"$@"'
		echo "$2" '$@'
		) > 'objcopy'
		chmod a+x 'objcopy'
		;;
	"strip")
		(
		echo '#! /bin/bash'
		echo 'echo' \'"$2"\' '"$@"'
		echo "$2" '$@'
		) > 'strip'
		chmod a+x 'strip'
		;;
	*)
		;;
esac

