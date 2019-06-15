
if h_arch == 'ia64'
	link_args += [ '-T', meson.current_source_dir() / 'elf64_ia64_loader.x' ]
else
	link_args += '-Wl,--Ttext-segment=0x70000000'
endif

src = files('main.c')
