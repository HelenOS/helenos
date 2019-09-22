#!/usr/bin/awk -f
#
# Copyright (c) 2018 CZ.NIC, z.s.p.o.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
