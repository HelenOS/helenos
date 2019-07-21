#!/bin/sh

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
