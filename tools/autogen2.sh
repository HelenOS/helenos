#!/bin/sh

# There's a bunch of restriction. The script assumes that the structure definition begins with "typedef struct struct_name {",
# ends with "struct_name_t;", and that every word in between that ends with ";" is a struct member name with optional
# array subscript. Error handling is mostly omitted for simplicity, so any input that does not follow these rules will result
# in cryptic errors.

input="$1"
output="$2"
toolsdir="$(readlink -f $(dirname "$0"))"
awkscript="$toolsdir/autogen2.awk"

# Set default value for $CC.
if [ -z "${CC}" ]; then
	CC=cc
fi

# If $CC is clang, we need to make sure integrated assembler is not used.
if ( ${CC} --version | grep clang > /dev/null ) ; then
	CFLAGS="${CFLAGS} -no-integrated-as"
fi

# Tell the compiler to generate makefile dependencies.
CFLAGS="${CFLAGS} -MD -MP -MT $output -MF $output.d"

# Generate defines
defs=`$awkscript -- $input || exit 1`

# Generate C file.
cat > $output.c <<- EOF

#include "`basename $input`"

#define DEFINE_MEMBER(ls, us, lm, um) \
	asm volatile ("/* #define "#us"_OFFSET_"#um" %0 */" :: "i" (__builtin_offsetof(struct ls, lm))); \
	asm volatile ("/* #define "#us"_SIZE_"#um" %0 */" :: "i" (sizeof((struct ls){}.lm)));

#define DEFINE_STRUCT(ls, us) \\
	asm volatile ("/* #define "#us"_SIZE %0 */" :: "i" (sizeof(struct ls)));

void autogen()
{
	$defs
}

EOF

# Turn the C file into assembly.
${CC} ${CFLAGS} -w -S -o $output.s $output.c || exit 1

# Process the output.

echo "/* Autogenerated file, do not modify. */" > $output
echo "#pragma once" >> $output
sed -n 's/^.* #define \([^ ]\+\) [^0-9]*\([0-9]\+\) .*/#define \1 \2/p' < $output.s > $output || exit 1