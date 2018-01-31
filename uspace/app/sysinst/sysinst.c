/*
 * Copyright (c) 2014 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup sysinst
 * @{
 */
/** @file System installer.
 *
 * Install the operating system onto a disk device. Note that this only works
 * on ia32/amd64 with Grub platform 'pc'.
 */

#include <block.h>
#include <byteorder.h>
#include <cap.h>
#include <errno.h>
#include <fdisk.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <task.h>
#include <vfs/vfs.h>
#include <str.h>

#include "futil.h"
#include "grub.h"

/** Device to install to
 *
 * Note that you cannot simply change this, because the installation
 * device is hardcoded in core.img. If you wanted to install to another
 * device, you must build your own core.img (e.g. using tools/grub/mkimage.sh
 * and modifying tools/grub/load.cfg, supplying the device to boot from
 * in Grub notation).
 */
#define DEFAULT_DEV "devices/\\hw\\pci0\\00:01.0\\ata-c1\\d0"
//#define DEFAULT_DEV "devices/\\hw\\pci0\\00:01.2\\uhci_rh\\usb01_a1\\mass-storage0\\l0"

/** Filysystem type. Cannot be changed without building a custom core.img */
#define FS_TYPE "mfs"

#define FS_SRV "/srv/mfs"
#define MOUNT_POINT "/inst"

/** Device containing HelenOS live CD */
#define CD_DEV "devices/\\hw\\pci0\\00:01.0\\ata-c2\\d0"

#define CD_FS_TYPE "cdfs"
#define CD_FS_SRV "/srv/cdfs"
#define CD_MOUNT_POINT "/cdrom"

#define BOOT_FILES_SRC "/cdrom"
#define BOOT_BLOCK_IDX 0 /* MBR */

/** Label the destination device.
 *
 * @param dev Disk device to label
 * @param pdev Place to store partition device name
 *
 * @return EOK on success or an error code
 */
static errno_t sysinst_label_dev(const char *dev, char **pdev)
{
	fdisk_t *fdisk;
	fdisk_dev_t *fdev;
	fdisk_part_t *part;
	fdisk_part_spec_t pspec;
	cap_spec_t cap;
	service_id_t sid;
	errno_t rc;

	printf("sysinst_label_dev(): get service ID '%s'\n", dev);
	rc = loc_service_get_id(dev, &sid, 0);
	if (rc != EOK)
		return rc;

	printf("sysinst_label_dev(): open device\n");

	rc = fdisk_create(&fdisk);
	if (rc != EOK) {
		printf("Error initializing fdisk.\n");
		return rc;
	}

	rc = fdisk_dev_open(fdisk, sid, &fdev);
	if (rc != EOK) {
		printf("Error opening device.\n");
		return rc;
	}

	printf("sysinst_label_dev(): create label\n");

	rc = fdisk_label_create(fdev, lt_mbr);
	if (rc != EOK) {
		printf("Error creating label: %s.\n", str_error(rc));
		return rc;
	}

	printf("sysinst_label_dev(): create partition\n");

	rc = fdisk_part_get_max_avail(fdev, spc_pri, &cap);
	if (rc != EOK) {
		printf("Error getting available capacity: %s.\n", str_error(rc));
		return rc;
	}

	fdisk_pspec_init(&pspec);
	pspec.capacity = cap;
	pspec.pkind = lpk_primary;
	pspec.fstype = fs_minix;

	rc = fdisk_part_create(fdev, &pspec, &part);
	if (rc != EOK) {
		printf("Error creating partition.\n");
		return rc;
	}

	/* XXX libfdisk should give us the service name */
	if (asprintf(pdev, "%sp1", dev) < 0)
		return ENOMEM;

	printf("sysinst_label_dev(): OK\n");
	return EOK;
}

/** Mount target file system.
 *
 * @param dev Partition device
 * @return EOK on success or an error code
 */
static errno_t sysinst_fs_mount(const char *dev)
{
	task_wait_t twait;
	task_exit_t texit;
	errno_t rc;
	int trc;

	printf("sysinst_fs_mount(): start filesystem server\n");
	rc = task_spawnl(NULL, &twait, FS_SRV, FS_SRV, NULL);
	if (rc != EOK)
		return rc;

	printf("sysinst_fs_mount(): wait for filesystem server\n");
	rc = task_wait(&twait, &texit, &trc);
	if (rc != EOK)
		return rc;

	printf("sysinst_fs_mount(): verify filesystem server result\n");
	if (texit != TASK_EXIT_NORMAL || trc != 0) {
		printf("sysinst_fs_mount(): not successful, but could be already loaded.\n");
	}

	rc = vfs_link_path(MOUNT_POINT, KIND_DIRECTORY, NULL);
	if (rc != EOK)
		return rc;

	printf("sysinst_fs_mount(): mount filesystem\n");
	rc = vfs_mount_path(MOUNT_POINT, FS_TYPE, dev, "", 0, 0);
	if (rc != EOK)
		return rc;

	printf("sysinst_fs_mount(): OK\n");
	return EOK;
}

/** Copy boot files.
 *
 * @return EOK on success or an error code
 */
