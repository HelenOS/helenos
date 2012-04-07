#
# Copyright (c) 2011 Petr Koupy
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

nullstring =
space = $(nullstring) # space

# When redefined to 'y' on the command line, certain areas of this Makefile
# behave slightly differently to simplify the maintenance of binutils port.
MAINTAIN_BINUTILS = n

# Information for obtaining specific binutils redistributable package.
# Might be subject to change in the future.
REDIST_VERSION = 2.21.1
REDIST_NAME = binutils-$(REDIST_VERSION)
REDIST_FILENAME = $(REDIST_NAME).tar.bz2
REDIST_SOURCE = ftp://ftp.gnu.org/gnu/binutils/
REDIST_MIRROR = http://www.helenos.org/binutils/

# Directory for the binutils source tree.
REDIST_DIR = ./redist

# File to detect the presence of binutils source tree.
REDIST_DETECT = $(REDIST_DIR)/configure

# Directory for resulting binutils binaries.
BIN_DIR = ./bin

# $USPACE_PREFIX have to be based on the absolute path,
# because targets derived from it will be referenced from
# other than the current directory.
USPACE_PREFIX = $(subst $(space),\ ,$(shell pwd))/../..

# Ensure static configuration of Makefile.common.
STATIC_ONLY = y

# Link with POSIX runtime library.
POSIX_COMPAT = y

# Makefile.common for native applications.
COMMON_MAKEFILE_NATIVE = $(USPACE_PREFIX)/Makefile.common

# Generated from native Makefile.common.
COMMON_MAKEFILE_PATCHED = ./Makefile.common

# AWK script which generates patched Makefile.common.
MAKEFILE_PATCH = ./patch.awk

# Compilers that can be used to build binutils.
SUPPORTED_COMPILERS = gcc_cross gcc_native

# Patched Makefile.common for ported user space applications.
-include $(COMMON_MAKEFILE_PATCHED)

# Patch $PATH to intercept toolchain calls.
PATH := $(shell pwd):$(PATH)
export PATH

# Shell script for false toolchain generation.
TOOLCHAIN_SCRIPT = ./toolchain.sh

# False toolchain.
TOOLCHAIN = ./gcc ./as ./ar ./ranlib ./ld ./objdump ./objcopy ./strip

# Stamp indicating whether the binutils source tree is patched.
BINUTILS_PATCHED = ./done

# Shell script for instrusive patches of binutils source tree.
BINUTILS_PATCH = ./intrusive.sh

# Detection whether the binutils are already configured.
BINUTILS_CONFIGURED = $(REDIST_DIR)/Makefile

# Generated source file for libposix function stubs without posix_ prefix.
CONFOBJ_SOURCE = ./confobj.c

# Dummy object file for libposix function stubs without posix_ prefix..
# Required by binutils configure script tests that links against symbols
# without including headers (which means that symbols are not prefixed).
CONFOBJ_OBJECT = $(subst $(space),\ ,$(shell pwd))/confobj.o

# Required by libposix on RISC platforms.
BASE_LIBS += $(LIBSOFTFLOAT_PREFIX)/libsoftfloat.a

# Map the HelenOS target to binutils target.
ifeq ($(PLATFORM),amd64)
TARGET = amd64-linux-gnu
endif
ifeq ($(PLATFORM),arm32)
TARGET = arm-linux-gnueabi
endif
ifeq ($(PLATFORM),ia32)
TARGET = i686-pc-linux-gnu
endif
ifeq ($(PLATFORM),ia64)
TARGET = ia64-pc-linux-gnu
endif
ifeq ($(PLATFORM),mips32)
TARGET = mipsel-linux-gnu
endif
ifeq ($(PLATFORM),mips32eb)
TARGET = mips-linux-gnu
endif
ifeq ($(PLATFORM),mips64)
TARGET = mips64el-linux-gnu
endif
ifeq ($(PLATFORM),ppc32)
TARGET = ppc-linux-gnu
endif
ifeq ($(PLATFORM),ppc64)
TARGET = ppc64-linux-gnu
endif
ifeq ($(PLATFORM),sparc64)
TARGET = sparc64-linux-gnu
endif

# Binutils configure flags.
CONF_FLAGS = --disable-nls --disable-shared --enable-static \
	--with-zlib=no --with-ppl=no --with-cloog=no  \
	--with-gmp=no --with-mpfr=no --with-mpc=no

# Binutils make targets.
MAKE_TARGETS = all-gas all-ld

