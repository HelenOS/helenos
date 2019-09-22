#!/bin/sh
#
# Copyright (c) 2018 Jiri Svoboda
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

ccheck=tools/sycek/ccheck
if [ ."$CCHECK" != . ]; then
	ccheck="$CCHECK"
fi

if [ ."$1" = .--fix ]; then
	opt=--fix
else
	opt=
fi

srepcnt=0
snorepcnt=0
fcnt=0

find abi kernel boot uspace -type f -regex '^.*\.[ch]$' \
  | grep -v -e '^uspace/lib/pcut/' \
  | (
while read fname; do
	outfile="$(mktemp)"
	"$ccheck" $opt $fname >"$outfile" 2>&1
	if [ $? -eq 0 ]; then
		if [ -s "$outfile" ] ; then
			srepcnt=$((srepcnt + 1))
			cat "$outfile"
		else
			snorepcnt=$((snorepcnt + 1))
		fi
	else
		fcnt=$((fcnt + 1))
		cat "$outfile"
	fi

	rm -f "$outfile"
done

if [ $srepcnt -eq 0 ] && [ $fcnt -eq 0 ]; then
	echo "Ccheck passed."
else
	echo "Ccheck failed."
	echo "Checked files with issues: $srepcnt"
	echo "Checked files without issues: $snorepcnt"
	echo "Files with parse errors: $fcnt"
	exit 1
fi
)
