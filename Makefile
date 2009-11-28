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

## Include configuration
#

CSCOPE = cscope
STANSE = stanse

.PHONY: all config config_default distclean clean cscope stanse

all: Makefile.config config.h config.defs
	$(MAKE) -C kernel
	$(MAKE) -C uspace
	$(MAKE) -C boot

stanse: Makefile.config config.h config.defs
	$(MAKE) -C kernel clean
	$(MAKE) -C kernel EXTRA_TOOL=stanse
	$(STANSE) --checker ReachabilityChecker --checker ThreadChecker:contrib/$(STANSE)/ThreadChecker.xml --jobfile kernel/kernel.job

cscope:
	find kernel boot uspace -regex '^.*\.[chsS]$$' | xargs $(CSCOPE) -b -k -u -f$(CSCOPE).out

Makefile.config: config_default

config.h: config_default

config.defs: config_default

config_default: HelenOS.config
	tools/config.py HelenOS.config default

config: HelenOS.config
	tools/config.py HelenOS.config

distclean: clean
	rm -f $(CSCOPE).out Makefile.config config.h config.defs tools/*.pyc

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C uspace clean
	$(MAKE) -C boot clean
