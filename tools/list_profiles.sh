#!/bin/sh
#
# SPDX-FileCopyrightText: 2021 Jiri Svoboda
# SPDX-FileCopyrightText: 2019 Jiří Zárevúcky
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Find out the path to the script.
SOURCE_DIR=`which -- "$0" 2>/dev/null`
# Maybe we are running bash.
[ -z "$SOURCE_DIR" ] && SOURCE_DIR=`which -- "$BASH_SOURCE"`
[ -z "$SOURCE_DIR" ] && exit 1
SOURCE_DIR=`dirname -- "$SOURCE_DIR"`
SOURCE_DIR=`cd $SOURCE_DIR && cd .. && echo $PWD`

CONFIG_DEFAULTS="${SOURCE_DIR}/defaults"

# Find the leaf subdirectories in the defaults directory.
level1dirs=`ls "$CONFIG_DEFAULTS"`
for dname in $level1dirs; do
	l2dirs=`ls "$CONFIG_DEFAULTS"/"$dname"`
	havel2=
	for dn in $l2dirs; do
		if [ -d "$CONFIG_DEFAULTS/$dname/$dn" ]; then
			echo "$dname/$dn"
			havel2=true
		fi
	done
	if [ -z "$havel2" ]; then
		echo "$dname"
	fi
done
