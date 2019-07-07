# TODO: Use vcs_tag() to generate version string
# TODO: jobfile
# TODO: lto
# TODO: fix clang build

project(
    'HelenOS',
    [ 'c', 'cpp' ],
    default_options : ['buildtype=plain', 'c_std=gnu11', 'cpp_std=c++17', 'warning_level=3', 'werror=false', 'b_staticpic=false', 'prefix=/' ],
    meson_version: '>=0.50.1',
)

debug_options = false
debug_shell_scripts = false

if not meson.is_cross_build()
	subdir('doxygen')
	subdir_done()
endif

cc = meson.get_compiler('c')
basename = find_program('basename')
cp = find_program('cp')
dirname = find_program('dirname')
find = find_program('find')
grep = find_program('grep')
mkarray = find_program('tools/mkarray_for_meson.sh')
mkext4 = find_program('tools/mkext4.py')
mkfat = find_program('tools/mkfat.py')
mkuimage = find_program('tools/mkuimage.py')
config_py = find_program('tools/config.py')
objcopy = find_program('objcopy')
objdump = find_program('objdump')
sed = find_program('sed')
unzip = find_program('unzip')
which = find_program('which')

sh = [ find_program('sh'), '-u', '-e' ]
if debug_shell_scripts
	sh += '-x'
endif

genisoimage = find_program('genisoimage', required: false)

if not genisoimage.found()
	genisoimage = find_program('mkisofs', required: false)
endif

if not genisoimage.found()
	xorriso = find_program('xorriso', required: false)

	if xorriso.found()
		genisoimage = [ xorriso, '-as', 'genisoimage' ]
	else
		error('Need genisoimage, mkisofs or xorriso.')
	endif
endif


autocheck = generator(find_program('tools/autocheck.awk'),
	arguments: [ '@INPUT@' ],
	output: '@PLAINNAME@.check.c',
	capture: true,
)

# TODO: bug in Meson
#gzip = generator(find_program('gzip'),
#	arguments: [ '--no-name', '-9', '--stdout', '@INPUT@' ],
#	output: '@PLAINNAME@.gz',
#	capture: true,
#)

gzip = [ find_program('gzip'), '--no-name', '-9', '--stdout', '@INPUT@' ]

# Tar's arguments make sure that the archive is reproducible.
tar = [
	find_program('tar'),
	'-c',
	'-f', '@OUTPUT@',
	'--mtime=1970-01-01 00:00:00Z',
	'--group=0',
	'--owner=0',
	'--numeric-owner',
	'--mode=go=rX,u+rw,a-s',
	'--no-acls',
	'--no-selinux',
	'--no-xattrs',
	'--format=ustar',
	'--transform', 's:@OUTDIR@/::',
	'@INPUT@',
]

# Output compiler flags for use by third-party builds.
# NOTE: See $srcroot/meson/cross/$arch for architecture-specific compiler flags.

if debug_options
	message('Cross c_args:')
	message(meson.get_cross_property('c_args'))
	message('Cross cpp_args:')
	message(meson.get_cross_property('cpp_args'))
	message('Cross c_link_args:')
	message(meson.get_cross_property('c_link_args'))
	message('Cross cpp_link_args:')
	message(meson.get_cross_property('cpp_link_args'))
endif

