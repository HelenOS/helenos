#!/usr/bin/awk -f

BEGIN {
	filename = ARGV[1]
	print "// Generated file. Fix the included header if static assert fails."
	print "#include \"" filename "\""
}

/}.*;/ {
	pattern = "}( __attribute__\\(.*\\))? (" struct_name "_t)?;"
	if ($0 !~ pattern) {
		print("Bad struct ending: " $0) > "/dev/stderr"
		exit 1
	}
	macro_name = toupper(struct_name) "_SIZE"
	print "_Static_assert(" macro_name " == sizeof(struct " struct_name "), \"\");"
	struct_name = ""
}

/;$/ {
	if (struct_name != "") {
		# Remove array subscript, if any.
		sub("(\\[.*\\])?;", "", $0)
		member = $NF

		macro_name = toupper(struct_name) "_OFFSET_" toupper(member)
		print "_Static_assert(" macro_name " == __builtin_offsetof(struct " struct_name ", " member "), \"\");"

		macro_name = toupper(struct_name) "_SIZE_" toupper(member)
		print "#ifdef " macro_name
		print "_Static_assert(" macro_name " == sizeof(((struct " struct_name "){})." member "), \"\");"
		print "#endif"
	}
}

/^typedef struct .* \{/ {
	struct_name = $3
}
