#
# Copyright (c) 2011 Jiri Svoboda
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

-include ../version
-include ../Makefile.config
-include ../boot/arch/$(BARCH)/Makefile.inc
-include ../boot/Makefile.common

PROFILES = amd64 arm32/GXemul arm32/integratorcp arm32/gta02 ia32 \
    ia64/i460GX ia64/ski mips32/GXemul mips32/msim sparc64/niagara \
    sparc64/serengeti sparc64/ultra

ifdef POST_OUTPUT
	IMGFILE = $(POST_OUTPUT)
else
	IMGFILE = $(BOOT_OUTPUT)
endif

SUFFIX = $(suffix $(IMGFILE))

ifdef PROFILE
	DISTFILE = Helenos-$(shell echo $(PROFILE) | tr '/' '-')$(SUFFIX)
else
	DISTFILE = HelenOS-$(RELEASE)-$(PLATFORM)-$(MACHINE)-$(PROCESSOR)$(SUFFIX)
endif

.PHONY: all clean dist distfile

all: distfile
distfile: $(DISTFILE)

$(DISTFILE): $(IMGFILE)
	cp $< $@

$(IMGFILE):
	$(MAKE) -C ..

dist:
	for profile in $(PROFILES); do \
		$(MAKE) -C .. clean ; \
		$(MAKE) -C .. PROFILE=$$profile distfile ; \
	done

clean:
	rm -f $(DISTFILE)
