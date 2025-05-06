/*
 * Copyright (c) 2025 Jiri Svoboda
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
#include <gfx/render.h>
#include <io/log.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <system.h>
#include <ui/msgdialog.h>
#include <ui/ui.h>
#include <ui/window.h>
#include <vfs/vfs.h>
#include <vol.h>

#include "grub.h"
#include "rdimg.h"
#include "volume.h"
#include "sysinst.h"

#define NAME "sysinst"

/** Device to install to
 *
 * Note that you cannot simply change this, because the installation
 * device is hardcoded in core.img. If you wanted to install to another
 * device, you must build your own core.img (e.g. using tools/grub/mkimage.sh
 * and modifying tools/grub/load.cfg, supplying the device to boot from
 * in Grub notation).
 */
#define DEFAULT_DEV_0 "devices/\\hw\\sys\\00:01.1\\c0d0"
#define DEFAULT_DEV_1 "devices/\\hw\\sys\\ide1\\c0d0"
#define DEFAULT_DEV_2 "devices/\\hw\\sys\\00:01.0\\ide1\\c0d0"
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
	DEFAULT_DEV_2,
	NULL
};

static const char *sys_dirs[] = {
	"/cfg",
	"/data",
	NULL
};

static fibril_mutex_t shutdown_lock;
static fibril_condvar_t shutdown_cv;
static bool shutdown_stopped;
static bool shutdown_failed;

static void sysinst_shutdown_complete(void *);
static void sysinst_shutdown_failed(void *);

static system_cb_t sysinst_system_cb = {
	.shutdown_complete = sysinst_shutdown_complete,
	.shutdown_failed = sysinst_shutdown_failed
};

static void wnd_close(ui_window_t *, void *);
static errno_t bg_wnd_paint(ui_window_t *, void *);

static ui_window_cb_t bg_window_cb = {
	.close = wnd_close,
	.paint = bg_wnd_paint
};

static ui_window_cb_t progress_window_cb = {
	.close = wnd_close
};

static void sysinst_confirm_button(ui_msg_dialog_t *, void *, unsigned);
static void sysinst_confirm_close(ui_msg_dialog_t *, void *);

static ui_msg_dialog_cb_t sysinst_confirm_cb = {
	.button = sysinst_confirm_button,
	.close = sysinst_confirm_close
};

static errno_t sysinst_restart_dlg_create(sysinst_t *);
static void sysinst_restart_dlg_button(ui_msg_dialog_t *, void *, unsigned);
static void sysinst_restart_dlg_close(ui_msg_dialog_t *, void *);

static ui_msg_dialog_cb_t sysinst_restart_dlg_cb = {
	.button = sysinst_restart_dlg_button,
	.close = sysinst_restart_dlg_close
};

static int sysinst_start(sysinst_t *);
static errno_t sysinst_restart(sysinst_t *);
static void sysinst_progress_destroy(sysinst_progress_t *);
static void sysinst_action(sysinst_t *, const char *);
static void sysinst_error(sysinst_t *, const char *);
static void sysinst_debug(sysinst_t *, const char *);

static void sysinst_futil_copy_file(void *, const char *, const char *);
static void sysinst_futil_create_dir(void *, const char *);
static errno_t sysinst_eject_dev(sysinst_t *, service_id_t);
static errno_t sysinst_eject_phys_by_mp(sysinst_t *, const char *);

static futil_cb_t sysinst_futil_cb = {
	.copy_file = sysinst_futil_copy_file,
	.create_dir = sysinst_futil_create_dir
};

static void sysinst_error_msg_button(ui_msg_dialog_t *, void *, unsigned);
static void sysinst_error_msg_close(ui_msg_dialog_t *, void *);

static ui_msg_dialog_cb_t sysinst_error_msg_cb = {
	.button = sysinst_error_msg_button,
	.close = sysinst_error_msg_close
};

/** Close window.
 *
 * @param window Window
 * @param arg Argument (sysinst_t *)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	(void)window;
	(void)arg;
}

/** Paint background window.
 *
 * @param window Window
 * @param arg Argument (sysinst_t *)
 */