# Read some variables from Makefile.common
config_variables = [
	# Uspace and kernel
	'CONFIG_BAREBONE',
	'CONFIG_BUILD_SHARED_LIBS',
	'CONFIG_DEBUG',
	'CONFIG_DEVEL_FILES',
	'CONFIG_FPU',
	'CONFIG_LINE_DEBUG',
	'CONFIG_LTO',
	'CONFIG_PCUT_SELF_TESTS',
	'CONFIG_PCUT_TESTS',
	'CONFIG_RTLD',
	'CONFIG_STRIP_BINARIES',
	'CONFIG_UBSAN',
	'CONFIG_USE_SHARED_LIBS',
	'CROSS_TARGET',
	'OPTIMIZATION',
	'PROCESSOR',
	'QUADFLOAT',
	'RDFMT',

	# Kernel only
	'CONFIG_ACPI',
	'CONFIG_AM335X_TIMERS',
	'CONFIG_ASID',
	'CONFIG_ASID_FIFO',
	'CONFIG_AT_KBD',
	'CONFIG_BCM2835_MAILBOX',
	'CONFIG_DSRLNIN',
	'CONFIG_DSRLNOUT',
	'CONFIG_EGA',
	'CONFIG_FB',
	'CONFIG_GICV2',
	'CONFIG_I8042',
	'CONFIG_I8259',
	'CONFIG_IOMAP_BITMAP',
	'CONFIG_IOMAP_DUMMY',
	'CONFIG_KCONSOLE',
	'CONFIG_MAC_KBD',
	'CONFIG_MULTIBOOT',
	'CONFIG_NS16550',
	'CONFIG_OFW_PCI',
	'CONFIG_OFW_TREE',
	'CONFIG_OMAP_UART',
	'CONFIG_PAGE_HT',
	'CONFIG_PAGE_PT',
	'CONFIG_PC_KBD',
	'CONFIG_PL011_UART',
	'CONFIG_PL050',
	'CONFIG_S3C24XX_IRQC',
	'CONFIG_S3C24XX_UART',
	'CONFIG_SMP',
	'CONFIG_SOFTINT',
	'CONFIG_SRLN',
	'CONFIG_SUN_KBD',
	'CONFIG_SYMTAB',
	'CONFIG_TEST',
	'CONFIG_TRACE',
	'CONFIG_TSB',
	'CONFIG_UDEBUG',
	'CONFIG_VIA_CUDA',
	'MACHINE',
	'MEMORY_MODEL',

	'UARCH',
	'KARCH',
	'BARCH',
	'GRUB_ARCH',
	'UIMAGE_OS',
	'CONFIG_COMPRESSED_INIT',
]

CONFIG_MAKEFILE = files(meson.build_root() / 'Makefile.config')

foreach varname : config_variables
	result = run_command(grep, '^' + varname + '\\b', CONFIG_MAKEFILE)
	if result.returncode() != 0
		# TODO: Output negative/inapplicable variables too in config, so that we can check for typos here.
		#warning('Missing configuration variable ' + varname + ' in Makefile.config')
		set_variable(varname, false)
		continue
	endif

	value = result.stdout().split('\n')[0].strip().split('=')[1].strip()
	if value == 'y'
		value = true
	elif value == 'n'
		value = false
	endif

	set_variable(varname, value)
	if debug_options
		message([ varname, value ])
	endif
endforeach

# Also read version information.
version_variables = [
	'COPYRIGHT',
	'NAME',
	'PATCHLEVEL',
	'SUBLEVEL',
	'VERSION',
]

foreach varname : version_variables
	line = run_command(grep, '^' + varname + '\\b', meson.source_root() / 'version', check: true).stdout().strip()
	value = line.split('=')[1].strip()
	set_variable('HELENOS_' + varname, value)
	if debug_options
		message([ 'HELENOS_' + varname, value ])
	endif
endforeach

HELENOS_RELEASE = HELENOS_VERSION + '.' + HELENOS_PATCHLEVEL + '.' + HELENOS_SUBLEVEL

add_project_arguments(
	# TODO: Remove from project arguments and only use where necessary.
	# Any change in config forces everything to rebuild due to lack of granularity here.
	'-imacros', meson.build_root() / 'config.h',
	language : [ 'c' ],
)

add_project_link_arguments(
	cc.get_supported_link_arguments([
		'-Wl,--gc-sections',
		'-Wl,--warn-common',
	]),
	'-Wl,--fatal-warnings',
	language : [ 'c', 'cpp' ],
)

# TODO: enable more warnings
# FIXME: -fno-builtin-strftime works around seemingly spurious format warning.
# We should investigate what's going on there.