static errno_t sysinst_copy_boot_files(void)
{
	task_wait_t twait;
	task_exit_t texit;
	errno_t rc;
	int trc;

	printf("sysinst_copy_boot_files(): start filesystem server\n");
	rc = task_spawnl(NULL, &twait, CD_FS_SRV, CD_FS_SRV, NULL);
	if (rc != EOK)
		return rc;

	printf("sysinst_copy_boot_files(): wait for filesystem server\n");
	rc = task_wait(&twait, &texit, &trc);
	if (rc != EOK)
		return rc;

	printf("sysinst_copy_boot_files(): verify filesystem server result\n");
	if (texit != TASK_EXIT_NORMAL || trc != 0) {
		printf("sysinst_fs_mount(): not successful, but could be already loaded.\n");
	}

	printf("sysinst_copy_boot_files(): create CD mount point\n");
	rc = vfs_link_path(CD_MOUNT_POINT, KIND_DIRECTORY, NULL);
	if (rc != EOK)
		return rc;

	printf("sysinst_copy_boot_files(): mount CD filesystem\n");
	rc = vfs_mount_path(CD_MOUNT_POINT, CD_FS_TYPE, CD_DEV, "", 0, 0);
	if (rc != EOK)
		return rc;

	printf("sysinst_copy_boot_files(): copy bootloader files\n");
	rc = futil_rcopy_contents(BOOT_FILES_SRC, MOUNT_POINT);
	if (rc != EOK)
		return rc;

	printf("sysinst_copy_boot_files(): unmount %s\n", MOUNT_POINT);
	rc = vfs_unmount_path(MOUNT_POINT);
	if (rc != EOK)
		return rc;

	printf("sysinst_copy_boot_files(): OK\n");
	return EOK;
}

/** Write unaligned 64-bit little-endian number.
 *
 * @param a Destination buffer
 * @param data Number
 */
static void set_unaligned_u64le(uint8_t *a, uint64_t data)
{
	int i;

	for (i = 0; i < 8; i++) {
		a[i] = (data >> (i * 8)) & 0xff;
	}
}

/** Copy boot blocks.
 *
 * Install Grub's boot blocks.
 *
 * @param devp Disk device
 * @return EOK on success or an error code
 */
static errno_t sysinst_copy_boot_blocks(const char *devp)
{
	void *boot_img;
	size_t boot_img_size;
	void *core_img;
	size_t core_img_size;
	service_id_t sid;
	size_t bsize;
	uint8_t bbuf[512];
	aoff64_t core_start;
	aoff64_t core_blocks;
	grub_boot_blocklist_t *first_bl, *bl;
	errno_t rc;

	printf("sysinst_copy_boot_blocks: Read boot block image.\n");
	rc = futil_get_file(BOOT_FILES_SRC "/boot/grub/i386-pc/boot.img",
	    &boot_img, &boot_img_size);
	if (rc != EOK || boot_img_size != 512)
		return EIO;

	printf("sysinst_copy_boot_blocks: Read GRUB core image.\n");
	rc = futil_get_file(BOOT_FILES_SRC "/boot/grub/i386-pc/core.img",
	    &core_img, &core_img_size);
	if (rc != EOK)
		return EIO;

	printf("sysinst_copy_boot_blocks: get service ID.\n");
	rc = loc_service_get_id(devp, &sid, 0);
	if (rc != EOK)
		return rc;

	printf("sysinst_copy_boot_blocks: block_init.\n");
	rc = block_init(sid, 512);
	if (rc != EOK)
		return rc;

	printf("sysinst_copy_boot_blocks: get block size\n");
	rc = block_get_bsize(sid, &bsize);
	if (rc != EOK)
		return rc;

	if (bsize != 512) {
		printf("Device block size != 512.\n");
		return EIO;
	}

	printf("sysinst_copy_boot_blocks: read boot block\n");
	rc = block_read_direct(sid, BOOT_BLOCK_IDX, 1, bbuf);
	if (rc != EOK)
		return EIO;

	core_start = 16;
	core_blocks = (core_img_size + 511) / 512;

	/* Clean blocklists */
	first_bl = core_img + 512 - sizeof(*first_bl);
	bl = first_bl;
	while (bl->len != 0) {
		memset(bl, 0, sizeof(*bl));
		--bl;
		if ((void *)bl < core_img) {
			printf("No block terminator in core image.\n");
			return EIO;
		}
	}

	first_bl->start = host2uint64_t_le(core_start + 1);
	first_bl->len = host2uint16_t_le(core_blocks - 1);
	first_bl->segment = grub_boot_i386_pc_kernel_seg + (512 >> 4);

	/* Write boot code into boot block */
	memcpy(bbuf, boot_img, 440); /* XXX 440 = sizeof(br_block_t.code_area) */
	bbuf[grub_boot_machine_boot_drive] = 0xff;
	set_unaligned_u64le(bbuf + grub_boot_machine_kernel_sector, core_start);

	printf("sysinst_copy_boot_blocks: write boot block\n");
	rc = block_write_direct(sid, BOOT_BLOCK_IDX, 1, bbuf);
	if (rc != EOK)
		return EIO;

	printf("sysinst_copy_boot_blocks: write boot block\n");
	/* XXX Must pad last block with zeros */
	rc = block_write_direct(sid, core_start, core_blocks, core_img);
	if (rc != EOK)
		return EIO;

	printf("sysinst_copy_boot_blocks: OK.\n");
	return EOK;
}

/** Install system to a device.
 *
 * @param dev Device to install to.
 * @return EOK on success or an error code
 */
static errno_t sysinst_install(const char *dev)
{
	errno_t rc;
	char *pdev;

	rc = sysinst_label_dev(dev, &pdev);
	if (rc != EOK)
		return rc;

	printf("Partition '%s'. Mount it.\n", pdev);
	rc = sysinst_fs_mount(pdev);
	if (rc != EOK)
		return rc;

	free(pdev);

	printf("FS created and mounted. Copying boot files.\n");
	rc = sysinst_copy_boot_files();
	if (rc != EOK)
		return rc;

	printf("Boot files done. Installing boot blocks.\n");
	rc = sysinst_copy_boot_blocks(dev);
	if (rc != EOK)
		return rc;

	return EOK;
}

int main(int argc, char *argv[])
{
	const char *dev = DEFAULT_DEV;
	return sysinst_install(dev);
}

/** @}
 */
