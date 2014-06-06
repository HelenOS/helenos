#
# Copyright (c) 2013 Vojtech Horky
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

PCUT_TARGET_SOURCES = src/os/stdc.c src/os/unix.c
OBJ_EXT = o
PCUT_LIB = libpcut.a
PCUT_PREPROC = ./pcut.bin

# Installation paths
PREFIX = /usr/local
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

-include pcut.mak

PCUT_OBJECTS := $(addsuffix .o,$(basename $(PCUT_SOURCES)))
PCUT_PREPROC_OBJECTS := $(addsuffix .o,$(basename $(PCUT_PREPROC_SOURCES)))

# Take care of dependencies
DEPEND = Makefile.depend
-include $(DEPEND)
$(DEPEND):
	makedepend -f - -Iinclude -- $(PCUT_SOURCES) >$@ 2>/dev/null


#
# Add the check target for running all the tests
#
TEST_BASE = tests/
EXE_EXT = run
TEST_CFLAGS = $(PCUT_CFLAGS)
TEST_LDFLAGS = 
-include tests/tests.mak
TEST_APPS_BASENAMES := $(basename $(TEST_APPS))
DIFF = diff
DIFFFLAGS = -du1

check: libpcut.a check-build
	@for i in $(TEST_APPS_BASENAMES); do \
		echo -n ./$$i; \
		./$$i.$(EXE_EXT) | sed 's:$(TEST_BASE)::g' >$$i.got; \
		if cmp -s $$i.got $$i.expected; then \
			echo " ok."; \
		else \
			echo " failed:"; \
			$(DIFF) $(DIFFFLAGS) $$i.expected $$i.got; \
		fi; \
	done

#
# Clean-up
#
platform-clean:
	rm -f $(DEPEND) $(PCUT_LIB) $(PCUT_PREPROC)

#
# Actual build rules
#
$(PCUT_LIB): $(PCUT_OBJECTS)
	$(AR) rc $@ $(PCUT_OBJECTS)
	$(RANLIB) $@

$(PCUT_PREPROC): $(PCUT_PREPROC_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(PCUT_PREPROC_OBJECTS)

%.o: $(DEPEND)

#
# Installation
#
install: $(PCUT_LIB)
	install -d -m 755 $(DESTDIR)$(LIBDIR)
	install -d -m 755 $(DESTDIR)$(INCLUDEDIR)/pcut
	install -m 644 $(PCUT_LIB) $(DESTDIR)$(LIBDIR)/$(PCUT_LIB)
	install -m 644 include/pcut/*.h $(DESTDIR)$(INCLUDEDIR)/pcut/
