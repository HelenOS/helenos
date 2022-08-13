#!/bin/sh

# git grep -l --null "^# Copyright\b" -- . | xargs -r -L1 -0 tools/transform-copyright.sh

FILENAME="$1"

grep -F '# Copyright' $FILENAME > $FILENAME.copyright__
grep -v -F '# Copyright' $FILENAME > $FILENAME.nocopyright__
head -n 27 $FILENAME.nocopyright__ > $FILENAME.license__

if diff -q $FILENAME.license__ license_text_tmp.txt; then
	tail -n +28 $FILENAME.nocopyright__ > $FILENAME.nolicense__
	echo '#!/usr/bin/env python3' > $FILENAME
	echo "#" >> $FILENAME
	sed 's/Copyright (c)/SPDX-FileCopyrightText:/g' $FILENAME.copyright__ >> $FILENAME
	echo "#" >> $FILENAME
	echo "# SPDX-License-Identifier: BSD-3-Clause" >> $FILENAME
	echo "#" >> $FILENAME
	
	cat $FILENAME.nolicense__ >> $FILENAME

	#rm $FILENAME.nolicense__
fi

#rm $FILENAME.copyright__
#rm $FILENAME.nocopyright__
#rm $FILENAME.license__

