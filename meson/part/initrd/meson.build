if CONFIG_DEVEL_FILES
	# If devel files are requested, we have to install libgcc.
	# We have to explicitly use gcc for this, because clang only prints
	# file name instead of whole path.
	libgcc = run_command(cc_arch + '-helenos-gcc',
		arch_uspace_c_args, '-print-libgcc-file-name',
		check: true,
	).stdout().strip()

	install_files += [[ 'lib', libgcc, 'libgcc.a' ]]
	install_deps += [ files(libgcc) ]
endif

# Emit the install script.

install_script_text = []

# Copy uspace/dist.
_uspace = meson.source_root() / 'uspace'
install_script_text += 'cp -r -L -T -u "@0@/dist" "${DESTDIR}"'.format(_uspace)

# Copy uspace/overlay
install_script_text += 'if ls @0@/overlay/* >/dev/null 2>/dev/null; then'.format(_uspace)
install_script_text += 'cp -r -L @0@/overlay/* "${DESTDIR}"'.format(_uspace)
install_script_text += 'fi'


foreach f : install_files
	_cmd = 'mkdir -p "${DESTDIR}@0@" && cp -L -T "@1@" "${DESTDIR}@0@/@2@"'
	install_script_text += _cmd.format(f[0], f[1], f[2])
endforeach

install_script_text += uspace_lib_install_script_text

install_script = configure_file(
	configuration: { 'text' : '\n'.join(install_script_text) },
	input: 'install.sh.in',
	output: 'install.sh',
)

# Build up dist

dist_dir = meson.build_root() / 'dist/'

dist = custom_target('DIST',
	output: 'dist.tag',
	input: [ install_script, install_deps ],
	command: [ sh, '@INPUT0@', '@OUTPUT@', dist_dir ],
)

# Build initrd image

if RDFMT == 'tmpfs'
	initrd_cmd = [ 'tar', '-c', '-f', '@OUTPUT@', '-C', dist_dir, '.' ]
elif RDFMT == 'fat'
	initrd_cmd = [ mkfat, '1048576', dist_dir, '@OUTPUT@' ]
elif RDFMT == 'ext4fs'
	initrd_cmd = [ mkext4, '1048576', dist_dir, '@OUTPUT@' ]
else
	error('Unknown RDFMT: ' + RDFMT)
endif

initrd_img = custom_target('initrd.img',
	output: 'initrd.img',
	input: dist,
	command: initrd_cmd,
)

rd_init_binaries += [[ initrd_img, 'boot/initrd.img' ]]
