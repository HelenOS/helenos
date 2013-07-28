#!/bin/sh

set -e

find "$1" -name '*.h' -exec \
	sed -n -e '/^#/d' -e 's/__POSIX_DEF__/\n&/gp' {} \; | \
	sed -n -e 's/__POSIX_DEF__(\([^)]*\)).*/\1/p' | \
	sort -u
