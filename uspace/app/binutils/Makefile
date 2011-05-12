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

# Symlink to the sources of the ported application.
APP_DIR = ./src

# Default directory for the binutils source tree.
REDIST_DIR = ./redist

# Absolute path to the binutils source tree.
# Can be set externally, defaults to $REDIST_DIR.
BINUTILS_ABS_PATH ?= $(subst $(space),\ ,$(PWD))/$(REDIST_DIR)

# $USPACE_PREFIX have to be based on the absolute path,
# because targets derived from it will be referenced from
# other than the current directory.
USPACE_PREFIX = $(subst $(space),\ ,$(PWD))/../..

# Ensure static configuration of Makefile.common.
STATIC_ONLY = y

# Makefile.common for native applications.
COMMON_MAKEFILE_NATIVE = $(USPACE_PREFIX)/Makefile.common

# Generated from native Makefile.common.
COMMON_MAKEFILE_PATCHED = ./Makefile.common

# AWK script which generates patched Makefile.common.
PATCH_SCRIPT = ./patch.awk

# Compilers that can be used to build binutils.
SUPPORTED_COMPILERS = gcc_cross gcc_native

# Patched Makefile.common for ported user space applications.
include $(COMMON_MAKEFILE_PATCHED)

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

# Patch native Makefile.common.
# Check presence of gcc compiler.
# Patch $PATH to intercept toolchain calls.
# Generate false toolchain.
# Create symlink to the binutils directory.
# Patch binutils.
# Configure binutils for target architecture.
# Make binutils.
# Copy binaries.
all: $(COMMON_MAKEFILE_PATCHED) all_
ifeq ($(COMPILER),$(findstring $(COMPILER),$(SUPPORTED_COMPILERS)))
	OLDPATH = $(PATH)
	PATH = $(PWD):$(PATH)
	export PATH
	./toolchain.sh gcc $(CC) '$(CFLAGS)'
	./toolchain.sh as $(AS)
	./toolchain.sh ar $(AR)
	./toolchain.sh ranlib
	./toolchain.sh ld $(LD) '$(LFLAGS)' '$(LINKER_SCRIPT)' '$(LIBS) $(BASE_LIBS)'
	./toolchain.sh objdump $(OBJDUMP)
	./toolchain.sh objcopy $(OBJCOPY)
	./toolchain.sh strip $(STRIP)
	ln -s -T $(BINUTILS_ABS_PATH) $(APP_DIR)
	./intrusive.sh do $(APP_DIR)
	ORIG_DIR = $(subst $(space),\ ,$(PWD))
	cd $(APP_DIR)
	./configure --target=$(TARGET) $(CONF_FLAGS)
	make $(MAKE_TARGETS)
	cd $(ORIG_DIR)
	cp $(APP_DIR)/gas/as-new ./as
	cp $(APP_DIR)/ld/ld-new ./ld
	PATH = $(OLDPATH)
	export PATH
else
	echo 'Skipped: Cannot build binutils with unsupported compiler.'
endif

# Create patched Makefile.common from native Makefile.common.
$(COMMON_MAKEFILE_PATCHED): $(PATCH_SCRIPT) $(COMMON_MAKEFILE_NATIVE)
	awk -f $^ > $@

# Delete binaries.
# Clean binutils.
# Unpatch binutils.
# Delete symlink
# Delete generated scripts.
clean:
	rm -f as ld
	ORIG_DIR = $(subst $(space),\ ,$(PWD))
	cd $(APP_DIR)
	make distclean
	cd $(ORIG_DIR)
	./intrusive.sh undo $(APP_DIR)
	rm -f $(APP_DIR)
	rm -f gcc as ar ranlib ld objdump objcopy strip
	rm -f $(COMMON_MAKEFILE_PATCHED)