static errno_t bg_wnd_paint(ui_window_t *window, void *arg)
{
	sysinst_t *sysinst = (sysinst_t *)arg;
	gfx_rect_t app_rect;
	gfx_context_t *gc;
	errno_t rc;

	gc = ui_window_get_gc(window);

	rc = gfx_set_color(gc, sysinst->bg_color);
	if (rc != EOK)
		return rc;

	ui_window_get_app_rect(window, &app_rect);

	rc = gfx_fill_rect(gc, &app_rect);
	if (rc != EOK)
		return rc;

	rc = gfx_update(gc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Installation confirm dialog OK button press.
 *
 * @param dialog Message dialog
 * @param arg Argument (sysinst_t *)
 * @param btn Button number
 * @param earg Entry argument
 */
static void sysinst_confirm_button(ui_msg_dialog_t *dialog, void *arg,
    unsigned btn)
{
	sysinst_t *sysinst = (sysinst_t *) arg;

	ui_msg_dialog_destroy(dialog);

	switch (btn) {
	case 0:
		/* OK */
		sysinst_start(sysinst);
		break;
	default:
		/* Cancel */
		ui_quit(sysinst->ui);
		break;
	}
}

/** Installation confirm dialog close request.
 *
 * @param dialog Message dialog
 * @param arg Argument (sysinst_t *)
 */
static void sysinst_confirm_close(ui_msg_dialog_t *dialog, void *arg)
{
	sysinst_t *sysinst = (sysinst_t *) arg;

	ui_msg_dialog_destroy(dialog);
	ui_quit(sysinst->ui);
}

/** Restart system dialog OK button press.
 *
 * @param dialog Message dialog
 * @param arg Argument (sysinst_t *)
 * @param btn Button number
 * @param earg Entry argument
 */
static void sysinst_restart_dlg_button(ui_msg_dialog_t *dialog, void *arg,
    unsigned btn)
{
	sysinst_t *sysinst = (sysinst_t *) arg;

	ui_msg_dialog_destroy(dialog);

	(void)sysinst;

	switch (btn) {
	case 0:
		/* OK */
		sysinst_action(sysinst, "Ejecting installation media.");
		(void)sysinst_eject_phys_by_mp(sysinst, CD_MOUNT_POINT);
		(void)sysinst_restart(sysinst);
		break;
	default:
		/* Cancel */
		ui_quit(sysinst->ui);
		break;
	}
}

/** Restat system dialog close request.
 *
 * @param dialog Message dialog
 * @param arg Argument (sysinst_t *)
 */
static void sysinst_restart_dlg_close(ui_msg_dialog_t *dialog, void *arg)
{
	sysinst_t *sysinst = (sysinst_t *) arg;

	ui_msg_dialog_destroy(dialog);
	ui_quit(sysinst->ui);
}

/** Installation error message dialog button press.
 *
 * @param dialog Message dialog
 * @param arg Argument (sysinst_t *)
 * @param bnum Button number
 */
static void sysinst_error_msg_button(ui_msg_dialog_t *dialog,
    void *arg, unsigned bnum)
{
	sysinst_t *sysinst = (sysinst_t *) arg;

	ui_msg_dialog_destroy(dialog);
	ui_quit(sysinst->ui);
}

/** Installation error message dialog close request.
 *
 * @param dialog Message dialog
 * @param arg Argument (shutdown_dlg_t *)
 */
static void sysinst_error_msg_close(ui_msg_dialog_t *dialog, void *arg)
{
	sysinst_t *sysinst = (sysinst_t *) arg;

	ui_msg_dialog_destroy(dialog);
	ui_quit(sysinst->ui);
}

/** Create error message dialog.
 *
 * @param sysinst System installer
 * @return EOK on success or an error code
 */
static errno_t sysinst_error_msg_create(sysinst_t *sysinst)
{
	ui_msg_dialog_params_t params;
	ui_msg_dialog_t *dialog;
	errno_t rc;

	ui_msg_dialog_params_init(&params);
	params.caption = "Error";
	params.text = sysinst->errmsg;
	params.flags |= umdf_topmost | umdf_center;

	rc = ui_msg_dialog_create(sysinst->ui, &params, &dialog);
	if (rc != EOK)
		return rc;

	ui_msg_dialog_set_cb(dialog, &sysinst_error_msg_cb, (void *)sysinst);

	return EOK;
}

/** Called when futil is starting to copy a file.
 *
 * @param arg Argument (sysinst_t *)
 * @param src Source path
 * @param dest Destination path
 */
static void sysinst_futil_copy_file(void *arg, const char *src,
    const char *dest)
{
	sysinst_t *sysinst = (sysinst_t *)arg;
	char buf[128];

	(void)src;
	snprintf(buf, sizeof(buf), "Copying %s.", dest);
	sysinst_action(sysinst, buf);
}

/** Called when futil is about to create a directory.
 *
 * @param arg Argument (sysinst_t *)
 * @param dest Destination path
 */
static void sysinst_futil_create_dir(void *arg, const char *dest)
{
	sysinst_t *sysinst = (sysinst_t *)arg;
	char buf[128];

	snprintf(buf, sizeof(buf), "Creating %s.", dest);
	sysinst_action(sysinst, buf);
}

/** System shutdown complete.
 *
 * @param arg Argument (shutdown_t *)
 */
static void sysinst_shutdown_complete(void *arg)
{
	(void)arg;

	fibril_mutex_lock(&shutdown_lock);
	shutdown_stopped = true;
	shutdown_failed = false;
	fibril_condvar_broadcast(&shutdown_cv);
	fibril_mutex_unlock(&shutdown_lock);
}

/** System shutdown failed.
 *
 * @param arg Argument (not used)
 */
static void sysinst_shutdown_failed(void *arg)
{
	(void)arg;

	fibril_mutex_lock(&shutdown_lock);
	shutdown_stopped = true;
	shutdown_failed = true;
	fibril_condvar_broadcast(&shutdown_cv);
	fibril_mutex_unlock(&shutdown_lock);
}

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

/** Label and mount the destination device.
 *
 * @param sysinst System installer
 * @param dev Disk device to label
 * @param psvc_id Place to store service ID of the created partition
 *
 * @return EOK on success or an error code
 */
static errno_t sysinst_label_dev(sysinst_t *sysinst, const char *dev)
{
	fdisk_t *fdisk = NULL;
	fdisk_dev_t *fdev = NULL;
	fdisk_part_t *part;
	fdisk_part_spec_t pspec;
	fdisk_part_info_t pinfo;
	bool dir_created = false;
	bool label_created = false;
	capa_spec_t capa;
	service_id_t sid;
	errno_t rc;

	sysinst_debug(sysinst, "sysinst_label_dev(): get service ID");

	rc = loc_service_get_id(dev, &sid, 0);
	if (rc != EOK)
		goto error;

	sysinst_debug(sysinst, "sysinst_label_dev(): open device");

	rc = fdisk_create(&fdisk);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error initializing fdisk.");
		goto error;
	}

	rc = fdisk_dev_open(fdisk, sid, &fdev);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error opening device.");
		goto error;
	}

	sysinst_debug(sysinst, "sysinst_label_dev(): create mount directory");

	rc = vfs_link_path(MOUNT_POINT, KIND_DIRECTORY, NULL);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error creating mount directory.");
		goto error;
	}

	dir_created = true;

	sysinst_debug(sysinst, "sysinst_label_dev(): create label");

	rc = fdisk_label_create(fdev, lt_mbr);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error creating label.");
		goto error;
	}

	label_created = true;

	sysinst_debug(sysinst, "sysinst_label_dev(): create partition");

	rc = fdisk_part_get_max_avail(fdev, spc_pri, &capa);
	if (rc != EOK) {
		sysinst_error(sysinst,
		    "Error getting available capacity.");
		goto error;
	}

	fdisk_pspec_init(&pspec);
	pspec.capacity = capa;
	pspec.pkind = lpk_primary;
	pspec.fstype = fs_ext4; /* Cannot be changed without modifying core.img */
	pspec.mountp = MOUNT_POINT;
	pspec.label = INST_VOL_LABEL;

	rc = fdisk_part_create(fdev, &pspec, &part);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error creating partition.");
		goto error;
	}

	rc = fdisk_part_get_info(part, &pinfo);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error getting partition information.");
		goto error;
	}

	sysinst_debug(sysinst, "sysinst_label_dev(): OK");
	fdisk_dev_close(fdev);
	fdisk_destroy(fdisk);
	sysinst->psvc_id = pinfo.svc_id;
	return EOK;
