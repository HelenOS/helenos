#!/usr/bin/awk -f

/}.*;/ {
	if (($0 != "};") && ($0 != "} " struct_name "_t;")) {
		print("Bad struct ending: " $0) > "/dev/stderr"
		exit 1
	}
	print "DEFINE_STRUCT(" struct_name ", " toupper(struct_name) ")"
	struct_name = ""
}

/;$/ {
	if (struct_name != "") {
		# Remove array subscript, if any.
		sub("(\\[.*\\])?;", "", $0)
		member = $NF
		print "DEFINE_MEMBER(" struct_name ", " toupper(struct_name) ", " member ", " toupper(member) ")"
	}
}

/^typedef struct .* \{/ {
	struct_name = $3
}
