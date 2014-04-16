/*
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup drvusbuhcirh
 * @{
 */
/** @file
 * @brief UHCI root hub initialization routines
 */

#include <ddf/driver.h>
#include <devman.h>
#include <device/hw_res_parsed.h>
#include <errno.h>
#include <str_error.h>

#include <usb_iface.h>
#include <usb/ddfiface.h>
#include <usb/debug.h>

#include "root_hub.h"

#define NAME "uhcirh"

static int hc_get_my_registers(ddf_dev_t *dev, addr_range_t *io_regs);

static int uhci_rh_dev_add(ddf_dev_t *device);

static driver_ops_t uhci_rh_driver_ops = {
	.dev_add = uhci_rh_dev_add,
};

static driver_t uhci_rh_driver = {
	.name = NAME,
	.driver_ops = &uhci_rh_driver_ops
};

/** Initialize global driver structures (NONE).
 *
 * @param[in] argc Nmber of arguments in argv vector (ignored).
 * @param[in] argv Cmdline argument vector (ignored).
 * @return Error code.
 *
 * Driver debug level is set here.
 */
int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS UHCI root hub driver.\n");
	log_init(NAME);
	return ddf_driver_main(&uhci_rh_driver);
}

/** Initialize a new ddf driver instance of UHCI root hub.
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
static int uhci_rh_dev_add(ddf_dev_t *device)
{
	if (!device)
		return EINVAL;

	usb_log_debug2("uhci_rh_dev_add(handle=%" PRIun ")\n",
	    ddf_dev_get_handle(device));

	addr_range_t regs;
	uhci_root_hub_t *rh = NULL;
	int rc;

	rc = hc_get_my_registers(device, &regs);
	if (rc != EOK) {
		usb_log_error( "Failed to get registers from HC: %s.\n",
		    str_error(rc));
		return rc;
	}

	usb_log_debug("I/O regs at %p (size %zuB).\n",
	    RNGABSPTR(regs), RNGSZ(regs));

	rh = ddf_dev_data_alloc(device, sizeof(uhci_root_hub_t));
	if (rh == NULL) {
		usb_log_error("Failed to allocate rh driver instance.\n");
		return ENOMEM;
	}

	rc = uhci_root_hub_init(rh, &regs, device);
	if (rc != EOK) {
		usb_log_error("Failed(%d) to initialize rh driver instance: "
		    "%s.\n", rc, str_error(rc));
		return rc;
	}

	usb_log_info("Controlling root hub '%s' (%" PRIun ").\n",
	    ddf_dev_get_name(device), ddf_dev_get_handle(device));

	return EOK;
}

/** Get address of I/O registers.
 *
 * @param[in] dev Device asking for the addresses.
 * @param[out] io_regs_p Pointer to the device's register range.
 * @return Error code.
 */
int hc_get_my_registers(ddf_dev_t *dev, addr_range_t *io_regs_p)
{
	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_SERIALIZE,
	    ddf_dev_get_handle(dev), IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;

	hw_res_list_parsed_t hw_res;
	hw_res_list_parsed_init(&hw_res);
	const int ret = hw_res_get_list_parsed(parent_sess, &hw_res, 0);
	async_hangup(parent_sess);
	if (ret != EOK) {
		return ret;
	}

	if (hw_res.io_ranges.count != 1) {
		hw_res_list_parsed_clean(&hw_res);
		return EINVAL;
	}

	if (io_regs_p != NULL)
		*io_regs_p = hw_res.io_ranges.ranges[0];

	hw_res_list_parsed_clean(&hw_res);
	return EOK;
}

/**
 * @}
 */
