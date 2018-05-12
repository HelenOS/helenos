#
# Copyright (c) 2017 Jaroslav Jindrak
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
ROOT_PATH = $(USPACE_PREFIX)/..

CONFIG_MAKEFILE = $(ROOT_PATH)/Makefile.config

LIBRARY = libcpp
SOVERSION = 0.0

-include $(CONFIG_MAKEFILE)

SOURCES = \
	src/condition_variable.cpp \
	src/exception.cpp \
	src/iomanip.cpp \
	src/ios.cpp \
	src/iostream.cpp \
	src/locale.cpp \
	src/mutex.cpp \
	src/new.cpp \
	src/stdexcept.cpp \
	src/string.cpp \
	src/system_error.cpp \
	src/thread.cpp \
	src/typeindex.cpp \
	src/typeinfo.cpp \
	src/internal/runtime.cpp \
	src/internal/trycatch.cpp \
	src/internal/unwind.cpp \
	src/internal/test/array.cpp \
	src/internal/test/bitset.cpp \
	src/internal/test/deque.cpp \
	src/internal/test/vector.cpp \
	src/internal/test/string.cpp \
	src/internal/test/test.cpp \
	src/internal/test/tuple.cpp

include $(USPACE_PREFIX)/Makefile.common
