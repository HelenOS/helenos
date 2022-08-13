#!/bin/sh
#
# SPDX-FileCopyrightText: 2014 Vojtech Horky
#
# SPDX-License-Identifier: BSD-3-Clause
#


LOOPS=1
JOBS=1
MAX_RETRIES=20
FILENAME_PREFIX=random_run_
PRUNE_CONFIG_FILE=${FILENAME_PREFIX}prune.config

echo -n "">"$PRUNE_CONFIG_FILE"

while getopts n:j:x:hs option; do
	case $option in
	n)
		LOOPS="$OPTARG"
		;;
	j)
		JOBS="$OPTARG"
		;;
	x)
		echo "$OPTARG" | tr -d ' ' >>"$PRUNE_CONFIG_FILE"
		;;
	s)
		;;
	*|h)
		echo "Usage: $0 [options]"
		echo "where [options] could be:"
		echo
		echo "  -n LOOPS"
		echo "      How many configurations to check."
		echo "      (Default is one configuration.)"
		echo "  -j PARALLELISM"
		echo "      How many parallel jobs to execute by 'make'."
		echo "      (Default is single job configuration.)"
		echo "  -s"
		echo "      Use only supported compilers."
		echo "      (That is, only GCC cross-compiler and Clang.)"
		echo "  -x CONFIG_OPTION=value"
		echo "      Skip the configuration if this option is present."
		echo "      (Example: CONFIG_BINUTILS=n means that only configurations"
		echo "      where binutils *are* included would be built.)"
		echo "  -h"
		echo "      Print this help and exit."
		echo
		if [ "$option" = "h" ]; then
			exit 0
		else
			exit 1
		fi
		;;
	esac
done


COUNTER=0
FAILED=0
while [ $COUNTER -lt $LOOPS ]; do
	COUNTER=$(( $COUNTER + 1 ))
	echo "Try #$COUNTER ($FAILED failed):" >&2

	(
		echo "  Cleaning after previous build." >&2
		make distclean -j$JOBS 2>&1 || exit 1


		echo "  Preparing random configuration." >&2
		# It would be nicer to allow set the constraints directly to
		# the tools/config.py script but this usually works.
		# We retry $MAX_RETRIES before aborting this run completely.
		RETRIES=0
		while true; do
			RETRIES=$(( $RETRIES + 1 ))
			if [ $RETRIES -ge $MAX_RETRIES ]; then
				echo "  Failed to generate random configuration with given constraints after $RETRIES tries." >&2
				exit 2
			fi

			make random-config 2>&1 || exit 1

			tr -d ' ' <Makefile.config >"${FILENAME_PREFIX}config.trimmed"

			THIS_CONFIG_OKAY=true
			while read pattern; do
				if grep -q -e "$pattern" "${FILENAME_PREFIX}config.trimmed"; then
					THIS_CONFIG_OKAY=false
					break
				fi
			done <"$PRUNE_CONFIG_FILE"

			rm -f "${FILENAME_PREFIX}config.trimmed"

			if $THIS_CONFIG_OKAY; then
				break
			fi
		done


		# Report basic info about the configuration and build it
		BASIC_CONFIG=`sed -n \
			-e 's#PLATFORM = \(.*\)#\1#p' \
			-e 's#MACHINE = \(.*\)#\1#p' \
			-e 's#COMPILER = \(.*\)#\1#p' \
			Makefile.config \
			| paste '-sd,' | sed 's#,#, #g'`
		echo -n "  Building ($BASIC_CONFIG)... " >&2

		make -j$JOBS 2>&1
		if [ $? -eq 0 ]; then
			echo "okay." >&2
			exit 0
		else
			echo "failed." >&2
			exit 1
		fi

	) >random_run_$COUNTER.log
	RC=$?

	if [ $RC -ne 0 ]; then
		tail -n 10 random_run_$COUNTER.log | sed 's#.*#    | &#'
		FAILED=$(( $FAILED + 1 ))
	fi

	if [ -e Makefile.config ]; then
		cp Makefile.config "$FILENAME_PREFIX$COUNTER.Makefile.config"
		cp config.h "$FILENAME_PREFIX$COUNTER.config.h"
	fi
done

rm "$PRUNE_CONFIG_FILE"

echo "Out of $LOOPS tries, $FAILED configurations failed to compile." >&2
