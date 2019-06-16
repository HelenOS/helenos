#
# Copyright (c) 2006 Martin Decky
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

# Just for this Makefile. Sub-makes will run in parallel if requested.
.NOTPARALLEL:

CCHECK = tools/sycek/ccheck
CSCOPE = cscope
FORMAT = clang-format
CHECK = tools/check.sh
CONFIG = tools/config.py
AUTOTOOL = tools/autotool.py
SANDBOX = autotool
MESON = meson

CONFIG_RULES = HelenOS.config

COMMON_MAKEFILE = Makefile.common

CONFIG_MAKEFILE = Makefile.config
CONFIG_HEADER = config.h
ERRNO_HEADER = abi/include/abi/errno.h
ERRNO_INPUT = abi/include/abi/errno.in

-include $(CONFIG_MAKEFILE)
-include $(COMMON_MAKEFILE)

# TODO: make meson reconfigure correctly when library build changes

ifeq ($(CONFIG_BUILD_SHARED_LIBS),y)
	MESON_ARGS = -Ddefault_library=shared
else
	MESON_ARGS = -Ddefault_library=static
endif

CROSS_PATH = $(shell dirname "$(CC)")

.PHONY: all precheck cscope cscope_parts autotool config_default config distclean clean check releasefile release common export-posix space

all: common export-cross test-xcw
	$(MAKE) -r -C boot PRECHECK=$(PRECHECK)

build/build.ninja: Makefile.config version
	PATH="$(CROSS_PATH):$$PATH" meson . build --cross-file meson/cross/$(UARCH) $(MESON_ARGS)

common: $(COMMON_MAKEFILE) $(CONFIG_MAKEFILE) $(CONFIG_HEADER) $(ERRNO_HEADER) build/build.ninja
	PATH="$(CROSS_PATH):$$PATH" ninja -C build
	PATH="$(CROSS_PATH):$$PATH" DESTDIR="$$PWD/dist" meson install --no-rebuild --only-changed -C build > build/install.log

test-xcw: common export-cross
ifeq ($(CONFIG_DEVEL_FILES),y)
	export PATH=$$PATH:$(abspath tools/xcw/bin) && $(MAKE) -r -C tools/xcw/demo
endif

export-posix: common
ifndef EXPORT_DIR
	@echo ERROR: Variable EXPORT_DIR is not defined. && false
else
	mkdir -p $(EXPORT_DIR)
	$(MAKE) -r -C uspace/lib/posix export EXPORT_DIR=$(abspath $(EXPORT_DIR)) UARCH=$(UARCH)
endif

export-cross: common
	mkdir -p uspace/export
	$(MAKE) -r -C uspace export EXPORT_DIR=$(abspath uspace/export) UARCH=$(UARCH)

precheck: clean
	$(MAKE) -r all PRECHECK=y

cscope:
	find abi kernel boot uspace -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE).out

cscope_parts:
	find abi -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE)_abi.out
	find kernel -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE)_kernel.out
	find boot -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE)_boot.out
	find uspace -type f -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE)_uspace.out

format:
	find abi kernel boot uspace -type f -regex '^.*\.[ch]$$' | xargs $(FORMAT) -i -sort-includes -style=file

ccheck: $(CCHECK)
	cd tools && ./build-ccheck.sh
	tools/ccheck.sh

ccheck-fix: $(CCHECK)
	cd tools && ./build-ccheck.sh
	tools/ccheck.sh --fix

$(CCHECK):
	cd tools && ./build-ccheck.sh

space:
	tools/srepl '[ \t]\+$$' ''

doxy:
	$(MAKE) -r -C doxygen

# Pre-integration build check
check: ccheck $(CHECK)
ifdef JOBS
	$(CHECK) -j $(JOBS)
else
	$(CHECK) -j $(shell nproc)
endif

# `sed` pulls a list of "compatibility-only" error codes from `errno.in`,
# the following grep finds instances of those error codes in HelenOS code.
check_errno:
	@ ! cat abi/include/abi/errno.in | \
	sed -n -e '1,/COMPAT_START/d' -e 's/__errno_entry(\([A-Z0-9]\+\).*/\\b\1\\b/p' | \
	git grep -n -f - -- ':(exclude)abi' ':(exclude)uspace/lib/posix'

# Autotool (detects compiler features)

autotool $(COMMON_MAKEFILE): $(CONFIG_MAKEFILE) $(AUTOTOOL)
	$(AUTOTOOL)

# Build-time configuration

config_default $(CONFIG_MAKEFILE) $(CONFIG_HEADER): $(CONFIG_RULES)
ifeq ($(HANDS_OFF),y)
	$(CONFIG) $< hands-off $(PROFILE)
else
	$(CONFIG) $< default $(PROFILE)
endif

config: $(CONFIG_RULES)
	$(CONFIG) $<

random-config: $(CONFIG_RULES)
	$(CONFIG) $< random

# Release files

releasefile: all
	$(MAKE) -r -C release releasefile

release:
	$(MAKE) -r -C release release

# Cleaning

distclean: clean
	rm -f $(CSCOPE).out $(COMMON_MAKEFILE) $(CONFIG_MAKEFILE) $(CONFIG_HEADER) tools/*.pyc tools/checkers/*.pyc release/HelenOS-*

clean:
	rm -fr $(SANDBOX)
	$(MAKE) -r -C kernel clean
	$(MAKE) -r -C uspace clean
	$(MAKE) -r -C boot clean
	$(MAKE) -r -C doxygen clean
	$(MAKE) -r -C tools/xcw/demo clean

$(ERRNO_HEADER): $(ERRNO_INPUT)
	echo '/* Generated file. Edit errno.in instead. */' > $@.new
	sed 's/__errno_entry(\([^,]*\),\([^,]*\),.*/#define \1 __errno_t(\2)/' < $< >> $@.new
	mv $@.new $@

-include Makefile.local
