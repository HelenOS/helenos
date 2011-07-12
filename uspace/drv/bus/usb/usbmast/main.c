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
#include <ipc/bd.h>
#include <macros.h>
#include <usb/dev/driver.h>
#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/classes/massstor.h>
#include <errno.h>
#include <str_error.h>
#include "cmds.h"
#include "mast.h"
#include "scsi_ms.h"

#define NAME "usbmast"

#define GET_BULK_IN(dev) ((dev)->pipes[BULK_IN_EP].pipe)
#define GET_BULK_OUT(dev) ((dev)->pipes[BULK_OUT_EP].pipe)

static usb_endpoint_description_t bulk_in_ep = {
	.transfer_type = USB_TRANSFER_BULK,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_MASS_STORAGE,
	.interface_subclass = USB_MASSSTOR_SUBCLASS_SCSI,
	.interface_protocol = USB_MASSSTOR_PROTOCOL_BBB,
	.flags = 0
};
static usb_endpoint_description_t bulk_out_ep = {
	.transfer_type = USB_TRANSFER_BULK,
	.direction = USB_DIRECTION_OUT,
	.interface_class = USB_CLASS_MASS_STORAGE,
	.interface_subclass = USB_MASSSTOR_SUBCLASS_SCSI,
	.interface_protocol = USB_MASSSTOR_PROTOCOL_BBB,
	.flags = 0
};

usb_endpoint_description_t *mast_endpoints[] = {
	&bulk_in_ep,
	&bulk_out_ep,
	NULL
};

/** Mass storage function.
 *
 * Serves as soft state for function/LUN.
 */
typedef struct {
	/** DDF function */
	ddf_fun_t *ddf_fun;
	/** Total number of blocks. */
	uint64_t nblocks;
	/** Block size in bytes. */
	size_t block_size;
	/** USB device function belongs to */
	usb_device_t *usb_dev;
} usbmast_fun_t;

static void usbmast_bd_connection(ipc_callid_t iid, ipc_call_t *icall,
    void *arg);

/** Callback when new device is attached and recognized as a mass storage.
 *
 * @param dev Representation of a the USB device.
 * @return Error code.
 */
static int usbmast_add_device(usb_device_t *dev)
{
	int rc;
	const char *fun_name = "a";
	ddf_fun_t *fun = NULL;
	usbmast_fun_t *msfun = NULL;

	/* Allocate softstate */
	msfun = calloc(1, sizeof(usbmast_fun_t));
	if (msfun == NULL) {
		usb_log_error("Failed allocating softstate.\n");
		rc = ENOMEM;
		goto error;
	}

	fun = ddf_fun_create(dev->ddf_dev, fun_exposed, fun_name);
	if (fun == NULL) {
		usb_log_error("Failed to create DDF function %s.\n", fun_name);
		rc = ENOMEM;
		goto error;
	}

	/* Set up a connection handler. */
	fun->conn_handler = usbmast_bd_connection;
	fun->driver_data = msfun;

	usb_log_info("Initializing mass storage `%s'.\n",
	    dev->ddf_dev->name);
	usb_log_debug(" Bulk in endpoint: %d [%zuB].\n",
	    dev->pipes[BULK_IN_EP].pipe->endpoint_no,
	    (size_t) dev->pipes[BULK_IN_EP].descriptor->max_packet_size);
	usb_log_debug("Bulk out endpoint: %d [%zuB].\n",
	    dev->pipes[BULK_OUT_EP].pipe->endpoint_no,
	    (size_t) dev->pipes[BULK_OUT_EP].descriptor->max_packet_size);

	usb_log_debug("Get LUN count...\n");
	size_t lun_count = usb_masstor_get_lun_count(dev);

	/* XXX Handle more than one LUN properly. */
	if (lun_count > 1) {
		usb_log_warning ("Mass storage has %zu LUNs. Ignoring all "
		    "but first.\n", lun_count);
	}

	usb_log_debug("Inquire...\n");
	usbmast_inquiry_data_t inquiry;
	rc = usbmast_inquiry(dev, &inquiry);
	if (rc != EOK) {
		usb_log_warning("Failed to inquire device `%s': %s.\n",
		    dev->ddf_dev->name, str_error(rc));
		rc = EIO;
		goto error;
	}

	usb_log_info("Mass storage `%s': " \
	    "%s by %s rev. %s is %s (%s), %zu LUN(s).\n",
	    dev->ddf_dev->name,
	    inquiry.product,
	    inquiry.vendor,
	    inquiry.revision,
	    usbmast_scsi_dev_type_str(inquiry.device_type),
	    inquiry.removable ? "removable" : "non-removable",
	    lun_count);

	uint32_t nblocks, block_size;

	rc = usbmast_read_capacity(dev, &nblocks, &block_size);
	if (rc != EOK) {
		usb_log_warning("Failed to read capacity, device `%s': %s.\n",
		    dev->ddf_dev->name, str_error(rc));
		rc = EIO;
		goto error;
	}

	usb_log_info("Read Capacity: nblocks=%" PRIu32 ", "
	    "block_size=%" PRIu32 "\n", nblocks, block_size);

	msfun->nblocks = nblocks;
	msfun->block_size = block_size;
	msfun->usb_dev = dev;

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		usb_log_error("Failed to bind DDF function %s: %s.\n",
		    fun_name, str_error(rc));
		goto error;
	}

	return EOK;

	/* Error cleanup */
