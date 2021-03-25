#
# Copyright (c) 2015 Jiri Svoboda
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