error:
	if (label_created)
		fdisk_label_destroy(fdev);
	if (dir_created)
		(void)vfs_unlink_path(MOUNT_POINT);
	if (fdev != NULL)
		fdisk_dev_close(fdev);
	if (fdisk != NULL)
		fdisk_destroy(fdisk);
	return rc;
}

/** Finish/unmount destination device.
 *
 * @param sysinst System installer
 *
 * @return EOK on success or an error code
 */
static errno_t sysinst_finish_dev(sysinst_t *sysinst)
{
	errno_t rc;

	sysinst_debug(sysinst, "sysinst_finish_dev(): eject target volume");
	rc = sysinst_eject_dev(sysinst, sysinst->psvc_id);
	if (rc != EOK)
		return rc;

	sysinst_debug(sysinst, "sysinst_finish_dev(): "
	    "deleting mount directory");
	(void)vfs_unlink_path(MOUNT_POINT);

	return EOK;
}

/** Set up system volume structure.
 *
 * @param sysinst System installer
 * @return EOK on success or an error code
 */
static errno_t sysinst_setup_sysvol(sysinst_t *sysinst)
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
			sysinst_error(sysinst, "Error creating directory.");
			goto error;
		}

		free(path);
		path = NULL;
		++cp;
	}

	free(path);
	path = NULL;

	/* Copy initial configuration files */
	rc = futil_rcopy_contents(sysinst->futil, CFG_FILES_SRC,
	    CFG_FILES_DEST);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error copying initial configuration "
		    "files.");
		return rc;
	}

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
static errno_t sysinst_copy_boot_files(sysinst_t *sysinst)
{
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "sysinst_copy_boot_files(): copy bootloader files");
	rc = futil_rcopy_contents(sysinst->futil, BOOT_FILES_SRC, MOUNT_POINT);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error copying bootloader "
		    "files.");
		return rc;
	}

	sysinst_debug(sysinst, "sysinst_copy_boot_files(): OK");
	return EOK;
}

