#!/bin/bash
#
# SPDX-FileCopyrightText: 2016 Jiri Svoboda
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Fake xorriso utility for extracting El Torrito boot image passed to it
# by the caller
#

while [ ."$1" != . ] ; do
	if [ ."$1" == .-b ] ; then
		shift 1
		image="$1"
		output=pc.img
	elif [ ."$1" == .--efi-boot ] ; then
		shift 1
		image="$1"
		output=efi.img
	elif [ ."$1" == .-r ] ; then
		shift 1
		root="$1"
	fi
	shift 1
done

echo image is "$root/$image"
cp "$root/$image" "$output"
