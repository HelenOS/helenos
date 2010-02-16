#
# Copyright (c) 2005 Martin Decky
# Copyright (c) 2007 Jakub Jermar
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

include Makefile.common

.PHONY: all clean

all: ../../../../version ../../../../Makefile.config ../../../../config.h ../../../../config.defs
	-[ -f $(DEPEND) ] && mv -f $(DEPEND) $(DEPEND_PREV)
	$(MAKE) -f Makefile.build PRECHECK=$(PRECHECK)

clean:
	rm -f $(USPACEDIR)/dist/srv/*
	rm -f $(USPACEDIR)/dist/app/*
	rm -f $(USPACEDIR)/dist/cfg/net/*

	for file in $(RD_SRVS) $(RD_SRVS_GENERIC) ; do \
		rm -f $(USPACEDIR)/dist/srv/`basename $$file` ; \
	done
	for file in $(RD_APPS) $(RD_APPS_GENERIC) ; do \
		rm -f $(USPACEDIR)/dist/app/`basename $$file` ; \
	done
	for file in $(NET_CFG) ; do \
		rm -f $(USPACEDIR)/dist/cfg/net/`basename $$file` ; \
	done
	rm -f $(DEPEND) $(DEPEND_PREV) $(JOB) $(OUTPUT) $(COMPS).h $(COMPS).c $(LINK) $(INITRD).img $(INITRD).fs
	find . -name '*.o' -follow -exec rm \{\} \;
	find . -name '*.co' -follow -exec rm \{\} \;