/** Set up configuration in the initial RAM disk.
 *
 * @param sysinst System installer
 * @return EOK on success or an error code
 */
static errno_t sysinst_customize_initrd(sysinst_t *sysinst)
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
		sysinst_error(sysinst, "Error opening initial RAM disk image.");
		goto error;
	}

	rv = asprintf(&path, "%s%s", rdpath, "/cfg/initvol.sif");
	if (rv < 0) {
		rc = ENOMEM;
		goto error;
	}

	sysinst_debug(sysinst, "Configuring volume server.");

	rc = vol_volumes_create(path, &volumes);
	if (rc != EOK) {
		sysinst_error(sysinst,
		    "Error creating volume server configuration.");
		rc = EIO;
		goto error;
	}

	sysinst_debug(sysinst, "Configuring volume server: look up volume");
	rc = vol_volume_lookup_ref(volumes, INST_VOL_LABEL, &volume);
	if (rc != EOK) {
		sysinst_error(sysinst,
		    "Error creating volume server configuration.");
		rc = EIO;
		goto error;
	}

	sysinst_debug(sysinst, "Configuring volume server: set mount point");
	rc = vol_volume_set_mountp(volume, INST_VOL_MP);
	if (rc != EOK) {
		sysinst_error(sysinst,
		    "Error creating system partition configuration.");
		rc = EIO;
		goto error;
	}

	rc = vol_volumes_sync(volumes);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error saving volume confiuration.");
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "Configuring volume server: delete reference");
	vol_volume_del_ref(volume);
	volume = NULL;
	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "Configuring volume server: destroy volumes object");
	vol_volumes_destroy(volumes);
	volumes = NULL;

	rc = rd_img_close(rd);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error closing initial RAM disk image.");
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
 * @param sysinst System installer
 * @param devp Disk device
 * @return EOK on success or an error code
 */
