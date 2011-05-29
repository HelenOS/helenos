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

# Information for obtaining specific binutils redistributable package.
# Might be subject to change in the future.
REDIST_VERSION = 2.21
REDIST_NAME = binutils-$(REDIST_VERSION)
REDIST_FILENAME = $(REDIST_NAME).tar.bz2
REDIST_SOURCE = ftp://ftp.gnu.org/gnu/binutils/

# Directory for the binutils source tree.
REDIST_DIR = ./redist

# File to detect the presence of binutils source tree.
REDIST_DETECT = $(REDIST_DIR)/configure

# $USPACE_PREFIX have to be based on the absolute path,
# because targets derived from it will be referenced from
# other than the current directory.
USPACE_PREFIX = $(subst $(space),\ ,$(PWD))/../..

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
include $(COMMON_MAKEFILE_PATCHED)

# Patch $PATH to intercept toolchain calls.
PATH := $(PWD):$(PATH)

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

# Map the HelenOS target to binutils target.
ifeq ($(PLATFORM),amd64)
TARGET=amd64-linux-gnu
else ($(PLATFORM),arm32)
TARGET=arm-linux-gnu
else ($(PLATFORM),ia32)
TARGET=i686-pc-linux-gnu
else ($(PLATFORM),ia64)
TARGET=ia64-pc-linux-gnu
else ($(PLATFORM),mips32)
TARGET=mipsel-linux-gnu
else ($(PLATFORM),mips32eb)
TARGET=mips-linux-gnu
else ($(PLATFORM),mips64)
TARGET=mips64el-linux-gnu
else ($(PLATFORM),ppc32)
TARGET=ppc-linux-gnu
else ($(PLATFORM),ppc64)
TARGET=ppc64-linux-gnu
else ($(PLATFORM),sparc64)
TARGET=sparc64-linux-gnu
endif

# Binutils configure flags.
CONF_FLAGS = --disable-nls --disable-shared --enable-static \
	--with-zlib=no

# Binutils make targets.
MAKE_TARGETS = all-gas all-ld

# Check presence of gcc compiler.
# Make binutils.
# Copy binaries.
ifeq ($(COMPILER),$(findstring $(COMPILER),$(SUPPORTED_COMPILERS)))
all: $(COMMON_MAKEFILE_PATCHED) all_ $(TOOLCHAIN) $(BINUTILS_PATCHED) \
		$(BINUTILS_CONFIGURED) $(REDIST_DETECT)
	export PATH
	make -C $(REDIST_DIR) $(MAKE_TARGETS)
	cp -f $(REDIST_DIR)/gas/as-new ./as
	cp -f $(REDIST_DIR)/ld/ld-new ./ld
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

# Extract binutils source tree.
$(REDIST_DETECT): $(REDIST_FILENAME)
	tar -x -j -f $<
	mv -f $(REDIST_NAME) $(REDIST_DIR)

# Generate false toolchain scripts.
$(TOOLCHAIN): $(TOOLCHAIN_SCRIPT)
	$^ gcc $(CC) '$(CFLAGS)'
	$^ as $(AS)
	$^ ar $(AR)
	$^ ranlib
	$^ ld $(LD) '$(LFLAGS)' '$(LINKER_SCRIPT)' '$(LIBS) $(BASE_LIBS)'
	$^ objdump $(OBJDUMP)
	$^ objcopy $(OBJCOPY)
	$^ strip $(STRIP)

# Patch binutils source tree.
$(BINUTILS_PATCHED): $(BINUTILS_PATCH) $(REDIST_DETECT)
	$^ do $(REDIST_DIR)
	touch $@

# Configure binutils.
$(BINUTILS_CONFIGURED): $(REDIST_DIR)/configure $(REDIST_DETECT)
	$^ --target=$(TARGET) $(CONF_FLAGS)

# Delete binaries.
# Clean binutils.
# Unpatch binutils.
# Delete generated scripts.
clean:
	rm -f as ld
	make -C $(REDIST_DIR) distclean
	./intrusive.sh undo $(REDIST_DIR)
	rm -f $(BINUTILS_PATCHED)
	rm -f $(TOOLCHAIN)
	rm -f $(COMMON_MAKEFILE_PATCHED)

