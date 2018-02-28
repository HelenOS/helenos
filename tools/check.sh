#! /bin/bash

#
# Copyright (c) 2010 Jakub Jermar
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

if [ $1" " == "-h " ];
then
	echo "Perform pre-integration hands-off build of all profiles."
	echo
	echo "Syntax:"
	echo " $0 [-h] [args...]"
	echo
	echo " -h        Print this help."
	echo " args...   All other args are passed to make (e.g. -j 6)"
	echo

	exit
fi

FAILED=""
PASSED=""
PROFILES=""
DIRS=`find defaults/ -name Makefile.config | sed 's/^defaults\/\(.*\)\/Makefile.config/\1/' | sort`

for D in $DIRS;
do
	for H in $DIRS;
	do
		if [ `echo $H | grep "^$D\/.*"`x != "x"  ];
		then
			continue 2
		fi
	done
	PROFILES="$PROFILES $D"
done

echo ">>> Going to build the following profiles:"
echo $PROFILES

for P in $PROFILES;
do
	echo -n ">>>> Building $P... "
	( make distclean && make PROFILE=$P HANDS_OFF=y "$@" ) >>/dev/null 2>>/dev/null
	if [ $? -ne 0 ];
	then
		FAILED="$FAILED $P"
		echo "failed."
	else
		PASSED="$PASSED $P"
		echo "ok."
	fi
done

echo ">>> Done."
echo

echo ">>> The following profiles passed:"
echo $PASSED
echo

echo ">>> The following profiles failed:"
echo $FAILED
echo

