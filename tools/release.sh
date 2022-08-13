#!/bin/sh
#
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
