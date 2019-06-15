deps = [ 'drv', 'hound', 'pcm' ]
src = files(
	'dplay.c',
	'drec.c',
	'main.c',
	'wave.c',
)

if install_nonessential_data
	# TODO: install this file somewhere sane
	install_data('demo.wav', install_dir: '/')
endif
