/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2011 Jiri Svoboda
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

/** @addtogroup drvusbmast
 * @{
 */
/**
 * @file
 * Main routines of USB mass storage driver.
 */

#include <as.h>
#include <async.h>
#include <bd_srv.h>
#include <macros.h>
#include <usb/dev/driver.h>
#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/classes/massstor.h>
#include <errno.h>
#include <io/logctl.h>
#include <str_error.h>
#include "cmdw.h"
#include "bo_trans.h"
#include "scsi_ms.h"
#include "usbmast.h"

#define NAME "usbmast"

static const usb_endpoint_description_t bulk_in_ep = {
	.transfer_type = USB_TRANSFER_BULK,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_MASS_STORAGE,
	.interface_subclass = USB_MASSSTOR_SUBCLASS_SCSI,
	.interface_protocol = USB_MASSSTOR_PROTOCOL_BBB,
	.flags = 0
};
static const usb_endpoint_description_t bulk_out_ep = {
	.transfer_type = USB_TRANSFER_BULK,
	.direction = USB_DIRECTION_OUT,
	.interface_class = USB_CLASS_MASS_STORAGE,
	.interface_subclass = USB_MASSSTOR_SUBCLASS_SCSI,
	.interface_protocol = USB_MASSSTOR_PROTOCOL_BBB,
	.flags = 0
};

static const usb_endpoint_description_t *mast_endpoints[] = {
	&bulk_in_ep,
	&bulk_out_ep,
	NULL
};

static errno_t usbmast_fun_create(usbmast_dev_t *mdev, unsigned lun);
static void usbmast_bd_connection(ipc_callid_t iid, ipc_call_t *icall,
    void *arg);

static errno_t usbmast_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t usbmast_bd_close(bd_srv_t *);
static errno_t usbmast_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *, size_t);
static errno_t usbmast_bd_sync_cache(bd_srv_t *, aoff64_t, size_t);
static errno_t usbmast_bd_write_blocks(bd_srv_t *, aoff64_t, size_t, const void *, size_t);
static errno_t usbmast_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t usbmast_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static bd_ops_t usbmast_bd_ops = {
	.open = usbmast_bd_open,
	.close = usbmast_bd_close,
	.read_blocks = usbmast_bd_read_blocks,
	.sync_cache = usbmast_bd_sync_cache,
	.write_blocks = usbmast_bd_write_blocks,
	.get_block_size = usbmast_bd_get_block_size,
	.get_num_blocks = usbmast_bd_get_num_blocks
};

static usbmast_fun_t *bd_srv_usbmast(bd_srv_t *bd)
{
	return (usbmast_fun_t *) bd->srvs->sarg;
}

/** Callback when a device is removed from the system.
 *
 * @param dev Representation of USB device.
 * @return Error code.
 */
static errno_t usbmast_device_gone(usb_device_t *dev)
{
	usbmast_dev_t *mdev = usb_device_data_get(dev);
	assert(mdev);

	for (size_t i = 0; i < mdev->lun_count; ++i) {
		const errno_t rc = ddf_fun_unbind(mdev->luns[i]);
		if (rc != EOK) {
			usb_log_error("Failed to unbind LUN function %zu: "
			    "%s\n", i, str_error(rc));
			return rc;
		}
		ddf_fun_destroy(mdev->luns[i]);
		mdev->luns[i] = NULL;
	}
	free(mdev->luns);
	return EOK;
}

/** Callback when a device is about to be removed.
 *
 * @param dev Representation of USB device.
 * @return Error code.
 */
static errno_t usbmast_device_remove(usb_device_t *dev)
{
	//TODO: flush buffers, or whatever.
	//TODO: remove device
	return ENOTSUP;
}

/** Callback when new device is attached and recognized as a mass storage.
 *
 * @param dev Representation of USB device.
 * @return Error code.
 */
