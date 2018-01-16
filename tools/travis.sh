#!/bin/sh

#
# Copyright (c) 2018 Vojtech Horky
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
# This is wrapper script for testing build of HelenOS under Travis CI [1].
#
# You probably do not want to run this script directly. If you wish to test
# that HelenOS builds for all architectures, consider using either check.sh
# script or running our CI solution [2] (if you want to test ported software
# too).
#
# [1] https://travis-ci.org/
# [2] http://www.helenos.org/wiki/CI
#

H_ARCH_CONFIG_CROSS_TARGET=2
H_ARCH_CONFIG_OUTPUT_FILENAME=3

h_get_arch_config_space() {
    cat <<'EOF_CONFIG_SPACE'
amd64:amd64-unknown-elf:image.iso
arm32/beagleboardxm:arm-linux-gnueabi:uImage.bin
arm32/beaglebone:arm-linux-gnueabi:uImage.bin
arm32/gta02:arm-linux-gnueabi:uImage.bin
arm32/integratorcp:arm-linux-gnueabi:image.boot
arm32/raspberrypi:arm-linux-gnueabi:uImage.bin
ia32:i686-pc-linux-gnu:image.iso
ia64/i460GX:ia64-pc-linux-gnu:image.boot
ia64/ski:ia64-pc-linux-gnu:image.boot
mips32/malta-be:mips-linux-gnu:image.boot
mips32/malta-le:mipsel-linux-gnu:image.boot
mips32/msim:mipsel-linux-gnu:image.boot
ppc32:ppc-linux-gnu:image.iso
sparc64/niagara:sparc64-linux-gnu:image.iso
sparc64/ultra:sparc64-linux-gnu:image.iso
EOF_CONFIG_SPACE
}

h_get_arch_config() {
    h_get_arch_config_space | grep "^$H_ARCH:" | cut '-d:' -f "$1"
}

H_DEFAULT_HARBOURS_LIST="binutils fdlibm jainja libgmp libiconv msim pcc zlib libisl libmpfr libpng python2 libmpc gcc"



#
# main script starts here
#

# Check we are actually running inside Travis
if [ -z "$TRAVIS" ]; then
    echo "\$TRAVIS env not set. Are you running me inside Travis?" >&2
    exit 5
fi

# Check HelenOS configuration was set-up
if [ -z "$H_ARCH" ]; then
    echo "\$H_ARCH env not set. Are you running me inside Travis?" >&2
    exit 5
fi

# Check cross-compiler target
H_CROSS_TARGET=`h_get_arch_config $H_ARCH_CONFIG_CROSS_TARGET`
if [ -z "$H_CROSS_TARGET" ]; then
    echo "No suitable cross-target found for '$H_ARCH.'" >&2
    exit 1
fi


# Custom CROSS_PREFIX
export CROSS_PREFIX=/usr/local/cross-static/

# Default Harbours repository
if [ -z "$H_HARBOURS_REPOSITORY" ]; then
    H_HARBOURS_REPOSITORY="https://github.com/HelenOS/harbours.git"
fi

if [ "$1" = "help" ]; then
    echo
    echo "Following variables needs to be set prior running this script."
    echo "Example settings follows:"
    echo
    echo "export H_ARCH=$H_ARCH"
    echo "export TRAVIS_BUILD_ID=`date +%s`"
    echo
    echo "export H_HARBOURS=true"
    echo "export H_HARBOUR_LIST=\"$H_DEFAULT_HARBOURS_LIST\""
    echo
    exit 0

elif [ "$1" = "install" ]; then
    set -x
    
    # Install dependencies
    sudo apt-get -qq update || exit 1
    sudo apt-get install -y genisoimage || exit 1

    # Fetch and install cross-compiler
    wget "http://ci.helenos.org/download/helenos-cross-$H_CROSS_TARGET.static.tar.xz" -O "/tmp/cross-$H_CROSS_TARGET.static.tar.xz" || exit 1
    sudo mkdir -p "$CROSS_PREFIX" || exit 1
    sudo tar -xJ -C "$CROSS_PREFIX" -f "/tmp/cross-$H_CROSS_TARGET.static.tar.xz" || exit 1
    exit 0


elif [ "$1" = "run" ]; then
    set -x
    
    # Expected output filename (bootable image)
    H_OUTPUT_FILENAME=`h_get_arch_config $H_ARCH_CONFIG_OUTPUT_FILENAME`
    if [ -z "$H_OUTPUT_FILENAME" ]; then
        echo "No suitable output image found for '$H_ARCH.'" >&2
        exit 1
    fi

    # Build HARBOURs too?
    H_HARBOURS=`echo "$H_HARBOURS" | grep -e '^true$' -e '^false$'`
    if [ -z "$H_HARBOURS" ]; then
        H_HARBOURS=false
    fi
    if [ -z "$H_HARBOUR_LIST" ]; then
        H_HARBOUR_LIST="$H_DEFAULT_HARBOURS_LIST"
    fi

	
    # Build it
    make "PROFILE=$H_ARCH" HANDS_OFF=y || exit 1
    test -s "$H_OUTPUT_FILENAME" || exit 1
    
    echo
    echo "HelenOS for $H_ARCH built okay."
    echo

    # Build harbours
    if $H_HARBOURS; then
        echo
        echo "Will try to build ported software for $H_ARCH."
        echo "Repository used is $H_HARBOURS_REPOSITORY."
        echo
        
        H_HELENOS_HOME=`pwd`
        cd "$HOME" || exit 1
        git clone --depth 10 "$H_HARBOURS_REPOSITORY" helenos-harbours || exit 1
        mkdir "build-harbours-$TRAVIS_BUILD_ID" || exit 1
        (
            cd "build-harbours-$TRAVIS_BUILD_ID" || exit 1
            mkdir build || exit 1
            cd build

            (
                #[ "$H_ARCH" = "mips32/malta-be" ] && H_ARCH="mips32eb/malta-be"
                echo "root = $H_HELENOS_HOME"
                echo "arch =" `echo "$H_ARCH" | cut -d/ -f 1`
                echo "machine =" `echo "$H_ARCH" | cut -d/ -f 2`
            ) >hsct.conf || exit 1
            
            # "$HOME/helenos-harbours/hsct.sh" init "$H_HELENOS_HOME" "$H_ARCH" build >/dev/null 2>/dev/null || exit 1
            
            "$HOME/helenos-harbours/hsct.sh" update || exit 1

            FAILED_HARBOURS=""    
            for HARBOUR in $H_HARBOUR_LIST; do
                "$HOME/helenos-harbours/hsct.sh" archive --no-deps "$HARBOUR" >"run-$HARBOUR.log" 2>&1
                if [ $? -eq 0 ]; then
                    tail -n 10 "run-$HARBOUR.log"
                else
                    FAILED_HARBOURS="$FAILED_HARBOURS $HARBOUR"
                    cat build/$HARBOUR/*/config.log
                    tail -n 100 "run-$HARBOUR.log"
                fi
                
            done
            
            if [ -n "$FAILED_HARBOURS" ]; then
                echo
                echo "ERROR: following packages were not built:$FAILED_HARBOURS."
                echo
                exit 1
            fi
        ) || exit 1
    fi
else
    echo "Invalid action specified." >&2
    exit 5
fi