error:
	if (fun != NULL)
		ddf_fun_destroy(fun);
	if (msfun != NULL)
		free(msfun);
	return rc;
}

/** Blockdev client connection handler. */
static void usbmast_bd_connection(ipc_callid_t iid, ipc_call_t *icall,
    void *arg)
{
	usbmast_fun_t *msfun;
	void *comm_buf = NULL;
	size_t comm_size;
	ipc_callid_t callid;
	ipc_call_t call;
	unsigned int flags;
	sysarg_t method;
	uint64_t ba;
	size_t cnt;
	int retval;

	async_answer_0(iid, EOK);

	if (!async_share_out_receive(&callid, &comm_size, &flags)) {
		async_answer_0(callid, EHANGUP);
		return;
	}

	comm_buf = as_get_mappable_page(comm_size);
	if (comm_buf == NULL) {
		async_answer_0(callid, EHANGUP);
		return;
	}

	(void) async_share_out_finalize(callid, comm_buf);

	msfun = (usbmast_fun_t *) ((ddf_fun_t *)arg)->driver_data;

	while (true) {
		callid = async_get_call(&call);
		method = IPC_GET_IMETHOD(call);

		if (!method) {
			/* The other side hung up. */
			async_answer_0(callid, EOK);
			return;
		}

		switch (method) {
		case BD_GET_BLOCK_SIZE:
			async_answer_1(callid, EOK, msfun->block_size);
			break;
		case BD_GET_NUM_BLOCKS:
			async_answer_2(callid, EOK, LOWER32(msfun->nblocks),
			    UPPER32(msfun->nblocks));
			break;
		case BD_READ_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call), IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			retval = usbmast_read(msfun->usb_dev, ba, cnt,
			    msfun->block_size, comm_buf);
			async_answer_0(callid, retval);
			break;
		case BD_WRITE_BLOCKS:
			ba = MERGE_LOUP32(IPC_GET_ARG1(call), IPC_GET_ARG2(call));
			cnt = IPC_GET_ARG3(call);
			retval = usbmast_write(msfun->usb_dev, ba, cnt,
			    msfun->block_size, comm_buf);
			async_answer_0(callid, retval);
			break;
		default:
			async_answer_0(callid, EINVAL);
		}
	}
}

/** USB mass storage driver ops. */
static usb_driver_ops_t usbmast_driver_ops = {
	.add_device = usbmast_add_device,
};

/** USB mass storage driver. */
static usb_driver_t usbmast_driver = {
	.name = NAME,
	.ops = &usbmast_driver_ops,
	.endpoints = mast_endpoints
};

int main(int argc, char *argv[])
{
	usb_log_enable(USB_LOG_LEVEL_DEFAULT, NAME);

	return usb_driver_main(&usbmast_driver);
}

/**
 * @}
 */
