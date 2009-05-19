# Copyright (c) 2005,  Martin Decky
# All rights reserved.
# Copyright (c) 2008, Tim Post <tinkertim@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# Neither the name of the original program's authors nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

include ../../../version

LIBC_PREFIX = ../../lib/libc
SOFTINT_PREFIX = ../../lib/softint
LIBBLOCK_PREFIX = ../../lib/libblock

include $(LIBC_PREFIX)/Makefile.toolchain

CFLAGS += -I../../srv/kbd/include -I$(LIBBLOCK_PREFIX)

LIBS = $(LIBBLOCK_PREFIX)/libblock.a $(LIBC_PREFIX)/libc.a
DEFS += -DRELEASE=$(RELEASE)

PROGRAM = bdsh

# Any directory that cleaning targets should know about
SUBDIRS = \
	./ \
	cmds/ \
	cmds/modules/ \
	cmds/modules/help/ \
	cmds/modules/mkdir/ \
	cmds/modules/rm/ \
	cmds/modules/bdd/ \
	cmds/modules/cat/ \
	cmds/modules/touch/ \
	cmds/modules/ls/ \
	cmds/modules/pwd/ \
	cmds/modules/sleep/ \
	cmds/modules/cp/ \
	cmds/modules/mv/ \
	cmds/modules/mount/ \
	cmds/modules/kcon/ \
	cmds/builtins/ \
	cmds/builtins/exit/\
	cmds/builtins/cd/

SOURCES = \
	cmds/modules/help/help.c \
	cmds/modules/mkdir/mkdir.c \
	cmds/modules/rm/rm.c \
	cmds/modules/bdd/bdd.c \
	cmds/modules/cat/cat.c \
	cmds/modules/touch/touch.c \
	cmds/modules/ls/ls.c \
	cmds/modules/pwd/pwd.c \
	cmds/modules/sleep/sleep.c \
	cmds/modules/cp/cp.c \
	cmds/modules/mv/mv.c \
	cmds/modules/mount/mount.c \
	cmds/modules/kcon/kcon.c \
	cmds/builtins/exit/exit.c \
	cmds/builtins/cd/cd.c \
	cmds/mod_cmds.c \
	cmds/builtin_cmds.c \
	errors.c \
	input.c \
	util.c \
	exec.c \
	scli.c

CFLAGS += -I. -Icmds/ -Icmds/builtins -Icmds/modules

OBJECTS = $(SOURCES:.c=.o)

# For easy cleaning, *.o is already handled
CLEANDIRS := $(addsuffix *~,$(SUBDIRS))
CLEANDIRS += $(addsuffix *.bak,$(SUBDIRS))
CLEANDIRS += $(addsuffix *.tmp,$(SUBDIRS))
CLEANDIRS += $(addsuffix *.out,$(SUBDIRS))
CLEANDIRS += $(addsuffix *.d,$(SUBDIRS))
CLEANDIRS += $(addsuffix *.gch,$(SUBDIRS) )

%.o: %.S
	$(CC) $(DEFS) $(AFLAGS) $(CFLAGS) -D__ASM__ -c $< -o $@

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@
	@$(CC) -M $(CFLAGS) $(INC) $*.c > $*.d

$(PROGRAM): $(OBJECTS) $(LIBS)
	$(LD) -T $(LIBC_PREFIX)/arch/$(UARCH)/_link.ld $(OBJECTS) $(LIBS) $(LFLAGS) -o $@ -Map $(PROGRAM).map

# Everything else is a phony target
.PHONY: all clean distclean depend disasm

all: $(PROGRAM) disasm

clean:
	@-rm -f $(OBJECTS)
	@-rm -f $(PROGRAM)
	@-rm -f $(PROGRAM).map
	@-rm -f $(PROGRAM).disasm
	@-rm -f $(CLEANDIRS)

depend:
	@echo ''

disasm:
	$(OBJDUMP) -d $(PROGRAM) >$(PROGRAM).disasm

distclean: clean

# Do not delete - dependencies
-include $(OBJECTS:.o=.d)
