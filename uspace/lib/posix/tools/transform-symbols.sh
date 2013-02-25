#!/bin/sh

OBJCOPY="$1"
AR="$2"
ECHO="$3"
LIB_SOURCE="$4"
LIB_RESULT="$5"
REDEFS_FIRST="$6"
REDEFS_SECOND="$7"

set -e

rm -f "$LIB_RESULT"

$ECHO -n "$LIB_RESULT:"

$AR t "$LIB_SOURCE" | sort | uniq -c | while read count filename; do
	rm -f "$filename"
	$ECHO -n " $filename"
	for idx in `seq 1 $count`; do
		$AR xN $idx "$LIB_SOURCE" "$filename"
		xargs "$OBJCOPY" "$filename" "$filename" <"$REDEFS_FIRST"
		xargs "$OBJCOPY" "$filename" "$filename" <"$REDEFS_SECOND"
		$AR qc "$LIB_RESULT" "$filename"
		rm -f "$filename"
	done
done

$AR s "$LIB_RESULT"

$ECHO ""
