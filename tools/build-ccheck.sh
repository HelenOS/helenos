#!/bin/sh
#
# SPDX-FileCopyrightText: 2021 Jiri Svoboda
#
# SPDX-License-Identifier: BSD-3-Clause
#

SYCEK_GIT="https://github.com/jxsvoboda/sycek"
SYCEK_REV="9946e29fbd013de4bc0277cd475555f3282e24f3"

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