static errno_t sysinst_copy_boot_blocks(sysinst_t *sysinst, const char *devp)
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

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "sysinst_copy_boot_blocks: Read boot block image.");

	rc = futil_get_file(sysinst->futil,
	    BOOT_FILES_SRC "/boot/grub/i386-pc/boot.img",
	    &boot_img, &boot_img_size);
	if (rc != EOK || boot_img_size != 512)
		return EIO;

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "sysinst_copy_boot_blocks: Read GRUB core image.");

	rc = futil_get_file(sysinst->futil,
	    BOOT_FILES_SRC "/boot/grub/i386-pc/core.img",
	    &core_img, &core_img_size);
	if (rc != EOK)
		return EIO;

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "sysinst_copy_boot_blocks: get service ID.");

	rc = loc_service_get_id(devp, &sid, 0);
	if (rc != EOK)
		return rc;

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "sysinst_copy_boot_blocks: block_init.");

	rc = block_init(sid);
	if (rc != EOK)
		return rc;

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "sysinst_copy_boot_blocks: get block size");

	rc = block_get_bsize(sid, &bsize);
	if (rc != EOK)
		return rc;

	if (bsize != 512) {
		sysinst_error(sysinst, "Device block size != 512.");
		return EIO;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "sysinst_copy_boot_blocks: read boot block");

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
			sysinst_error(sysinst,
			    "No block terminator in core image.");
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

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "sysinst_copy_boot_blocks: write boot block");

	rc = block_write_direct(sid, BOOT_BLOCK_IDX, 1, bbuf);
	if (rc != EOK)
		return EIO;

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "sysinst_copy_boot_blocks: write core blocks");

	/* XXX Must pad last block with zeros */
	rc = block_write_direct(sid, core_start, core_blocks, core_img);
	if (rc != EOK)
		return EIO;

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "sysinst_copy_boot_blocks: OK.");

	return EOK;
}

/** Eject installation volume.
 *
 * @param sysinst System installer
 * @param part_id Partition service ID
 * @return EOK on success or an error code
 */
static errno_t sysinst_eject_dev(sysinst_t *sysinst, service_id_t part_id)
{
	vol_t *vol = NULL;
	errno_t rc;

	rc = vol_create(&vol);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error contacting volume service.");
		goto out;
	}

	rc = vol_part_eject(vol, part_id, vef_none);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error ejecting volume.");
		goto out;
	}

	rc = EOK;
out:
	vol_destroy(vol);
	return rc;
}

/** Physically eject volume by mount point.
 *
 * @param sysinst System installer
 * @param path Mount point
 * @return EOK on success or an error code
 */
static errno_t sysinst_eject_phys_by_mp(sysinst_t *sysinst, const char *path)
{
	vol_t *vol = NULL;
	sysarg_t part_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "sysinst_eject_phys_by_mp(%s)", path);

	rc = vol_create(&vol);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error contacting volume service.");
		goto out;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "vol_part_by_mp: mp='%s'\n",
	    path);
	rc = vol_part_by_mp(vol, path, &part_id);
	if (rc != EOK) {
		sysinst_error(sysinst,
		    "Error finding installation media mount point.");
		goto out;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "eject svc_id %lu", (unsigned long)part_id);
	rc = vol_part_eject(vol, part_id, vef_physical);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error ejecting volume.");
		goto out;
	}

	rc = EOK;
out:
	vol_destroy(vol);
	return rc;
}

/** Restart the system.
 *
 * @param sysinst System installer
 * @return EOK on success or an error code
 */
static errno_t sysinst_restart(sysinst_t *sysinst)
{
	errno_t rc;
	system_t *system;

	fibril_mutex_initialize(&shutdown_lock);
	fibril_condvar_initialize(&shutdown_cv);
	shutdown_stopped = false;
	shutdown_failed = false;

	sysinst_action(sysinst, "Restarting the system.");

	rc = system_open(SYSTEM_DEFAULT, &sysinst_system_cb, NULL, &system);
	if (rc != EOK) {
		sysinst_error(sysinst,
		    "Failed opening system control service.");
		return rc;
	}

	rc = system_restart(system);
	if (rc != EOK) {
		system_close(system);
		sysinst_error(sysinst, "Failed requesting system restart.");
		return rc;
	}

	fibril_mutex_lock(&shutdown_lock);
	sysinst_debug(sysinst, "The system is shutting down...");

	while (!shutdown_stopped)
		fibril_condvar_wait(&shutdown_cv, &shutdown_lock);

	if (shutdown_failed) {
		sysinst_error(sysinst, "Shutdown failed.");
		system_close(system);
		return rc;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE,
	    "Shutdown complete. It is now safe to remove power.");

	/* Sleep forever */
	while (true)
		fibril_condvar_wait(&shutdown_cv, &shutdown_lock);

	fibril_mutex_unlock(&shutdown_lock);

	system_close(system);
	return 0;

}

/** Install system to a device.
 *
 * @parma sysinst System installer
 * @param dev Device to install to.
 * @return EOK on success or an error code
 */
