#!/bin/sh

#
# Copyright (c) 2014 Vojtech Horky
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
