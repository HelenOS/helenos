/*
 * Copyright (c) 2024 Jiri Svoboda
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
#include <capa.h>
#include <errno.h>
#include <fdisk.h>
#include <futil.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <vfs/vfs.h>
#include <vol.h>

#include "grub.h"
#include "rdimg.h"
#include "volume.h"

/** Device to install to
 *
 * Note that you cannot simply change this, because the installation
 * device is hardcoded in core.img. If you wanted to install to another
 * device, you must build your own core.img (e.g. using tools/grub/mkimage.sh
 * and modifying tools/grub/load.cfg, supplying the device to boot from
 * in Grub notation).
 */
#define DEFAULT_DEV_0 "devices/\\hw\\sys\\00:01.1\\c0d0"
#define DEFAULT_DEV_1 "devices/\\hw\\sys\\00:01.0\\ata1\\c0d0"
//#define DEFAULT_DEV "devices/\\hw\\pci0\\00:01.2\\uhci_rh\\usb01_a1\\mass-storage0\\l0"
/** Volume label for the new file system */
#define INST_VOL_LABEL "HelenOS"
/** Mount point of system partition when running installed system */
#define INST_VOL_MP "/w"

#define MOUNT_POINT "/inst"

/** HelenOS live CD volume label */
#define CD_VOL_LABEL "HelenOS-CD"
/** XXX Should get this from the volume server */
#define CD_MOUNT_POINT "/vol/" CD_VOL_LABEL

#define BOOT_FILES_SRC CD_MOUNT_POINT
#define BOOT_BLOCK_IDX 0 /* MBR */

#define CFG_FILES_SRC "/cfg"
#define CFG_FILES_DEST MOUNT_POINT "/cfg"

static const char *default_devs[] = {
	DEFAULT_DEV_0,
	DEFAULT_DEV_1,
	NULL
};

static const char *sys_dirs[] = {
	"/cfg",
	"/data",
	NULL
};

/** Check the if the destination device exists.
 *
 * @param dev Disk device
 *
 * @return EOK on success or an error code
 */
static errno_t sysinst_check_dev(const char *dev)
{
	service_id_t sid;
	errno_t rc;

	rc = loc_service_get_id(dev, &sid, 0);
	if (rc != EOK)
		return rc;

	(void)sid;
	return EOK;
}

/** Label the destination device.
 *
 * @param dev Disk device to label
 * @param psvc_id Place to store service ID of the created partition
 *
 * @return EOK on success or an error code
 */
static errno_t sysinst_label_dev(const char *dev, service_id_t *psvc_id)
{
	fdisk_t *fdisk;
	fdisk_dev_t *fdev;
	fdisk_part_t *part;
	fdisk_part_spec_t pspec;
	fdisk_part_info_t pinfo;
	capa_spec_t capa;
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

	printf("sysinst_label_dev(): create mount directory\n");

	rc = vfs_link_path(MOUNT_POINT, KIND_DIRECTORY, NULL);
	if (rc != EOK)
		return rc;

	printf("sysinst_label_dev(): create label\n");

	rc = fdisk_label_create(fdev, lt_mbr);
	if (rc != EOK) {
		printf("Error creating label: %s.\n", str_error(rc));
		return rc;
	}

	printf("sysinst_label_dev(): create partition\n");

	rc = fdisk_part_get_max_avail(fdev, spc_pri, &capa);
	if (rc != EOK) {
		printf("Error getting available capacity: %s.\n", str_error(rc));
		return rc;
	}

	fdisk_pspec_init(&pspec);
	pspec.capacity = capa;
	pspec.pkind = lpk_primary;
	pspec.fstype = fs_ext4; /* Cannot be changed without modifying core.img */
	pspec.mountp = MOUNT_POINT;
	pspec.label = INST_VOL_LABEL;

	rc = fdisk_part_create(fdev, &pspec, &part);
	if (rc != EOK) {
		printf("Error creating partition.\n");
		return rc;
	}

	rc = fdisk_part_get_info(part, &pinfo);
	if (rc != EOK) {
		printf("Error getting partition information.\n");
		return rc;
	}

	printf("sysinst_label_dev(): OK\n");
	*psvc_id = pinfo.svc_id;
	return EOK;
}

/** Set up system volume structure.
 *
 * @return EOK on success or an error code
 */
static errno_t sysinst_setup_sysvol(void)
{
	errno_t rc;
	char *path = NULL;
	const char **cp;
	int rv;

	cp = sys_dirs;
	while (*cp != NULL) {
		rv = asprintf(&path, "%s%s", MOUNT_POINT, *cp);
		if (rv < 0) {
			rc = ENOMEM;
			goto error;
		}

		rc = vfs_link_path(path, KIND_DIRECTORY, NULL);
		if (rc != EOK) {
			printf("Error creating directory '%s'.\n", path);
			goto error;
		}

		free(path);
		path = NULL;
		++cp;
	}

	free(path);
	path = NULL;

	/* Copy initial configuration files */
	rc = futil_rcopy_contents(CFG_FILES_SRC, CFG_FILES_DEST);
	if (rc != EOK)
		return rc;

	return EOK;
error:
	if (path != NULL)
		free(path);
	return rc;
}

/** Copy boot files.
 *
 * @return EOK on success or an error code
 */
