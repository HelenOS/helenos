#!/bin/sh

# Find out the path to the script.
SOURCE_DIR=`which -- "$0" 2>/dev/null`
# Maybe we are running bash.
[ -z "$SOURCE_DIR" ] && SOURCE_DIR=`which -- "$BASH_SOURCE"`
[ -z "$SOURCE_DIR" ] && exit 1
SOURCE_DIR=`dirname -- "$SOURCE_DIR"`
SOURCE_DIR=`cd $SOURCE_DIR && echo $PWD`

# Make sure we don't make a mess in the source root.
if [ "$PWD" = "$SOURCE_DIR" ]; then
	mkdir -p build_all
	cd build_all
fi

CONFIG_RULES="${SOURCE_DIR}/HelenOS.config"
CONFIG_DEFAULTS="${SOURCE_DIR}/defaults"

# Find all the leaf subdirectories in the defaults directory.
PROFILES=`find ${CONFIG_DEFAULTS} -type d -links 2 -printf "%P\n" | sort`


echo
echo "###################### Configuring all profiles ######################"

echo "Configuring profiles" $PROFILES

for profile in $PROFILES; do
	# echo "Configuring profile ${profile}"
	if [ -f ${profile}/build.ninja ]; then
		script -q -e /dev/null -c "ninja -C '${profile}' build.ninja" </dev/null >"${profile}/configure_output.log" 2>&1  &
		echo "$!" >"${profile}/configure.pid"
		continue
	fi

	# Let HelenOS config tool write out Makefile.config and config.h.
	mkdir -p ${profile} || exit 1
	cd ${profile} || exit 1
	"${SOURCE_DIR}/tools/config.py" "${CONFIG_RULES}" "${CONFIG_DEFAULTS}" hands-off "${profile}" || exit 1
	cd -


	if [ "$profile" = 'special/abs32le' ]; then
		cross_target='ia32'
	else
		cross_target=`dirname $profile`
		if [ "$cross_target" = '.' ]; then
			cross_target=$profile
		fi
	fi

	script -q -e /dev/null -c "meson '${SOURCE_DIR}' '${profile}' --cross-file '${SOURCE_DIR}/meson/cross/${cross_target}'" </dev/null >"${profile}/configure_output.log" 2>&1 &
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
	ninja -C ${profile} || exit 1
done


if [ "$#" -eq 1 ] && [ "$1" = 'images' ]; then
	echo
	echo "###################### Building all images ######################"

	for profile in $PROFILES; do
		ninja -C ${profile} image_path || exit 1
	done
else
	echo
	echo "Bootable images not built."
	echo "Run '$0 images' to build them as well."
fi