static errno_t usbmast_device_add(usb_device_t *dev)
{
	errno_t rc;
	usbmast_dev_t *mdev = NULL;
	unsigned i;

	usb_endpoint_mapping_t *epm_in =
	    usb_device_get_mapped_ep_desc(dev, &bulk_in_ep);
	usb_endpoint_mapping_t *epm_out =
	    usb_device_get_mapped_ep_desc(dev, &bulk_out_ep);
	if (!epm_in || !epm_out || !epm_in->present || !epm_out->present) {
		usb_log_error("Required EPs were not mapped.\n");
		return ENOENT;
	}

	/* Allocate softstate */
	mdev = usb_device_data_alloc(dev, sizeof(usbmast_dev_t));
	if (mdev == NULL) {
		usb_log_error("Failed allocating softstate.\n");
		return ENOMEM;
	}

	mdev->usb_dev = dev;

	usb_log_info("Initializing mass storage `%s'.\n",
	    usb_device_get_name(dev));
	usb_log_debug("Bulk in endpoint: %d [%zuB].\n",
	    epm_in->pipe.endpoint_no, epm_in->pipe.max_packet_size);
	usb_log_debug("Bulk out endpoint: %d [%zuB].\n",
	    epm_out->pipe.endpoint_no, epm_out->pipe.max_packet_size);

	usb_log_debug("Get LUN count...\n");
	mdev->lun_count = usb_masstor_get_lun_count(mdev);
	mdev->luns = calloc(mdev->lun_count, sizeof(ddf_fun_t*));
	if (mdev->luns == NULL) {
		rc = ENOMEM;
		usb_log_error("Failed allocating luns table.\n");
		goto error;
	}

	mdev->bulk_in_pipe = &epm_in->pipe;
	mdev->bulk_out_pipe = &epm_out->pipe;
	for (i = 0; i < mdev->lun_count; i++) {
		rc = usbmast_fun_create(mdev, i);
		if (rc != EOK)
			goto error;
	}

	return EOK;
error:
	/* Destroy functions */
	for (size_t i = 0; i < mdev->lun_count; ++i) {
		if (mdev->luns[i] == NULL)
			continue;
		const errno_t rc = ddf_fun_unbind(mdev->luns[i]);
		if (rc != EOK) {
			usb_log_warning("Failed to unbind LUN function %zu: "
			    "%s.\n", i, str_error(rc));
		}
		ddf_fun_destroy(mdev->luns[i]);
	}
	free(mdev->luns);
	return rc;
}

/** Create mass storage function.
 *
 * Called once for each LUN.
 *
 * @param mdev		Mass storage device
 * @param lun		LUN
 * @return		EOK on success or an error code.
 */
static errno_t usbmast_fun_create(usbmast_dev_t *mdev, unsigned lun)
{
	errno_t rc;
	char *fun_name = NULL;
	ddf_fun_t *fun = NULL;
	usbmast_fun_t *mfun = NULL;

	if (asprintf(&fun_name, "l%u", lun) < 0) {
		usb_log_error("Out of memory.\n");
		rc = ENOMEM;
		goto error;
	}

	fun = usb_device_ddf_fun_create(mdev->usb_dev, fun_exposed, fun_name);
	if (fun == NULL) {
		usb_log_error("Failed to create DDF function %s.\n", fun_name);
		rc = ENOMEM;
		goto error;
	}

	/* Allocate soft state */
	mfun = ddf_fun_data_alloc(fun, sizeof(usbmast_fun_t));
	if (mfun == NULL) {
		usb_log_error("Failed allocating softstate.\n");
		rc = ENOMEM;
		goto error;
	}

	mfun->ddf_fun = fun;
	mfun->mdev = mdev;
	mfun->lun = lun;

	bd_srvs_init(&mfun->bds);
	mfun->bds.ops = &usbmast_bd_ops;
	mfun->bds.sarg = mfun;

	/* Set up a connection handler. */
	ddf_fun_set_conn_handler(fun, usbmast_bd_connection);

	usb_log_debug("Inquire...\n");
	usbmast_inquiry_data_t inquiry;
	rc = usbmast_inquiry(mfun, &inquiry);
	if (rc != EOK) {
		usb_log_warning("Failed to inquire device `%s': %s.\n",
		    usb_device_get_name(mdev->usb_dev), str_error(rc));
		rc = EIO;
		goto error;
	}

	usb_log_info("Mass storage `%s' LUN %u: " \
	    "%s by %s rev. %s is %s (%s).\n",
	    usb_device_get_name(mdev->usb_dev),
	    lun,
	    inquiry.product,
	    inquiry.vendor,
	    inquiry.revision,
	    usbmast_scsi_dev_type_str(inquiry.device_type),
	    inquiry.removable ? "removable" : "non-removable");

	uint32_t nblocks, block_size;

	rc = usbmast_read_capacity(mfun, &nblocks, &block_size);
	if (rc != EOK) {
		usb_log_warning("Failed to read capacity, device `%s': %s.\n",
		    usb_device_get_name(mdev->usb_dev), str_error(rc));
		rc = EIO;
		goto error;
	}

	usb_log_info("Read Capacity: nblocks=%" PRIu32 ", "
	    "block_size=%" PRIu32 "\n", nblocks, block_size);

	mfun->nblocks = nblocks;
	mfun->block_size = block_size;

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		usb_log_error("Failed to bind DDF function %s: %s.\n",
		    fun_name, str_error(rc));
		goto error;
	}

	ddf_fun_add_to_category(fun, "disk");

	free(fun_name);
	mdev->luns[lun] = fun;

	return EOK;

	/* Error cleanup */
