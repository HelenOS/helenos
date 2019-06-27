deps = [ 'drv', 'hound', 'pcm' ]
src = files(
	'dplay.c',
	'drec.c',
	'main.c',
	'wave.c',
)

if install_nonessential_data
	# TODO: install this file somewhere sane
	installed_data += { 'name': 'demo.wav', 'dir': '/' }
endif
