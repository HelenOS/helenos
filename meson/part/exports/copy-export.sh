#!/bin/sh

#
# Copyright (c) 2024 Vojtech Horky
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

set -ue

target_dir="${DESTDIR:-export-dev}"

rm -rf "${target_dir:?}/lib"
rm -rf "${target_dir}/include"

mkdir -p "${target_dir}/lib"
mkdir -p "${target_dir}/include"


while [ "$#" -gt 0 ]; do
    case "$1" in
        staticlib)
            ar -t "$2" | xargs ar crs "${target_dir}/lib/$3"
            ;;
        sharedlib)
            filename="$(basename "$2")"
            major_versioned_name="$3"
            # libfoo.so.5 -> libfoo.so (remove the last part delimited by dot)
            unversioned_name="${major_versioned_name%.[0-9]*}"
            cp -L "$2" "${target_dir}/lib/$filename"
            ln -s "$filename" "${target_dir}/lib/$major_versioned_name"
            ln -s "$filename" "${target_dir}/lib/$unversioned_name"
            ;;
        include)
            mkdir -p "${target_dir}/include/$3"
            cp -r -L -t "${target_dir}/include/$3" "$2"/*
            ;;
        includesymlink)
            (
                cd "${target_dir}/include/$3" && ln -s "../$2" .
            )
            ;;
        includenamedsymlink)
            (
                cd "${target_dir}/include/" && ln -s "$2" "$3"
            )
            ;;
        config)
            cp -L "$2" "${target_dir}/$3"
            ;;
        *)
            echo "Unknown type $1, aborting." >&2
            exit 1
            ;;
    esac
    shift 3
done
