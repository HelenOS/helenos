#!/bin/bash
#
# SPDX-FileCopyrightText: 2016 Jiri Svoboda
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Build core.img GRUB image file for use by HelenOS installer (sysinst)
# Note that we need a suitable build of Grub2 utilities, such as the one
# we get by running grub-update.sh
#
# When sysinst can build core.img itself, this script (and its product,
# prebuilt core.img in the source tree) will be no longer needed.
#

orig_dir="$(cd "$(dirname "$0")" && pwd)"
grub_tools_dir="$orig_dir/grub-build/i386-pc/bin"
grub_mod_dir="$orig_dir/../../boot/grub/ia32-pc/i386-pc"

"$grub_tools_dir"/grub-mkimage --directory "$grub_mod_dir" \
    --prefix "$grub_mod_dir" --output "$grub_mod_dir/core.img" \
    --format 'i386-pc' --compression 'auto' --config "$orig_dir/load.cfg" \
    "ext2" "part_msdos" "biosdisk" "search_fs_uuid"
