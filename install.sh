#!/bin/sh

export CONFIG_DEVEL_FILES=$1

(
	# Install generated map files into the 'debug' subdirectory.

	echo "######## Installing library map files ########"

	# TODO: add configuration option to install debug files

	if false; then
		cd ${MESON_BUILD_ROOT}/uspace
		find -name '*.map' -a -path './lib/*' | sed 's:^\./::' | xargs --verbose -I'@' install -C -D -m644 -T '@' "${MESON_INSTALL_DESTDIR_PREFIX}debug/"'@'
	fi

	# Install library headers that are mixed in with source files (no separate 'include' subdir).
	# The properly separated headers are installed by the meson script.

) > ${MESON_BUILD_ROOT}/install_custom.log 2>&1