static errno_t sysinst_install(sysinst_t *sysinst, const char *dev)
{
	errno_t rc;
	bool clean_dev = false;

	sysinst_action(sysinst, "Creating device label and file system.");

	rc = sysinst_label_dev(sysinst, dev);
	if (rc != EOK)
		goto error;

	clean_dev = true;

	sysinst_action(sysinst, "Creating system directory structure.");
	rc = sysinst_setup_sysvol(sysinst);
	if (rc != EOK)
		goto error;

	sysinst_action(sysinst, "Copying boot files.");
	rc = sysinst_copy_boot_files(sysinst);
	if (rc != EOK)
		goto error;

	sysinst_action(sysinst, "Configuring the system.");
	rc = sysinst_customize_initrd(sysinst);
	if (rc != EOK)
		goto error;

	sysinst_action(sysinst, "Finishing system volume.");
	rc = sysinst_finish_dev(sysinst);
	if (rc != EOK)
		goto error;

	clean_dev = false;

	sysinst_action(sysinst, "Installing boot blocks.");
	rc = sysinst_copy_boot_blocks(sysinst, dev);
	if (rc != EOK)
		return rc;

	return EOK;
error:
	if (clean_dev)
		(void)sysinst_finish_dev(sysinst);
	return rc;
}

/** Installation fibril.
 *
 * @param arg Argument (sysinst_t *)
 * @return EOK on success or an error code
 */
static errno_t sysinst_install_fibril(void *arg)
{
	sysinst_t *sysinst = (sysinst_t *)arg;
	unsigned i;
	errno_t rc;

	(void)sysinst;

	i = 0;
	while (default_devs[i] != NULL) {
		rc = sysinst_check_dev(default_devs[i]);
		if (rc == EOK)
			break;
		++i;
	}

	if (default_devs[i] == NULL) {
		sysinst_error(sysinst, "Cannot determine installation device.");
		rc = ENOENT;
		goto error;
	}

	rc = sysinst_install(sysinst, default_devs[i]);
	if (rc != EOK)
		goto error;

	sysinst_progress_destroy(sysinst->progress);
	sysinst->progress = NULL;

	rc = sysinst_restart_dlg_create(sysinst);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	ui_lock(sysinst->ui);
	sysinst_progress_destroy(sysinst->progress);
	(void)sysinst_error_msg_create(sysinst);
	ui_unlock(sysinst->ui);
	return rc;
}

/** Create installation progress window.
 *
 * @param sysinst System installer
 * @param rprogress Place to store pointer to new progress window
 * @return EOK on success or an error code
 */
static errno_t sysinst_progress_create(sysinst_t *sysinst,
    sysinst_progress_t **rprogress)
{
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	gfx_rect_t rect;
	gfx_rect_t arect;
	ui_resource_t *ui_res;
	sysinst_progress_t *progress;
	ui_fixed_t *fixed = NULL;
	errno_t rc;

	ui_wnd_params_init(&params);
	params.caption = "System Installation";
	params.style &= ~ui_wds_titlebar;
	params.flags |= ui_wndf_topmost;
	params.placement = ui_wnd_place_center;
	if (ui_is_textmode(sysinst->ui)) {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 64;
		params.rect.p1.y = 5;
	} else {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 500;
		params.rect.p1.y = 60;
	}

	progress = calloc(1, sizeof(sysinst_progress_t));
	if (progress == NULL) {
		rc = ENOMEM;
		sysinst_error(sysinst, "Out of memory.");
		goto error;
	}

	rc = ui_window_create(sysinst->ui, &params, &window);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error creating window.");
		goto error;
	}

	ui_window_set_cb(window, &progress_window_cb, (void *)sysinst);

	ui_res = ui_window_get_res(window);

	rc = ui_fixed_create(&fixed);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error creating fixed layout.");
		goto error;
	}

	rc = ui_label_create(ui_res, "Installing system. Please wait...",
	    &progress->label);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error creating label.");
		goto error;
	}

	ui_window_get_app_rect(window, &arect);

	if (ui_is_textmode(sysinst->ui)) {
		rect.p0.x = arect.p0.x;
		rect.p0.y = arect.p0.y;
		rect.p1.x = arect.p1.x;
		rect.p1.y = 2;
	} else {
		rect.p0.x = arect.p0.x;
		rect.p0.y = arect.p0.y;
		rect.p1.x = arect.p1.x;
		rect.p1.y = 30;
	}
	ui_label_set_rect(progress->label, &rect);
	ui_label_set_halign(progress->label, gfx_halign_center);
	ui_label_set_valign(progress->label, gfx_valign_center);

	rc = ui_fixed_add(fixed, ui_label_ctl(progress->label));
	if (rc != EOK) {
		sysinst_error(sysinst, "Error adding control to layout.");
		ui_label_destroy(progress->label);
		progress->label = NULL;
		goto error;
	}

	rc = ui_label_create(ui_res, "",
	    &progress->action);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error creating label.");
		goto error;
	}

	if (ui_is_textmode(sysinst->ui)) {
		rect.p0.x = arect.p0.x;
		rect.p0.y = 3;
		rect.p1.x = arect.p1.x;
		rect.p1.y = arect.p1.y;
	} else {
		rect.p0.x = arect.p0.x;
		rect.p0.y = 30;
		rect.p1.x = arect.p1.x;
		rect.p1.y = arect.p1.y;
	}
	ui_label_set_rect(progress->action, &rect);
	ui_label_set_halign(progress->action, gfx_halign_center);
	ui_label_set_valign(progress->action, gfx_valign_center);

	rc = ui_fixed_add(fixed, ui_label_ctl(progress->action));
	if (rc != EOK) {
		sysinst_error(sysinst, "Error adding control to layout.");
		ui_label_destroy(progress->label);
		progress->label = NULL;
		goto error;
	}

	ui_window_add(window, ui_fixed_ctl(fixed));
	fixed = NULL;

	rc = ui_window_paint(window);
	if (rc != EOK) {
		sysinst_error(sysinst, "Error painting window.");
		goto error;
	}

	progress->window = window;
	progress->fixed = fixed;
	*rprogress = progress;
	return EOK;
