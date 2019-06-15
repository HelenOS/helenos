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

	# TODO We don't currently install them all due to image size issues
	installed_libs="lib/c lib/gui lib/draw lib/softrend"

	if $CONFIG_DEVEL_FILES; then
		echo "######## Installing headers ########"
		cd ${MESON_SOURCE_ROOT}/uspace

		incdir="${MESON_INSTALL_DESTDIR_PREFIX}include"

		for libdir in ${installed_libs}; do
			if [ -d ${libdir} -a ! -d ${libdir}/include ]; then
				find ${libdir} -maxdepth 1 -name '*.h' -a '!' -path ${libdir}'/doc/*' | sed 's:^lib/::' | xargs --verbose -I'@' install -C -D -m644 -T 'lib/@' ${incdir}'/lib@'
			fi
		done
	fi

	# Due to certain quirks of our build, executables need to be built with a different name than what they are installed with.
	# Meson doesn't support renaming installed files (at least not as of mid-2019) so we do it here manually.

	echo "######## Installing executables ########"

	cd ${MESON_BUILD_ROOT}/uspace

	find -name 'install@*' |
		sed -e 'h; s:^.*/install@:@DESTDIR@:; s:\$:/:g; x; G; s:\s: :g' -e "s:@DESTDIR@:${MESON_INSTALL_DESTDIR_PREFIX}:g" |
		xargs -L1 --verbose install -C -D -m755 -T

) > ${MESON_BUILD_ROOT}/install_custom.log 2>&1