extra_common_flags = [
	'-O' + OPTIMIZATION,
	'-fexec-charset=UTF-8',
	'-finput-charset=UTF-8',

	'-D_HELENOS_SOURCE',

	'-Wa,--fatal-warnings',

	'-Wall',
	'-Wextra',
	'-Werror-implicit-function-declaration',
	'-Wwrite-strings',
	'-Wunknown-pragmas',

	'-Wno-unused-parameter',

	'-pipe',

	'-ffunction-sections',
	'-fno-common',
	'-fdebug-prefix-map=' + meson.source_root() + '=.',
]

if cc.get_id() != 'clang'
	# Clang's own headers emit macro redefinition warnings.
	extra_common_flags += '-Wsystem-headers'
endif

if UARCH != 'ia64'
	extra_common_flags += [ '-fvar-tracking-assignments' ]
endif

if CONFIG_DEBUG
	extra_common_flags += [ '-Werror' ]
endif

if CONFIG_LINE_DEBUG
	extra_common_flags += [ '-gdwarf-4', '-g3' ]
endif

extra_cflags = extra_common_flags + [
	'-Wmissing-prototypes',

	'-Wno-missing-braces',
	'-Wno-missing-field-initializers',
	'-Wno-unused-command-line-argument',
	'-Wno-unused-parameter',
	'-Wno-typedef-redefinition',
	'-Wno-clobbered',
	'-Wno-nonnull-compare',

	'-fno-builtin-strftime',
]

if CONFIG_UBSAN
	extra_cflags += '-fsanitize=undefined'
endif

extra_cppflags = extra_common_flags + [
	'-fno-exceptions',
	'-frtti',
]

w_flags = {
	'c': extra_cflags,
	'cpp': extra_cppflags,
}

# Process flags. Only sets those that compiler supports.
foreach lang : [ 'c', 'cpp' ]
	extra_cflags = meson.get_compiler(lang).get_supported_arguments(w_flags.get(lang))
	add_project_arguments(extra_cflags, language : [ lang ])
	add_project_link_arguments(extra_cflags, language : [ lang ])
endforeach

# Init binaries.
rd_init = [
	# IMPORTANT: The order of entries is important for bootloader!
	'srv/ns',
	'srv/loader',
	'app/init',
	'srv/locsrv',
	'srv/bd/rd',
	'srv/vfs',
	'srv/logger',
	'srv/fs/' + RDFMT,
]

# References to the actual binary files. Filled in by uspace.
rd_init_binaries = []

# Binaries allowed on the initrd image when CONFIG_BAREBONE is enabled.
rd_essential = [
	'app/bdsh',
	'app/getterm',
	'app/kio',

	'srv/devman',
	'srv/fs/locfs',
	'srv/hid/console',
	'srv/hid/input',
	'srv/hid/output',
	'srv/klog',

	'drv/root/root',
	'drv/root/virt',
	'drv/fb/kfb',
]

if CONFIG_FB
	rd_essential += [
		'app/vlaunch',
		'app/vterm',

		'srv/hid/compositor',
	]
endif


## The at-sign
#
# The `atsign` variable holds the ASCII character representing the at-sign
# ('@') used in various $(AS) constructs (e.g. @progbits). On architectures that
# don't use '@' for starting a comment, `atsign` is merely '@'. However, on
# those that do use it for starting a comment (e.g. arm32), `atsign` must be
# defined as the percentile-sign ('%') in the architecture-dependent files.
#
atsign = '@'

## Some architectures need a particular string at the beginning of assembly files.
kernel_as_prolog = ''
uspace_as_prolog = ''

subdir('meson' / 'arch' / UARCH)

install_files = []
install_deps = []

subdir('kernel')
subdir('uspace')


## Generate config.mk

# Get the directory where the compiler resides.

cc_fullname = run_command(which, meson.get_compiler('c').cmd_array()[0].split(' ')[0], check: true).stdout().strip()
cc_path = run_command(dirname, cc_fullname, check: true).stdout().strip()