error:
	if (progress != NULL && progress->fixed != NULL)
		ui_fixed_destroy(progress->fixed);
	if (window != NULL)
		ui_window_destroy(window);
	if (progress != NULL)
		free(progress);
	return rc;
}

/** Destroy installation progress window.
 *
 * @param sysinst System installer
 * @param rprogress Place to store pointer to new progress window
 * @return EOK on success or an error code
 */
static void sysinst_progress_destroy(sysinst_progress_t *progress)
{
	if (progress == NULL)
		return;

	ui_window_destroy(progress->window);
	free(progress);
}

/** Set current action message.
 *
 * @param sysinst System installer
 * @param action Action text
 */
static void sysinst_action(sysinst_t *sysinst, const char *action)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "%s", action);

	if (sysinst->progress == NULL)
		return;

	ui_label_set_text(sysinst->progress->action, action);
	ui_label_paint(sysinst->progress->action);
}

/** Set current error message.
 *
 * @param sysinst System installer
 * @param errmsg Error message
 */
static void sysinst_error(sysinst_t *sysinst, const char *errmsg)
{
	str_cpy(sysinst->errmsg, sizeof(sysinst->errmsg), errmsg);
	log_msg(LOG_DEFAULT, LVL_ERROR, errmsg);
}

/** Log a debug message.
 *
 * @param sysinst System installer
 * @param errmsg Error message
 */
static void sysinst_debug(sysinst_t *sysinst, const char *msg)
{
	log_msg(LOG_DEFAULT, LVL_ERROR, msg);
}

/** Start system installation.
 *
 * @param sysinst System installer
 * @return EOK on success or an error code
 */
static int sysinst_start(sysinst_t *sysinst)
{
	errno_t rc;
	fid_t fid;

	rc = sysinst_progress_create(sysinst, &sysinst->progress);
	if (rc != EOK)
		return rc;

	fid = fibril_create(sysinst_install_fibril, (void *)sysinst);
	if (fid == 0) {
		sysinst_error(sysinst, "Out of memory.");
		return ENOMEM;
	}

	fibril_add_ready(fid);
	return EOK;
}

/** Create installation confirmation dialog.
 *
 * @param sysinst System installer
 * @return EOK on success or an error code
 */
static errno_t sysinst_confirm_create(sysinst_t *sysinst)
{
	ui_msg_dialog_params_t params;
	ui_msg_dialog_t *dialog;
	errno_t rc;

	ui_msg_dialog_params_init(&params);
	params.caption = "System installation";
	params.text = "This will install HelenOS to your computer. Continue?";
	params.choice = umdc_ok_cancel;
	params.flags |= umdf_topmost | umdf_center;

	rc = ui_msg_dialog_create(sysinst->ui, &params, &dialog);
	if (rc != EOK)
		return rc;

	ui_msg_dialog_set_cb(dialog, &sysinst_confirm_cb, sysinst);
	return EOK;
}

