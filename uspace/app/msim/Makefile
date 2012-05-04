#
# Copyright (c) 2012 Vojtech Horky
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
BINARY = msim

MSIM_VERSION = 1.3.8.1
MSIM_DIST = msim-$(MSIM_VERSION)
MSIM_TARBALL = $(MSIM_DIST).tar.bz2
MSIM_MIRROR = http://d3s.mff.cuni.cz/~holub/sw/msim/

MSIM_HELENOS_MAKEFILE = Makefile.msim.src
MSIM_HELENOS_PLATFORM_FILES = misc.c input.c
MSIM_HELENOS_ARCH_DIR = arch/helenos
MSIM_PATCHED_FILES = \
	$(MSIM_DIST)/config.h \
	$(MSIM_DIST)/src/$(MSIM_HELENOS_MAKEFILE) \
	$(MSIM_DIST)/src/$(MSIM_HELENOS_ARCH_DIR)/helenos.h \
	$(MSIM_DIST)/src/$(MSIM_HELENOS_ARCH_DIR)/misc.c \
	$(MSIM_DIST)/src/$(MSIM_HELENOS_ARCH_DIR)/input.c

MSIM_EXTRA_SOURCES := $(addprefix $(MSIM_HELENOS_ARCH_DIR)/,$(MSIM_HELENOS_PLATFORM_FILES))

CP = /bin/cp -f

all: $(BINARY)

.PHONY: all clean unpack-tarball

clean:
	rm -rf msim Makefile.depend* *.map *.disasm $(MSIM_TARBALL) $(MSIM_DIST)

$(BINARY): $(MSIM_DIST)/src/msim
	$(CP) $< $@

# Run make in MSIM src directory
$(MSIM_DIST)/src/msim: $(MSIM_PATCHED_FILES) $(MSIM_DIST)/src/Makefile.sources Makefile 
	cd $(MSIM_DIST)/src; make -f $(MSIM_HELENOS_MAKEFILE) USPACE_PREFIX=../../$(USPACE_PREFIX)
	
# Copy HelenOS specific files
$(MSIM_DIST)/config.h: config.h.msim | unpack-tarball
	$(CP) $< $@

$(MSIM_DIST)/src/$(MSIM_HELENOS_ARCH_DIR)/helenos.h: arch_helenos/helenos.h | unpack-tarball
	$(CP) $< $@

$(MSIM_DIST)/src/$(MSIM_HELENOS_ARCH_DIR)/misc.c: arch_helenos/misc.c | unpack-tarball
	$(CP) $< $@

$(MSIM_DIST)/src/$(MSIM_HELENOS_ARCH_DIR)/input.c: arch_helenos/input.c | unpack-tarball
	$(CP) $< $@

$(MSIM_DIST)/src/$(MSIM_HELENOS_MAKEFILE): $(MSIM_HELENOS_MAKEFILE) | unpack-tarball
	$(CP) $< $@

# Extract list of sources, remove POSIX/Linux specific
$(MSIM_DIST)/src/Makefile.sources: | unpack-tarball
	cd $(MSIM_DIST)/src; ( \
		echo 'all__:'; \
		echo -e '\t@echo SOURCES = $$(SOURCES)'; cat Makefile.in \
	) | make -f - | grep 'SOURCES' >Makefile.sources
	sed \
		-e 's:io/input.c:$(MSIM_EXTRA_SOURCES):' \
		-e 's:debug/gdb.c::' \
		-e 's:arch/posix/stdin.c::' \
		-i $(MSIM_DIST)/src/Makefile.sources

# Download tarball
$(MSIM_TARBALL):
	wget "$(MSIM_MIRROR)$(MSIM_TARBALL)" -O $(MSIM_TARBALL)

unpack-tarball: $(MSIM_TARBALL)
	tar xjf $(MSIM_TARBALL)
	mkdir -p $(MSIM_DIST)/src/$(MSIM_HELENOS_ARCH_DIR)
