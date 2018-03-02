#!/bin/sh

[ -z "$CROSS_HELENOS_PREFIX" ] && CROSS_HELENOS_PREFIX="/usr/local/cross-helenos"

get_define() {
	echo "$2" | "$1" -E -P -
}

print_error() {
	echo "  ERROR:" "$@"
}

check_define() {
	def_value=`get_define "$1" "$2"`
	if [ "$def_value" = "$3" ]; then
		return 0
	else
		print_error "Macro $1 not defined (expected '$3', got '$def_value')."
		return 1
	fi
}

print_version() {
	if [ -x "$1" ]; then
		echo " " `$1 --version 2>/dev/null | head -n 1` "@ $1"
	else
		print_error "$2";
		return 1
	fi
}

for arch_path in "$CROSS_HELENOS_PREFIX"/*; do
	arch=`echo "$arch_path" | sed -e 's#/$##' -e 's#.*/\([^/]*\)#\1#'`
	echo "Checking $arch..."

	gcc_path=`echo "$arch_path"/bin/*-gcc`
	ld_path=`echo "$arch_path"/bin/*-ld`
	objcopy_path=`echo "$arch_path"/bin/*-objcopy`
	gdb_path=`echo "$arch_path"/bin/*-gdb`

	print_version "$ld_path" "Linker not found!" || continue

	print_version "$objcopy_path" "objcopy not found!" || continue

	print_version "$gcc_path" "GCC not found!" || continue
	check_define "$gcc_path" "__helenos__" 1 || continue
	check_define "$gcc_path" "helenos_uarch" "$arch" || continue

	print_version "$gdb_path" "GDB not found!" || continue
done
