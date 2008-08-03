#!/bin/bash
#
# Copyright (c) 2008 Jakub Jermar 
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

MFORMAT=`which mformat`
MCOPY=`which mcopy`

if [ `which mformat` == "" ]; then
	echo "Please install mtools."
	exit 1
fi

if [ `which mcopy` == "" ]; then
	echo "Please install mtools."
	exit 1
fi

if [ $# -ne 2 ]; then
	echo "Usage: $0 <PATH> <IMAGE>"
	exit 1
fi

if [ ! -d $1 ]; then
	echo "Usage: $0 <PATH> <IMAGE>"
	exit 1
fi

BPS=512
SPC=4
FAT16_MIN_SEC=$((4085 * $SPC))
HEADS=2
TRACKS=16
BYTES=`du -sb $1 | cut -f 1`
SECTORS=$(($BYTES / $BPS))
SPTPH=$((($SECTORS + $FAT16_MIN_SEC) / ($HEADS * $TRACKS)))

# Format the image as FAT16
$MFORMAT -h $HEADS -t $TRACKS -s $SPTPH -M $BPS -c $SPC -v "FAT16HORD" -B /dev/zero -C -i $2 ::

if [ $? -ne 0 ]; then
	echo "$MFORMAT failed: $?"
	exit $?
fi

# Copy the subtree to the image
$MCOPY -vspQmi $2 $1/* ::

exit $?
