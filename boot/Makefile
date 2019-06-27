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

.PHONY: all clean

include Makefile.common

all: $(VERSION_DEF) $(COMMON_MAKEFILE) $(CONFIG_MAKEFILE) $(CONFIG_HEADER) $(PREBUILD)
	$(MAKE) -r -f $(BUILD) PRECHECK=$(PRECHECK)
ifneq ($(POSTBUILD),)
	$(MAKE) -r -f $(POSTBUILD) PRECHECK=$(PRECHECK)
endif

clean:
	$(MAKE) -r -f $(BUILD) clean PRECHECK=$(PRECHECK)
ifneq ($(POSTBUILD),)
	$(MAKE) -r -f $(POSTBUILD) clean PRECHECK=$(PRECHECK)
endif
	rm -f $(POST_OUTPUT) $(BOOT_OUTPUT) arch/*/include/common.h
	find generic/src/ arch/*/src/ genarch/src/ -name '*.o' -follow -exec rm \{\} \;
	find . -name '*.d' -follow -exec rm \{\} \;
