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

CSCOPE = cscope
CONFIG = tools/config.py
AUTOTOOL = tools/autotool.py
SANDBOX = autotool

CONFIG_RULES = HelenOS.config

COMMON_MAKEFILE = Makefile.common
COMMON_HEADER = common.h
COMMON_HEADER_PREV = $(COMMON_HEADER).prev

CONFIG_MAKEFILE = Makefile.config
CONFIG_HEADER = config.h

.PHONY: all precheck cscope autotool config_default config distclean clean

all: $(COMMON_MAKEFILE) $(COMMON_HEADER) $(CONFIG_MAKEFILE) $(CONFIG_HEADER)
	cp -a $(COMMON_HEADER) $(COMMON_HEADER_PREV)
	$(MAKE) -C kernel PRECHECK=$(PRECHECK)
	$(MAKE) -C uspace PRECHECK=$(PRECHECK)
	$(MAKE) -C boot PRECHECK=$(PRECHECK)

precheck: clean
	$(MAKE) all PRECHECK=y

cscope:
	find kernel boot uspace -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE).out

$(COMMON_MAKEFILE): autotool
$(COMMON_HEADER): autotool

autotool: $(CONFIG_MAKEFILE)
	$(AUTOTOOL)
	-[ -f $(COMMON_HEADER_PREV) ] && diff -q $(COMMON_HEADER_PREV) $(COMMON_HEADER) && mv -f $(COMMON_HEADER_PREV) $(COMMON_HEADER)

$(CONFIG_MAKEFILE): config_default
$(CONFIG_HEADER): config_default

config_default: $(CONFIG_RULES)
	$(CONFIG) $< default

config: $(CONFIG_RULES)
	$(CONFIG) $<

distclean: clean
	rm -f $(CSCOPE).out $(COMMON_MAKEFILE) $(COMMON_HEADER) $(COMMON_HEADER_PREV) $(CONFIG_MAKEFILE) $(CONFIG_HEADER) tools/*.pyc tools/checkers/*.pyc

clean:
	rm -fr $(SANDBOX)
	$(MAKE) -C kernel clean
	$(MAKE) -C uspace clean
	$(MAKE) -C boot clean
