#
# Copyright (c) 2011 Petr Koupy
# Copyright (c) 2011 Jiri Zarevucky
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

USPACE_PREFIX = ../..
LIBRARY = libposix

EXTRA_CFLAGS = -Iinclude/posix -D_XOPEN_SOURCE

EXPORT_FILES = \
	../math/libmath.a \
	../clui/libclui.a \
	../gui/libgui.a \
	../draw/libdraw.a \
	../softrend/libsoftrend.a \
	../hound/libhound.a \
	../pcm/libpcm.a \
	$(LIBC_PREFIX)/libc.a \
	$(LIBC_PREFIX)/crt0.o \
	$(LIBC_PREFIX)/crt1.o \
	$(LIBRARY).a

PRE_DEPEND = $(INCLUDE_LIBC)
EXTRA_CLEAN = $(INCLUDE_LIBC)

SOURCES = \
	src/ctype.c \
	src/dlfcn.c \
	src/fcntl.c \
	src/fnmatch.c \
	src/locale.c \
	src/pthread/condvar.c \
	src/pthread/keys.c \
	src/pthread/mutex.c \
	src/pthread/threads.c \
	src/pwd.c \
	src/signal.c \
	src/stdio.c \
	src/stdlib.c \
	src/string.c \
	src/strings.c \
	src/sys/mman.c \
	src/sys/stat.c \
	src/sys/wait.c \
	src/time.c \
	src/unistd.c

TEST_SOURCES = \
	test/main.c \
	test/stdio.c \
	test/stdlib.c \
	test/unistd.c

EXTRA_TEST_CFLAGS = -Wno-deprecated-declarations

EXPORT_CPPFLAGS = \
	-isystem $$(HELENOS_EXPORT_ROOT)/include/posix \
	-isystem $$(HELENOS_EXPORT_ROOT)/include/libc \
	-idirafter $$(HELENOS_EXPORT_ROOT)/include \
	$(COMMON_CPPFLAGS)

EXPORT_LDFLAGS = \
	-L$$(HELENOS_EXPORT_ROOT)/lib \
	$$(HELENOS_EXPORT_ROOT)/lib/crt0.o \
	$$(HELENOS_EXPORT_ROOT)/lib/crt1.o

EXPORT_LDLIBS = \
	-Wl,--start-group -lposix -lmath -lc -lgcc -Wl,--end-group

EXPORT_CFLAGS = \
	$(COMMON_CFLAGS)

include $(USPACE_PREFIX)/Makefile.common

export: $(EXPORT_DIR)/config.mk $(EXPORT_DIR)/config.rc \
    $(EXPORT_DIR)/Makefile.common $(EXPORT_DIR)/Makefile.config

$(EXPORT_DIR)/config.mk: export-libs export-includes
	echo '# Generated file, do not modify.' >> $@.new
	echo '# Do not forget to set HELENOS_EXPORT_ROOT.' >> $@.new
	echo 'HELENOS_CROSS_PATH="$(shell dirname `which $(CC)`)"' >> $@.new
	echo 'HELENOS_ARCH="$(firstword $(subst -, ,$(TARGET)))"' >> $@.new
	echo 'HELENOS_TARGET="$(TARGET)"' >> $@.new
	echo 'HELENOS_CPPFLAGS="$(EXPORT_CPPFLAGS)"' >> $@.new
	echo 'HELENOS_CFLAGS="$(EXPORT_CFLAGS)"' >> $@.new
	echo 'HELENOS_CXXFLAGS="$(EXPORT_CFLAGS)"' >> $@.new
	echo 'HELENOS_LDFLAGS="$(EXPORT_LDFLAGS)"' >> $@.new
	echo 'HELENOS_LDLIBS="$(EXPORT_LDLIBS)"' >> $@.new
	mv $@.new $@

$(EXPORT_DIR)/config.rc: $(EXPORT_DIR)/config.mk
	sed 's:$$(HELENOS_EXPORT_ROOT):$$HELENOS_EXPORT_ROOT:g' < $< >$@

$(EXPORT_DIR)/Makefile.common: ../../../Makefile.common
	cp $< $@

$(EXPORT_DIR)/Makefile.config: ../../../Makefile.config
	cp $< $@

export-libs: $(EXPORT_FILES) export-includes
	mkdir -p $(EXPORT_DIR)/lib
	cp -L $(EXPORT_FILES) $(EXPORT_DIR)/lib

export-includes: $(INCLUDE_LIBC) $(shell find ./include ../c/arch/$(UARCH)/include $(ROOT_PATH)/abi/include -name '*.h')
	mkdir -p $(EXPORT_DIR)/include
	rm -rf $(EXPORT_DIR)/include.new
	cp -r -L -T ./include $(EXPORT_DIR)/include.new
	cp -r -L ../c/arch/$(UARCH)/include/libarch $(EXPORT_DIR)/include.new/libc
	cp -r -L $(ROOT_PATH)/abi/include/* $(EXPORT_DIR)/include.new
	mkdir -p $(EXPORT_DIR)/include.new/libclui
	cp -L ../clui/tinput.h $(EXPORT_DIR)/include.new/libclui
	rm -rf $(EXPORT_DIR)/include
	mkdir -p $(EXPORT_DIR)/include.new/libdraw
	cp -r -L -t $(EXPORT_DIR)/include.new/libdraw ../draw/*.h
	mkdir -p $(EXPORT_DIR)/include.new/libdraw/codec
	cp -r -L -t $(EXPORT_DIR)/include.new/libdraw/codec ../draw/codec/*.h
	mkdir -p $(EXPORT_DIR)/include.new/libgui
	cp -L -t $(EXPORT_DIR)/include.new/libgui ../gui/*.h
	mkdir -p $(EXPORT_DIR)/include.new/libhound
	cp -r -L -t $(EXPORT_DIR)/include.new/libhound ../hound/include/*
	mkdir -p $(EXPORT_DIR)/include.new/libpcm
	cp -r -L -t $(EXPORT_DIR)/include.new/libpcm ../pcm/include/*
	mv $(EXPORT_DIR)/include.new $(EXPORT_DIR)/include
