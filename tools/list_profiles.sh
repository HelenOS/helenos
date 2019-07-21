#!/bin/sh

# Find out the path to the script.
SOURCE_DIR=`which -- "$0" 2>/dev/null`
# Maybe we are running bash.
[ -z "$SOURCE_DIR" ] && SOURCE_DIR=`which -- "$BASH_SOURCE"`
[ -z "$SOURCE_DIR" ] && exit 1
SOURCE_DIR=`dirname -- "$SOURCE_DIR"`
SOURCE_DIR=`cd $SOURCE_DIR && cd .. && echo $PWD`

CONFIG_DEFAULTS="${SOURCE_DIR}/defaults"

# Find all the leaf subdirectories in the defaults directory.
find ${CONFIG_DEFAULTS} -type d -links 2 -printf "%P\n" | sort
