#!/usr/bin/awk -f
#
# SPDX-FileCopyrightText: 2018 CZ.NIC, z.s.p.o.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Authors:
# 	Jiří Zárevúcky <jiri.zarevucky@nic.cz>

BEGIN {
	filename = ARGV[1]
	output_lines = 0
	print "// Generated file. Fix the included header if static assert fails."
	print "// Inlined \"" filename "\""
}

{
	print $0
}

/}.*;/ {
	pattern = "}( __attribute__\\(.*\\))? (" struct_name "_t)?;"
	if ($0 !~ pattern) {
		print("Bad struct ending: " $0) > "/dev/stderr"
		exit 1
	}
	macro_name = toupper(struct_name) "_SIZE"
	output[output_lines++] = "_Static_assert(" macro_name " == sizeof(struct " struct_name "), \"\");"
	struct_name = ""
}

/;$/ {
	if (struct_name != "") {
		# Remove array subscript, if any.
		sub("(\\[.*\\])?;", "", $0)
		member = $NF

		macro_name = toupper(struct_name) "_OFFSET_" toupper(member)
		output[output_lines++] = "_Static_assert(" macro_name " == __builtin_offsetof(struct " struct_name ", " member "), \"\");"

		macro_name = toupper(struct_name) "_SIZE_" toupper(member)
		output[output_lines++] = "#ifdef " macro_name
		output[output_lines++] = "_Static_assert(" macro_name " == sizeof(((struct " struct_name "){ })." member "), \"\");"
		output[output_lines++] = "#endif"
	}
}

/^typedef struct .* \{/ {
	struct_name = $3
}

END {
	for ( i = 0; i < output_lines; i++ ) {
		print output[i]
	}
}
