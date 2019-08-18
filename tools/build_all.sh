#!/bin/sh

#
# Copyright (c) 2019 Jiří Zárevúcky
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

# Find out the path to the script.
SOURCE_DIR=`which -- "$0" 2>/dev/null`
# Maybe we are running bash.
[ -z "$SOURCE_DIR" ] && SOURCE_DIR=`which -- "$BASH_SOURCE"`
[ -z "$SOURCE_DIR" ] && exit 1
SOURCE_DIR=`dirname -- "$SOURCE_DIR"`
SOURCE_DIR=`cd $SOURCE_DIR && cd .. && echo $PWD`

# Check command line arguments.

if [ "$#" -gt 1 ] || ( [ "$#" -eq 1 ] && [ "$1" != '--no-images' ] ); then
	echo "Unknown command-line arguments."
	echo "Usage:"
	echo "    $0                    # Build everything."
	echo "    $0 --no-images        # Build all code, but don't create bootable images."
	exit 1
fi

if [ "$#" -eq 1 ]; then
	NO_IMAGES=true
else
	NO_IMAGES=false
fi

# Make sure we don't make a mess in the source root.
if [ "$PWD" = "$SOURCE_DIR" ]; then
	mkdir -p build_all
	cd build_all
fi

CONFIG_RULES="${SOURCE_DIR}/HelenOS.config"

PROFILES=`sh ${SOURCE_DIR}/tools/list_profiles.sh`

echo
echo "###################### Configuring all profiles ######################"

echo "Configuring profiles" $PROFILES

for profile in $PROFILES; do
	# echo "Configuring profile ${profile}"

	mkdir -p ${profile} || exit 1
	script -q -e /dev/null -c "cd '${profile}' && '${SOURCE_DIR}/configure.sh' '${profile}' && ninja build.ninja" </dev/null >"${profile}/configure_output.log" 2>&1  &
	echo "$!" >"${profile}/configure.pid"
done

failed='no'

for profile in $PROFILES; do
	if ! wait `cat "${profile}/configure.pid"`; then
		failed='yes'
		cat "${profile}/configure_output.log"
		echo
		echo "Configuration of profile ${profile} failed."
		echo
	fi
done

if [ "$failed" = 'yes' ]; then
	echo
	echo "Some configuration jobs failed."
	exit 1
else
	echo "All profiles configured."
fi

echo
echo "###################### Building all profiles ######################"

for profile in $PROFILES; do
	echo
	ninja -C ${profile} || exit 1
done

if [ "$NO_IMAGES" = 'true' ]; then
	echo
	echo "Bootable images not built due to argument --no-images."
	exit 0
fi

echo
echo "###################### Building all images ######################"

for profile in $PROFILES; do
	echo
	ninja -C ${profile} image_path || exit 1
done

echo
for profile in $PROFILES; do
	path=`cat ${profile}/image_path`

	if [ ! -z "$path" ]; then
		echo "built ${profile}/${path}"
	fi
done
