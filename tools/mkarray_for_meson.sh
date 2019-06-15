#!/bin/bash

TOOLS_DIR=`which -- "$0" 2>/dev/null`
if [ -z "$TOOLS_DIR" ]; then
	TOOLS_DIR=`which -- "$BASH_SOURCE" 2>/dev/null`
fi
TOOLS_DIR=`dirname $TOOLS_DIR`
TOOLS_DIR=`cd $TOOLS_DIR && echo $PWD`


_outdir="$1"
_arg1="$2"
_arg2="$3"
_arg3="$4"
_arg4="$5"
_inputs=""

shift 5

for file in "$@"; do
	_inputs="$_inputs $PWD/${file}"
done

cd $_outdir
$TOOLS_DIR/mkarray.py "$_arg1" "$_arg2" "$_arg3" "$_arg4" $_inputs > /dev/null
