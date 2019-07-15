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