message(['Compiler directory is:', cc_path])

export_cppflags = [
	'-isystem', '$(HELENOS_EXPORT_ROOT)/include/posix',
	'-isystem', '$(HELENOS_EXPORT_ROOT)/include/libc',
	'-idirafter', '$(HELENOS_EXPORT_ROOT)/include',
]

export_ldflags = [
	'-nostdlib',
	'-L$(HELENOS_EXPORT_ROOT)/lib',
	'-Wl,--whole-archive',
	'$(HELENOS_EXPORT_ROOT)/lib/libstartfiles.a',
	'-Wl,--no-whole-archive',
]

export_ldlibs = [
	'-Wl,--as-needed',
	'-lposix',
	'-lmath',
	'-lc',
	'-lgcc',
	'-Wl,--no-as-needed',
]

cc_arch = meson.get_cross_property('cc_arch')

conf_data = configuration_data({
	'HELENOS_CROSS_PATH' : cc_path,
	'HELENOS_ARCH' : cc_arch,
	'HELENOS_CFLAGS' : ' '.join(arch_uspace_c_args),
	'HELENOS_CXXFLAGS' : ' '.join(arch_uspace_c_args),
	'HELENOS_CPPFLAGS' : ' '.join(export_cppflags),
	'HELENOS_LDFLAGS' : ' '.join(export_ldflags),
	'HELENOS_LDLIBS' : ' '.join(export_ldlibs),
	'HELENOS_TARGET' : cc_arch + '-helenos',
})

config_mk = configure_file(
	input: 'config.mk.in',
	output: 'config.mk',
	configuration: conf_data,
)

config_sh = custom_target('config.sh',
	input: config_mk,
	output: 'config.sh',
	command: [ sed, 's:$(HELENOS_EXPORT_ROOT):${HELENOS_EXPORT_ROOT}:g', '@INPUT@' ],
	capture: true,
)

install_files += [[ 'config', meson.current_build_dir() / 'config.mk', 'config.mk' ]]
install_deps += [ config_mk ]
install_files += [[ 'config', meson.current_build_dir() / 'config.sh', 'config.sh' ]]
install_deps += [ config_sh ]

# TODO: remove
install_files += [[ 'config', meson.build_root() / 'Makefile.config', 'Makefile.config' ]]
install_deps += CONFIG_MAKEFILE

if CONFIG_DEVEL_FILES
	# Also install libgcc.
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
_uspace = meson.current_source_dir() / 'uspace'
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

dist_dir = meson.current_build_dir()/'dist/'

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

subdir('boot')

if is_variable('POST_INPUT')
	image = custom_target(POST_OUTPUT,
		output: POST_OUTPUT,
		input: POST_INPUT,
		command: [ cp, '@INPUT@', '@OUTPUT@' ],
	)

	custom_target('image_path',
		output: 'image_path',
		input: image,
		command: [ 'echo', '@INPUT@' ],
		capture: true,
	)
else
	custom_target('image_path',
		output: 'image_path',
		command: [ 'echo' ],
		capture: true,
	)
endif

run_target('config',
	command: [
		sh,
		'-c', 'cd $1 && $2 $3 $4',
		'--',
		meson.build_root(),
		config_py.path(),
		meson.source_root() / 'HelenOS.config',
		meson.source_root() / 'defaults',
	]
)

# TODO: Check when cross target has changed, since it won't work.

if false
	# TODO: doesn't work because it changes cross target
	run_target('random-config',
		command: [
			sh,
			'-c', 'cd $1 && $2 $3 $4 random',
			'--',
			meson.build_root(),
			config_py.path(),
			meson.source_root() / 'HelenOS.config',
			meson.source_root() / 'defaults',
		]
	)
endif


# TODO text-xcw
#ifeq ($(CONFIG_DEVEL_FILES),y)
#	export PATH=$$PATH:$(abspath tools/xcw/bin) && $(MAKE) -r -C tools/xcw/demo
#endif

# TODO special target for posix and xcw exports, update coastline for it
