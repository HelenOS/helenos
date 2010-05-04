#!/bin/bash

# Download SILO and patch it so that it can be used to create a bootable CD
# for the Serengeti machine
#  by Pavel Rimsky <rimskyp@seznam.cz>
#  portions by Martin Decky <martin@decky.cz>
#
#  GPL'ed, copyleft
#

# stuff to be downloaded
SILO_DOWNLOAD_FILE='silo-loaders-1.4.11.tar.gz'
SILO_DOWNLOAD_URL='http://silo.auxio.org/pub/silo/old/'$SILO_DOWNLOAD_FILE

# check whether the last command failed, if so, write an error message and exit
check_error() {
    if [ "$1" -ne "0" ]; then
        echo
        echo "Script failed: $2"
        exit
    fi
}

# temporary files are to be stored in /tmp
# the resulting file in the current directory
WD=`pwd`
cd /tmp

# download SILO from its official website
echo ">>> Downloading SILO"
wget $SILO_DOWNLOAD_URL
check_error $? "Error downloading SILO."

# unpack the downloaded file
echo ">>> Unpacking tarball"
tar xvzf $SILO_DOWNLOAD_FILE
check_error $? "Error unpacking tarball."

# CD to the unpacked directory
echo ">>> Changing to the unpacked SILO directory"
cd boot
check_error $? "Changing directory failed."

# patch it - remove bytes 512 to 512 + 32 (counted from 0), which belong to
# the ELF header which is not recognized by the Serengeti firmware
echo ">>> Patching SILO"
(((xxd -p -l 512 isofs.b) && (xxd -p -s 544 isofs.b)) | xxd -r -p) \
	 > isofs.b.patched
check_error $? "Patching SILO failed"
mv isofs.b.patched isofs.b

# get rid of files which are not needed for creating the bootable CD
echo ">>> Purging SILO directory"
for file in `ls`; do
	if [ \( -f $file \) -a \( $file != "isofs.b" \) -a \( $file != "second.b" \) ];
		then
		rm -fr $file;
	fi
done
check_error $? "Purging SILO directory failed"

# create the gzipped tarball with patched SILO
echo ">>> Creating tarball with patched SILO"
tar cvzf silo.patched.tar.gz *.b
check_error $? "Creating tarball with patched SILO failed"

# and move it to the directory where the user expects it to be
echo ">>> Moving the tarball with patched SILO to the current directory"
mv silo.patched.tar.gz $WD
check_error $? "Moving the tarball with patched SILO failed"

# move back to the working directory from /tmp
cd $WD

