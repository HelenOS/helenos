deps = [ 'usbdev', 'usb', 'nic', 'ieee80211', 'crypto' ]
src = files(
	'ath_usb.c',
	'hw.c',
	'wmi.c',
	'htc.c',
	'ar9271.c',
)

install_data('ar9271.fw', install_dir: 'drv' / 'ar9271')
