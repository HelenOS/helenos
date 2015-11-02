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

USPACE_PREFIX = ../..
LIBS = $(LIBCONF_PREFIX)/libconf.a
EXTRA_CFLAGS += -I. \
	-I./units \
	-I$(LIBCONF_PREFIX)/include \
	-I$(LIBSYSMAN_PREFIX)/include
BINARY = sysman
STATIC_NEEDED = y

SYSMAN_SOURCES = \
	connection_broker.c \
	connection_ctl.c \
	edge.c \
	job.c \
	log.c \
	repo.c \
	sm_task.c \
	sysman.c \
	unit.c \
	units/unit_cfg.c \
	units/unit_mnt.c \
	units/unit_svc.c \
	units/unit_tgt.c \
	util.c

SOURCES = \
	main.c \
	$(SYSMAN_SOURCES)

TEST_SOURCES = \
	$(SYSMAN_SOURCES) \
	test/job_closure.c \
	test/job_queue.c \
	test/mock_unit.c \
	test/main.c

include $(USPACE_PREFIX)/Makefile.common
