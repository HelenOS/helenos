#!/bin/sh
#
# Copyright (c) 2023 Jiri Svoboda
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

SYCEK_GIT="https://github.com/jxsvoboda/sycek"
SYCEK_REV="e606f73fd875b5d9ff48261b23f69552a893f5d4"

if [ ! -d sycek ]; then
	git clone "$SYCEK_GIT" sycek
fi

cd sycek

# Make sure we have the required revision
echo "Making sure Sycek is up to date..."
git checkout "$SYCEK_REV" 2>/dev/null
rc=$?
if [ $rc != 0 ]; then
	echo "Pulling from Sycek repo..."
	git checkout master
	git pull
	git checkout "$SYCEK_REV" 2>/dev/null
	rc=$?
	if [ $rc != 0 ]; then
		echo "Error checking out Sycek rev $SYCEK_REV"
		exit 1
	fi

	make clean || exit 1
	make || exit 1
else
	make >/dev/null || exit 1
fi
