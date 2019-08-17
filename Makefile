#
# Copyright (c) 2006 Martin Decky
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

CSCOPE = cscope
FORMAT = clang-format

ERRNO_HEADER = abi/include/abi/errno.h
ERRNO_INPUT = abi/include/abi/errno.in

.PHONY: nothing cscope cscope_parts format ccheck ccheck-fix space check_errno

nothing:

cscope:
	find abi kernel boot uspace -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE).out

cscope_parts:
	find abi -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE)_abi.out
	find kernel -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE)_kernel.out
	find boot -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE)_boot.out
	find uspace -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE)_uspace.out

format:
	find abi kernel boot uspace -type f -regex '^.*\.[ch]$$' | xargs $(FORMAT) -i -sort-includes -style=file

ccheck:
	cd tools && ./build-ccheck.sh
	tools/ccheck.sh

ccheck-fix:
	cd tools && ./build-ccheck.sh
	tools/ccheck.sh --fix

space:
	tools/srepl '[ \t]\+$$' ''

# `sed` pulls a list of "compatibility-only" error codes from `errno.in`,
# the following grep finds instances of those error codes in HelenOS code.
check_errno:
	@ ! cat abi/include/abi/errno.in | \
	sed -n -e '1,/COMPAT_START/d' -e 's/__errno_entry(\([A-Z0-9]\+\).*/\\b\1\\b/p' | \
	git grep -n -f - -- ':(exclude)abi' ':(exclude)uspace/lib/posix'

$(ERRNO_HEADER): $(ERRNO_INPUT)
	echo '/* Generated file. Edit errno.in instead. */' > $@.new
	sed 's/__errno_entry(\([^,]*\),\([^,]*\),.*/#define \1 __errno_t(\2)/' < $< >> $@.new
	mv $@.new $@
