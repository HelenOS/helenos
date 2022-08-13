#
# SPDX-FileCopyrightText: 2015 Jiri Svoboda
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Example cross-build for HelenOS using XCW (HelenOS Cross Compiler Wrapper)
# This allows an external (cross-platform) application to be compiled for
# HelenOS by changing the compiler/linker helenos-cc/helenos-ld and adding
# platform-specific support. To configure use of HelenOS libraries one can use
# helenos-pkg-config. The HelenOS source tree containing XCW must be configured
# and build for XCW to work as a cross compiler.
#
# Example commands to build this demo:
#
#    cd <helenos-source-dir>
#    mkdir build
#    cd build
#    ../configure.sh
#    ninja
#    ../tools/export.sh export
#    cd ../tools/xcw/demo
#    export PATH=$PATH:<helenos-source-dir>/build/tools/xcw/bin
#    make
#

CC = helenos-cc
LD = helenos-ld
INSTALL = install
TEST = helenos-test
CFLAGS = -std=gnu11 -Wall `helenos-pkg-config --cflags libui libgfximage` \
    -D_HELENOS_SOURCE
LIBS = `helenos-pkg-config --libs libui libgfximage`
PREFIX = `helenos-bld-config --install-dir`
output = viewer
objects = viewer.o

default: $(output)
all: $(output)

clean:
	rm -f $(output) $(objects)

install: $(output)
	mkdir -p $(PREFIX)/app
	$(INSTALL) -T $(output) $(PREFIX)/app/$(output)-xcw

uninstall:
	rm -f $(PREFIX)/app/$(output)-xcw

test: install
	$(TEST)

$(output): $(objects)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
