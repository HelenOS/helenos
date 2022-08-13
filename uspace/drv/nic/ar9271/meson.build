#
# SPDX-FileCopyrightText: 2014 Jan Kolarik
#
# SPDX-License-Identifier: BSD-3-Clause
#

deps = [ 'usbdev', 'usb', 'nic', 'ieee80211', 'crypto' ]
src = files(
	'ath_usb.c',
	'hw.c',
	'wmi.c',
	'htc.c',
	'ar9271.c',
)

installed_data += { 'name': 'ar9271.fw', 'dir': 'drv/ar9271' }
