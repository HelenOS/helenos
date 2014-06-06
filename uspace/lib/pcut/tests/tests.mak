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

TEST_DEPS = $(TEST_BASE)tested.o

TEST_APPS = \
	$(TEST_BASE)alloc.$(EXE_EXT) \
	$(TEST_BASE)asserts.$(EXE_EXT) \
	$(TEST_BASE)errno.$(EXE_EXT) \
	$(TEST_BASE)manytests.$(EXE_EXT) \
	$(TEST_BASE)multisuite.$(EXE_EXT) \
	$(TEST_BASE)null.$(EXE_EXT) \
	$(TEST_BASE)nullteardown.$(EXE_EXT) \
	$(TEST_BASE)printing.$(EXE_EXT) \
	$(TEST_BASE)simple.$(EXE_EXT) \
	$(TEST_BASE)skip.$(EXE_EXT) \
	$(TEST_BASE)suites.$(EXE_EXT) \
	$(TEST_BASE)teardown.$(EXE_EXT) \
	$(TEST_BASE)timeout.$(EXE_EXT)

check-build: $(TEST_APPS)

.PRECIOUS: $(TEST_BASE)tested.o

check-clean:
	rm -f $(TEST_BASE)*.o $(TEST_BASE)*.pcut.c $(TEST_BASE)*.$(EXE_EXT) $(TEST_BASE)*.got

$(TEST_BASE)%.$(EXE_EXT): $(TEST_DEPS)
	$(LD) -o $@ $^ $(TEST_LDFLAGS)

$(TEST_BASE)alloc.$(EXE_EXT): $(TEST_BASE)alloc.o $(PCUT_LIB)
$(TEST_BASE)asserts.$(EXE_EXT): $(TEST_BASE)asserts.o $(PCUT_LIB)
$(TEST_BASE)errno.$(EXE_EXT): $(TEST_BASE)errno.o $(PCUT_LIB)
$(TEST_BASE)manytests.$(EXE_EXT): $(TEST_BASE)manytests.o $(PCUT_LIB)
$(TEST_BASE)multisuite.$(EXE_EXT): $(TEST_BASE)suite_all.o $(TEST_BASE)suite1.o $(TEST_BASE)suite2.o $(PCUT_LIB)
$(TEST_BASE)null.$(EXE_EXT): $(TEST_BASE)null.o $(PCUT_LIB)
$(TEST_BASE)nullteardown.$(EXE_EXT): $(TEST_BASE)nullteardown.o $(PCUT_LIB)
$(TEST_BASE)printing.$(EXE_EXT): $(TEST_BASE)printing.o $(PCUT_LIB)
$(TEST_BASE)simple.$(EXE_EXT): $(TEST_BASE)simple.o $(PCUT_LIB)
$(TEST_BASE)skip.$(EXE_EXT): $(TEST_BASE)skip.o $(PCUT_LIB)
$(TEST_BASE)suites.$(EXE_EXT): $(TEST_BASE)suites.o $(PCUT_LIB)
$(TEST_BASE)teardown.$(EXE_EXT): $(TEST_BASE)teardown.o $(PCUT_LIB)
$(TEST_BASE)timeout.$(EXE_EXT): $(TEST_BASE)timeout.o $(PCUT_LIB)


ifeq ($(NEEDS_PREPROC),y)
$(TEST_BASE)%.o: $(TEST_BASE)%.pcut.c
	$(CC) -c -o $@ $(TEST_CFLAGS) $<

$(TEST_BASE)%.pcut.c: $(TEST_BASE)%.c $(PCUT_PREPROC)
	$(CC) -E $(TEST_CFLAGS) $< | $(PCUT_PREPROC) >$@
else
$(TEST_BASE)%.o: $(TEST_BASE)%.c
	$(CC) -c -o $@ $(TEST_CFLAGS) $<
endif
