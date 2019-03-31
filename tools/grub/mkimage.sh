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
