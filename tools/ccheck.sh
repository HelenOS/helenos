#!/bin/sh
#
# SPDX-FileCopyrightText: 2018 Jiri Svoboda
#
# SPDX-License-Identifier: BSD-3-Clause
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
