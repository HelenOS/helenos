# Find all the programs and tools used by meson.
# Whenever you use an external command anywhere,
# first find and assign it to a variable here. Don't use its name directly.
# This way it's obvious what programs are used, and for some we can use
# the same argument list everywhere.

_tools_dir = meson.source_root() / 'tools'

basename = find_program('basename')
cc = meson.get_compiler('c')
config_py = find_program(_tools_dir / 'config.py')
cp = find_program('cp')
dirname = find_program('dirname')
doxygen = find_program('doxygen')
find = find_program('find')
grep = find_program('grep')
mkarray = find_program(_tools_dir / 'mkarray_for_meson.sh')
mkext4 = find_program(_tools_dir / 'mkext4.py')
mkfat = find_program(_tools_dir / 'mkfat.py')
mkuimage = find_program(_tools_dir / 'mkuimage.py')
objcopy = find_program('objcopy')
objdump = find_program('objdump')
patch = find_program('patch')
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


autocheck = generator(find_program(_tools_dir / 'autocheck.awk'),
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