error:
	if (fun != NULL)
		ddf_fun_destroy(fun);
	if (fun_name != NULL)
		free(fun_name);
	return rc;
}

/** Blockdev client connection handler. */
static void usbmast_bd_connection(ipc_callid_t iid, ipc_call_t *icall,
    void *arg)
{
	usbmast_fun_t *mfun;

	mfun = (usbmast_fun_t *) ddf_fun_data_get((ddf_fun_t *)arg);
	bd_conn(iid, icall, &mfun->bds);
}

/** Open device. */
static errno_t usbmast_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	return EOK;
}

/** Close device. */
static errno_t usbmast_bd_close(bd_srv_t *bd)
{
	return EOK;
}

/** Read blocks from the device. */
static errno_t usbmast_bd_read_blocks(bd_srv_t *bd, uint64_t ba, size_t cnt, void *buf,
    size_t size)
{
	usbmast_fun_t *mfun = bd_srv_usbmast(bd);

	if (size < cnt * mfun->block_size)
		return EINVAL;

	return usbmast_read(mfun, ba, cnt, buf);
}

/** Synchronize blocks to nonvolatile storage. */
static errno_t usbmast_bd_sync_cache(bd_srv_t *bd, uint64_t ba, size_t cnt)
{
	usbmast_fun_t *mfun = bd_srv_usbmast(bd);

	return usbmast_sync_cache(mfun, ba, cnt);
}

/** Write blocks to the device. */
static errno_t usbmast_bd_write_blocks(bd_srv_t *bd, uint64_t ba, size_t cnt,
    const void *buf, size_t size)
{
	usbmast_fun_t *mfun = bd_srv_usbmast(bd);

	if (size < cnt * mfun->block_size)
		return EINVAL;

	return usbmast_write(mfun, ba, cnt, buf);
}

/** Get device block size. */
static errno_t usbmast_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	usbmast_fun_t *mfun = bd_srv_usbmast(bd);
	*rsize = mfun->block_size;
	return EOK;
}

/** Get number of blocks on device. */
static errno_t usbmast_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	usbmast_fun_t *mfun = bd_srv_usbmast(bd);
	*rnb = mfun->nblocks;
	return EOK;
}


/** USB mass storage driver ops. */
static const usb_driver_ops_t usbmast_driver_ops = {
	.device_add = usbmast_device_add,
	.device_rem = usbmast_device_remove,
	.device_gone = usbmast_device_gone,
};

/** USB mass storage driver. */
static const usb_driver_t usbmast_driver = {
	.name = NAME,
	.ops = &usbmast_driver_ops,
	.endpoints = mast_endpoints
};

int main(int argc, char *argv[])
{
	log_init(NAME);
	logctl_set_log_level(NAME, LVL_NOTE);
	return usb_driver_main(&usbmast_driver);
}

/**
 * @}
 */
