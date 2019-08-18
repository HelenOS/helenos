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


echo "Running tools/build_all.sh"

mkdir -p build_all
cd build_all
sh "${SOURCE_DIR}/tools/build_all.sh"
cd ..

echo
echo

PROFILES=`sh ${SOURCE_DIR}/tools/list_profiles.sh`
RELEASE=`sed -n 's:^HELENOS_RELEASE \?= \?\(.*\)$:\1:p' "${SOURCE_DIR}/version"`
SRC_ARCHIVE="HelenOS-${RELEASE}-src.tar"

git -C "${SOURCE_DIR}" archive master -o "${PWD}/${SRC_ARCHIVE}"
bzip2 -f "${SRC_ARCHIVE}"
echo "Created ${SRC_ARCHIVE}.bz2"

for profile in $PROFILES; do
	image_name=`cat build_all/$profile/image_path`
	if [ -z "$image_name" ]; then
		continue
	fi

	image_path="build_all/$profile/`cat build_all/$profile/image_path`"
	image_suffix=`echo "$image_name" | sed 's:.*\.::'`

	release_name="HelenOS-${RELEASE}-`echo $profile | tr '/' '-'`.$image_suffix"
	cp "$image_path" "$release_name"

	echo "Created $release_name"
done
