#
# Provide a string to be included in a special DT_INTERP header, even though
# this is a statically-linked executable. This will mark the binary as
# the program loader.
#

#ifdef UARCH_arm32
#define AT_NOTE	%note
#else
#define AT_NOTE	@note
#endif

.section .interp, "a", AT_NOTE
	.string "kernel"
