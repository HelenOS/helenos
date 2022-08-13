#
# SPDX-FileCopyrightText: 2013 Vojtech Horky
#
# SPDX-License-Identifier: BSD-3-Clause
#

SOURCES = \
	src/os/helenos.c \
	src/assert.c \
	src/helper.c \
	src/list.c \
	src/main.c \
	src/print.c \
	src/report/report.c \
	src/report/tap.c \
	src/report/xml.c \
	src/run.c

EXTRA_CFLAGS = -D__helenos__ -Wno-unknown-pragmas

LIBRARY = libpcut
