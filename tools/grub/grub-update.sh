#!/bin/bash
#
# Copyright (c) 2016 Jiri Svoboda
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

#
# Script to generate/update prebuilt Grub2 in HelenOS source tree
# Be sure you know what you are doing!
#

origdir="$(cd "$(dirname "$0")" && pwd)"
helenosdir="$origdir/../.."
workdir="$origdir/grub-src"
builddir="$origdir/grub-build"
git_repo="git://git.savannah.gnu.org/grub.git"
grub_rev="bc220962e366b1b46769ed6f9fa5be603ba58ab5"

build_ia32amd64_pc=false
build_ia32amd64_efi=false
build_arm64_efi=false

function grub_build()
{
	platform="$1"
	conf_target="$2"
	conf_platform="$3"

	./configure --prefix="$builddir/$platform" --target="$conf_target" --with-platform="$conf_platform" --disable-werror || exit 1
	make clean || exit 1
	make install || exit 1
}

function grub_files_update()
{
	gdir="$1"
	platform="$2"

	echo "$grub_rev" > "$helenosdir"/boot/grub/"$gdir"/REVISION || exit 1
	rm -rf "$helenosdir"/boot/grub/"$gdir"/"$platform" || exit 1
	cp -R "$builddir"/"$platform"/lib*/grub/"$platform" "$helenosdir"/boot/grub/"$gdir" || exit 1
	rm -f "$helenosdir"/boot/grub/"$gdir"/"$platform"/*.image || exit 1
	rm -f "$helenosdir"/boot/grub/"$gdir"/"$platform"/*.module || exit 1
	git add "$helenosdir"/boot/grub/"$gdir"/"$platform" || exit 1
}

function show_usage()
{
	echo "Script to generate/update prebuild Grub2 in HelenOS source tree"
	echo
	echo "Syntax:"
	echo " $0 <target>"
	echo
	echo "Possible targets are:"
	echo " ia32+amd64-pc"
	echo " ia32+amd64-efi"
	echo " arm64-efi"
	echo " all             build all targets"
	echo

	exit 3
}

if [ "$#" -ne "1" ] ; then
	show_usage
fi

case "$1" in
	ia32+amd64-pc)
		build_ia32amd64_pc=true
		;;
	ia32+amd64-efi)
		build_ia32amd64_efi=true
		;;
	arm64-efi)
		build_arm64_efi=true
		;;
	all)
		build_ia32amd64_pc=true
		build_ia32amd64_efi=true
		build_arm64_efi=true
		;;
	*)
		show_usage
		;;
esac

# Prepare a clone of Grub2 repo
if [ ! -d "$workdir" ] ; then
	rm -rf "$workdir" "$builddir" || exit 1
	git clone "$git_repo" "$workdir" || exit 1
fi

cd "$workdir" || exit 1
git pull || exit 1
git reset --hard "$grub_rev" || exit 1

./autogen.sh || exit 1

if $build_ia32amd64_pc ; then
	cd "$workdir" || exit 1
	grub_build i386-pc i386 pc

	# Extract El Torrito boot image for i386-pc
	cd "$helenosdir"/boot/grub/ia32-pc || exit 1
	rm -f pc.img || exit 1
	"$builddir"/i386-pc/bin/grub-mkrescue -o phony --xorriso="$origdir/getimage.sh" || exit 1

	grub_files_update ia32-pc i386-pc
fi

if $build_ia32amd64_efi ; then
	cd "$workdir" || exit 1

	# Build each platform to a different directory
	grub_build i386-efi i386 efi
	grub_build x86_64-efi x86_64 efi

	# Extract El Torrito boot image for i386-efi
	cd "$helenosdir"/boot/grub/ia32-efi || exit 1
	rm -f efi.img.gz || exit 1
	"$builddir"/i386-efi/bin/grub-mkrescue -o phony --xorriso="$origdir/getimage.sh" || exit 1
	mv efi.img i386-efi.img

	# Extract El Torrito boot image for x86_64-efi
	cd "$helenosdir"/boot/grub/ia32-efi || exit 1
	rm -f efi.img.gz || exit 1
	"$builddir"/x86_64-efi/bin/grub-mkrescue -o phony --xorriso="$origdir/getimage.sh" || exit 1

	# Combine El Torrito boot images for x86_64-efi and i386-efi
	mkdir tmp || exit 1
	mcopy -ns -i i386-efi.img ::efi tmp || exit 1
	mcopy -s -i efi.img tmp/* :: || exit 1
	gzip efi.img || exit 1
	rm -rf tmp || exit 1
	rm -f i386-efi.img || exit 1

	# Update Grub files for all platforms
	grub_files_update ia32-efi i386-efi
	grub_files_update ia32-efi x86_64-efi
fi

if $build_arm64_efi ; then
	cd "$workdir" || exit 1

	# Check that the expected compiler is present on PATH
	if ! [ -x "$(command -v aarch64-linux-gnu-gcc)" ] ; then
		echo "Error: aarch64-linux-gnu-gcc is missing" >&2
		exit 1
	fi

	grub_build arm64-efi aarch64-linux-gnu efi

	# Extract El Torrito boot image for arm64-efi
	cd "$helenosdir"/boot/grub/arm64-efi || exit 1
	rm -f efi.img.gz || exit 1
	"$builddir"/arm64-efi/bin/grub-mkrescue -o phony --xorriso="$origdir/getimage.sh" || exit 1
	gzip efi.img || exit 1

	grub_files_update arm64-efi arm64-efi
fi

echo "GRUB update successful."
