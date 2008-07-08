#
# Provide a string to be included in a special DT_INTERP header, even though
# this is a statically-linked executable. This will mark the binary as
# the program loader.
#
.section .interp , ""
	.string "kernel"
