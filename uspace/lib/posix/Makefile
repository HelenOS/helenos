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

EXTRA_CFLAGS = -Iinclude/

INCLUDE_LIBC = ./include/libc

LIBC_FILE = $(LIBC_PREFIX)/libc.a

FIXED_C_LIBRARY = libc4posix.a
MERGED_C_LIBRARY = libc.a
MERGE_LIBRARIES = \
	$(FIXED_C_LIBRARY) \
	$(LIBRARY).a \
	$(LIBSOFTFLOAT_PREFIX)/libsoftfloat.a \
	$(LIBSOFTINT_PREFIX)/libsoftint.a

SPECS = gcc.specs
LIBC_LINKER_SCRIPT = $(LIBC_PREFIX)/arch/$(UARCH)/_link.ld
LIBC_STARTUP_FILE = $(shell sed -n -e 's/^.*STARTUP(\(.*\)).*$$/\1/p' $(LIBC_LINKER_SCRIPT))
EXPORT_LINKER_SCRIPT = link.ld
EXPORT_STARTUP_FILE = crt0.o

EXPORT_FILES = \
	../math/libmath.a \
	../clui/libclui.a \
	$(MERGED_C_LIBRARY) \
	$(EXPORT_STARTUP_FILE) \
	$(EXPORT_LINKER_SCRIPT) \
	$(SPECS)

REDEFS_HIDE_LIBC = redefs-hide-libc-symbols.list

PRE_DEPEND = $(INCLUDE_LIBC)
EXTRA_CLEAN = \
	$(INCLUDE_LIBC) \
	$(REDEFS_HIDE_LIBC) \
	libc.o

EXTRA_OUTPUT = $(FIXED_C_LIBRARY) $(FIXED_POSIX_LIBRARY) $(MERGED_C_LIBRARY) $(SPECS) $(EXPORT_LINKER_SCRIPT) $(EXPORT_STARTUP_FILE)

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
	src/stdio/scanf.c \
	src/stdlib.c \
	src/stdlib/strtold.c \
	src/string.c \
	src/strings.c \
	src/sys/mman.c \
	src/sys/stat.c \
	src/sys/wait.c \
	src/time.c \
	src/unistd.c

TEST_SOURCES = \
	test/main.c \
	test/scanf.c

EXPORT_CPPFLAGS = \
	-specs $$(HELENOS_EXPORT_ROOT)/lib/gcc.specs \
	-isystem $$(HELENOS_EXPORT_ROOT)/include

EXPORT_LDFLAGS = \
	-L$$(HELENOS_EXPORT_ROOT)/lib \
	-T link.ld

EXPORT_LDLIBS = \
	-lmath -lc

EXPORT_CFLAGS = \
	-specs $$(HELENOS_EXPORT_ROOT)/lib/gcc.specs

include $(USPACE_PREFIX)/Makefile.common

$(SPECS): $(CONFIG_MAKEFILE)
	echo '*self_spec:' > $@.new
	echo '+ $(COMMON_CFLAGS)' >> $@.new
	echo >> $@.new
	echo '*cpp:' >> $@.new
	echo '+ $(COMMON_CPPFLAGS)' >> $@.new
	echo >> $@.new
	echo '*lib:' >> $@.new
	echo '$(EXPORT_LDLIBS)' >> $@.new
	mv $@.new $@

$(EXPORT_LINKER_SCRIPT): $(LIBC_LINKER_SCRIPT)
	sed 's/STARTUP(.*)/STARTUP(crt0.o)/' $< > $@

$(EXPORT_STARTUP_FILE): $(LIBC_STARTUP_FILE)
	cp $< $@

$(INCLUDE_LIBC): $(shell find ../c/include -name '*.h')
	cp -r -L --remove-destination -T ../c/include $@
	find ../c/include -type f -and -not -name '*.h' -delete

export: $(EXPORT_DIR)/config.mk $(EXPORT_DIR)/config.rc

$(EXPORT_DIR)/config.mk: export-libs export-includes
	echo '# Generated file, do not modify.' >> $@.new
	echo '# Do not forget to set HELENOS_EXPORT_ROOT.' >> $@.new
	echo 'HELENOS_CROSS_PATH="$(shell dirname $(CC))"' >> $@.new
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

export-libs: $(EXPORT_FILES) export-includes
	mkdir -p $(EXPORT_DIR)/lib
	cp -L $(EXPORT_FILES) $(EXPORT_DIR)/lib

export-includes: $(INCLUDE_LIBC) $(shell find ./include ../c/arch/$(UARCH)/include $(ROOT_PATH)/abi/include -name '*.h')
	mkdir -p $(EXPORT_DIR)/include
	rm -rf $(EXPORT_DIR)/include.new
	cp -r -L -T ./include/posix $(EXPORT_DIR)/include.new
	cp -r -L -T ./include/libc $(EXPORT_DIR)/include.new/libc
	cp -r -L ../c/arch/$(UARCH)/include/* $(EXPORT_DIR)/include.new/libc
	cp -r -L $(ROOT_PATH)/abi/include/* $(EXPORT_DIR)/include.new
	mkdir -p $(EXPORT_DIR)/include.new/libclui
	cp -L ../clui/tinput.h $(EXPORT_DIR)/include.new/libclui
	
	find "$(EXPORT_DIR)/include.new/libc" "$(EXPORT_DIR)/include.new/libclui" -name '*.h' -exec sed \
		-e 's:#include <:#include <libc/:' \
		-e 's:#include <libc/abi/:#include <abi/:' \
		-e 's:#include <libc/_bits/:#include <_bits/:' \
		-e 's:#include <libc/libc/:#include <libc/:' \
		-i {} \;
	find "$(EXPORT_DIR)/include.new" -name '*.h' -exec sed \
		-e 's:#include "posix/:#include ":' \
		-e 's:#include <posix/:#include <:' \
		-i {} \;
	
	rm -rf $(EXPORT_DIR)/include
	mv $(EXPORT_DIR)/include.new $(EXPORT_DIR)/include

$(FIXED_C_LIBRARY): $(LIBC_FILE) $(REDEFS_HIDE_LIBC)
	$(OBJCOPY) --redefine-syms=$(REDEFS_HIDE_LIBC) $(LIBC_FILE) $@

$(MERGED_C_LIBRARY): $(MERGE_LIBRARIES)
	$(LD) --fatal-warnings -r -o libc.o --whole-archive $(MERGE_LIBRARIES) --no-whole-archive
	$(AR) rc $@ libc.o

$(REDEFS_HIDE_LIBC): $(SOURCES)
	sed -n -e 's/_HIDE_LIBC_SYMBOL(\(.*\))/\1 __helenos_libc_\1/p' $(SOURCES) >$@
