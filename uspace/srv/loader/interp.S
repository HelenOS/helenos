#
# Provide a string to be included in a special DT_INTERP header, even though
# this is a statically-linked executable. This will mark the binary as
# the program loader.
#

#if ((defined(UARCH_abs32le)) && (defined(COMPILER_gcc_cross)) \
    && (defined(CROSS_TARGET_arm32)))
	#define ATSIGN(arg)  % ## arg
#endif

#ifdef UARCH_arm32
	#define ATSIGN(arg)  % ## arg
#endif

#ifndef ATSIGN
	#define ATSIGN(arg)  @ ## arg
#endif

.section .interp, "a", ATSIGN(progbits)
	.string "kernel"