# Check presence of gcc compiler.
# Make binutils.
# Copy binaries.
ifeq ($(COMPILER),$(findstring $(COMPILER),$(SUPPORTED_COMPILERS)))
all: $(COMMON_MAKEFILE_PATCHED) all_ $(TOOLCHAIN) $(BINUTILS_PATCHED) \
		$(CONFOBJ_OBJECT) $(BINUTILS_CONFIGURED) $(REDIST_DETECT)
	$(MAKE) -C $(REDIST_DIR) $(MAKE_TARGETS)
	cp -f $(REDIST_DIR)/gas/as-new $(BIN_DIR)/as
	cp -f $(REDIST_DIR)/ld/ld-new $(BIN_DIR)/ld
else
all:
	# Skipped: Cannot build binutils with unsupported compiler.
endif

# Create patched Makefile.common from native Makefile.common.
$(COMMON_MAKEFILE_PATCHED): $(MAKEFILE_PATCH) $(COMMON_MAKEFILE_NATIVE)
	awk -f $^ > $@

# Download binutils redistributable package.
$(REDIST_FILENAME):
	wget -c $(REDIST_SOURCE)$(REDIST_FILENAME)
	if [ ! -e $(REDIST_FILENAME) ]; then \
		wget -c $(REDIST_MIRROR)$(REDIST_FILENAME); \
	fi

# Extract binutils source tree.
$(REDIST_DETECT): $(REDIST_FILENAME)
	tar -x -j -f $<
	mv -f -T $(REDIST_NAME) $(REDIST_DIR)
	touch $@

# Generate stubs for libposix functions without posix_ prefix.
$(CONFOBJ_SOURCE):
	echo '/* GENERATED FILE. DO NOT MODIFY. */' > $@; \
	$(OBJDUMP) -t $(LIBPOSIX_PREFIX)/libposix.a | \
	grep 'F .text' | grep -o -h -I -E 'posix_[^.]*$$' | sort -u | \
	sed 's/posix_\([^ ]*\)/char \1() { return 0; }/g' >> $@

# Compile dummy object for configure script tests.
$(CONFOBJ_OBJECT): $(CONFOBJ_SOURCE) $(TOOLCHAIN)
	./gcc -c -o $@ $<

# Generate false toolchain scripts.
$(TOOLCHAIN): $(TOOLCHAIN_SCRIPT) $(COMMON_MAKEFILE_PATCHED)
	./$< gcc $(CC) \
		'$(CFLAGS)' '$(LINKER_SCRIPT)' '$(LIBS) $(BASE_LIBS) $(CONFOBJ_OBJECT)'
	./$< as $(AS)
	./$< ar $(AR)
	./$< ranlib
	./$< ld $(LD) \
		'$(LFLAGS)' '$(LINKER_SCRIPT)' '$(LIBS) $(BASE_LIBS)'
	./$< objdump $(OBJDUMP)
	./$< objcopy $(OBJCOPY)
	./$< strip $(STRIP)

# Patch binutils source tree.
$(BINUTILS_PATCHED): $(BINUTILS_PATCH) $(REDIST_DETECT)
	./$< do $(REDIST_DIR)
	touch $@

# Configure binutils.
# $LD variable have to exported to override configure script caching.
$(BINUTILS_CONFIGURED): $(REDIST_DETECT) $(BINUTILS_PATCHED) \
		$(TOOLCHAIN) $(CONFOBJ_OBJECT)
	export LD=ld; \
	cd $(REDIST_DIR); \
	./configure --target=$(TARGET) $(CONF_FLAGS)

# Delete binaries.
# Clean binutils.
# Unpatch binutils.
# Delete everything in redist folder.
# Delete generated scripts.
clean: $(BINUTILS_PATCH) clean_
	rm -f $(BIN_DIR)/as $(BIN_DIR)/ld
	rm -f $(CONFOBJ_SOURCE) $(CONFOBJ_OBJECT)
ifeq ($(MAINTAIN_BINUTILS),y)
	if [ -e $(REDIST_DIR)/Makefile ]; then \
		make -C $(REDIST_DIR) distclean; \
	fi
	if [ -e $(BINUTILS_PATCHED) ]; then \
		./intrusive.sh undo $(REDIST_DIR); \
		rm -f $(BINUTILS_PATCHED); \
	fi
else
	rm -r -f redist/*
	rm -f $(BINUTILS_PATCHED)
endif
	rm -f $(TOOLCHAIN)
	rm -f $(COMMON_MAKEFILE_PATCHED)