/** Create restart dialog.
 *
 * @param sysinst System installer
 * @return EOK on success or an error code
 */
static errno_t sysinst_restart_dlg_create(sysinst_t *sysinst)
{
	ui_msg_dialog_params_t params;
	ui_msg_dialog_t *dialog;
	errno_t rc;

	ui_msg_dialog_params_init(&params);
	params.caption = "Restart System";
	params.text = "Installation complete. Restart the system?";
	params.choice = umdc_ok_cancel;
	params.flags |= umdf_topmost | umdf_center;

	rc = ui_msg_dialog_create(sysinst->ui, &params, &dialog);
	if (rc != EOK)
		return rc;

	ui_msg_dialog_set_cb(dialog, &sysinst_restart_dlg_cb, sysinst);
	return EOK;
}

/** Run system installer on display.
 *
 * @param display_spec Display specification
 * @return EOK on success or an error code
 */
static errno_t sysinst_run(const char *display_spec)
{
	ui_t *ui = NULL;
	sysinst_t *sysinst;
	ui_wnd_params_t params;
	errno_t rc;

	sysinst = calloc(1, sizeof(sysinst_t));
	if (sysinst == NULL)
		return ENOMEM;

	rc = futil_create(&sysinst_futil_cb, (void *)sysinst, &sysinst->futil);
	if (rc != EOK) {
		printf("Out of memory.\n");
		goto error;
	}

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		goto error;
	}

	sysinst->ui = ui;

	ui_wnd_params_init(&params);
	params.caption = "System Installation";
	params.style &= ~ui_wds_decorated;
	params.placement = ui_wnd_place_full_screen;
	params.flags |= ui_wndf_topmost | ui_wndf_nofocus;

	rc = ui_window_create(sysinst->ui, &params, &sysinst->bgwindow);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(sysinst->bgwindow, &bg_window_cb, (void *)sysinst);

	if (ui_is_textmode(sysinst->ui)) {
		rc = gfx_color_new_ega(0x17, &sysinst->bg_color);
		if (rc != EOK) {
			printf("Error allocating color.\n");
			goto error;
		}
	} else {
		rc = gfx_color_new_rgb_i16(0x8000, 0xc800, 0xffff, &sysinst->bg_color);
		if (rc != EOK) {
			printf("Error allocating color.\n");
			goto error;
		}
	}

	rc = ui_window_paint(sysinst->bgwindow);
	if (rc != EOK) {
		printf("Error painting window.\n");
		goto error;
	}

	(void)sysinst_confirm_create(sysinst);

	ui_run(ui);

	if (sysinst->bgwindow != NULL)
		ui_window_destroy(sysinst->bgwindow);
	if (sysinst->system != NULL)
		system_close(sysinst->system);
	gfx_color_delete(sysinst->bg_color);
	ui_destroy(ui);
	free(sysinst);
	return EOK;
error:
	if (sysinst->futil != NULL)
		futil_destroy(sysinst->futil);
	if (sysinst->system != NULL)
		system_close(sysinst->system);
	if (sysinst->bg_color != NULL)
		gfx_color_delete(sysinst->bg_color);
	if (sysinst->bgwindow != NULL)
		ui_window_destroy(sysinst->bgwindow);
	if (ui != NULL)
		ui_destroy(ui);
	free(sysinst);
	return rc;
}

static void print_syntax(void)
{
	printf("Syntax: " NAME " [-d <display-spec>]\n");
}

int main(int argc, char *argv[])
{
	const char *display_spec = UI_ANY_DEFAULT;
	errno_t rc;
	int i;

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		if (str_cmp(argv[i], "-d") == 0) {
			++i;
			if (i >= argc) {
				printf("Argument missing.\n");
				print_syntax();
				return 1;
			}

			display_spec = argv[i++];
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			return 1;
		}
	}

	if (i < argc) {
		print_syntax();
		return 1;
	}

	if (log_init(NAME) != EOK) {
		printf(NAME ": Failed to initialize logging.\n");
		return 1;
	}

	rc = sysinst_run(display_spec);
	if (rc != EOK)
		return 1;

	return 0;
}

/** @}
 */