static errno_t sysinst_copy_boot_files(void)
{
	errno_t rc;

	printf("sysinst_copy_boot_files(): copy bootloader files\n");
	rc = futil_rcopy_contents(BOOT_FILES_SRC, MOUNT_POINT);
	if (rc != EOK)
		return rc;

	printf("sysinst_copy_boot_files(): OK\n");
	return EOK;
}

/** Set up configuration in the initial RAM disk.
 *
 * @return EOK on success or an error code
 */
static errno_t sysinst_customize_initrd(void)
{
	errno_t rc;
	rd_img_t *rd = NULL;
	char *rdpath = NULL;
	char *path = NULL;
	vol_volumes_t *volumes = NULL;
	vol_volume_t *volume = NULL;
	int rv;

	rc = rd_img_open(MOUNT_POINT "/boot/initrd.img", &rdpath, &rd);
	if (rc != EOK) {
		printf("Error opening initial RAM disk image.\n");
		goto error;
	}

	rv = asprintf(&path, "%s%s", rdpath, "/cfg/initvol.sif");
	if (rv < 0) {
		rc = ENOMEM;
		goto error;
	}

	printf("Configuring volume server.\n");
	rc = vol_volumes_create(path, &volumes);
	if (rc != EOK) {
		printf("Error creating volume server configuration.\n");
		rc = EIO;
		goto error;
	}

	printf("Configuring volume server: look up volume\n");
	rc = vol_volume_lookup_ref(volumes, INST_VOL_LABEL, &volume);
	if (rc != EOK) {
		printf("Error creating volume server configuration.\n");
		rc = EIO;
		goto error;
	}

	printf("Configuring volume server: set mount point\n");
	rc = vol_volume_set_mountp(volume, INST_VOL_MP);
	if (rc != EOK) {
		printf("Error creating system partition configuration.\n");
		rc = EIO;
		goto error;
	}

	rc = vol_volumes_sync(volumes);
	if (rc != EOK) {
		printf("Error saving volume confiuration.\n");
		goto error;
	}

	printf("Configuring volume server: delete reference\n");
	vol_volume_del_ref(volume);
	volume = NULL;
	printf("Configuring volume server: destroy volumes object\n");
	vol_volumes_destroy(volumes);
	volumes = NULL;

	rc = rd_img_close(rd);
	if (rc != EOK) {
		printf("Error closing initial RAM disk image.\n");
		rc = EIO;
		goto error;
	}

	free(rdpath);
	rdpath = NULL;
	free(path);
	path = NULL;

	return EOK;
error:
	if (volume != NULL)
		vol_volume_del_ref(volume);
	if (volumes != NULL)
		vol_volumes_destroy(volumes);
	if (rd != NULL)
		(void) rd_img_close(rd);
	if (path != NULL)
		free(path);
	if (rdpath != NULL)
		free(rdpath);
	return rc;
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
	rc = block_init(sid);
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

	printf("sysinst_copy_boot_blocks: write core blocks\n");
	/* XXX Must pad last block with zeros */
	rc = block_write_direct(sid, core_start, core_blocks, core_img);
	if (rc != EOK)
		return EIO;

	printf("sysinst_copy_boot_blocks: OK.\n");
	return EOK;
}

/** Eject installation volume.
 *
 * @param psvc_id Partition service ID
 */
static errno_t sysinst_eject_dev(service_id_t part_id)
{
	vol_t *vol = NULL;
	errno_t rc;

	rc = vol_create(&vol);
	if (rc != EOK) {
		printf("Error contacting volume service.\n");
		goto out;
	}

	rc = vol_part_eject(vol, part_id);
	if (rc != EOK) {
		printf("Error ejecting volume.\n");
		goto out;
	}

	rc = EOK;
out:
	vol_destroy(vol);
	return rc;
}

/** Install system to a device.
 *
 * @param dev Device to install to.
 * @return EOK on success or an error code
 */
static errno_t sysinst_install(const char *dev)
{
	errno_t rc;
	service_id_t psvc_id;

	rc = sysinst_label_dev(dev, &psvc_id);
	if (rc != EOK)
		return rc;

	printf("FS created and mounted. Creating system directory structure.\n");
	rc = sysinst_setup_sysvol();
	if (rc != EOK)
		return rc;

	printf("Directories created. Copying boot files.\n");
	rc = sysinst_copy_boot_files();
	if (rc != EOK)
		return rc;

	printf("Boot files done. Configuring the system.\n");
	rc = sysinst_customize_initrd();
	if (rc != EOK)
		return rc;

	printf("Boot files done. Installing boot blocks.\n");
	rc = sysinst_copy_boot_blocks(dev);
	if (rc != EOK)
		return rc;

	printf("Ejecting device.\n");
	rc = sysinst_eject_dev(psvc_id);
	if (rc != EOK)
		return rc;

	return EOK;
}

int main(int argc, char *argv[])
{
	unsigned i;
	errno_t rc;

	i = 0;
	while (default_devs[i] != NULL) {
		rc = sysinst_check_dev(default_devs[i]);
		if (rc == EOK)
			break;
	}

	if (default_devs[i] == NULL) {
		printf("Cannot determine installation device.\n");
		return 1;
	}

	return sysinst_install(default_devs[i]);
}

/** @}
 */
