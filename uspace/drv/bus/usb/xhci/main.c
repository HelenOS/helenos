/*
 * Copyright (c) 2017 Ondrej Hlavaty <aearsis@eideo.cz>
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

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * Main routines of XHCI driver.
 */

#include <ddf/driver.h>
#include <errno.h>
#include <io/log.h>
#include <io/logctl.h>
#include <usb/debug.h>
#include <usb/host/ddf_helpers.h>

#include "hc.h"
#include "rh.h"
#include "endpoint.h"

#define NAME "xhci"

static int hc_driver_init(hcd_t *, const hw_res_list_parsed_t *, ddf_dev_t *);
static int hcd_irq_code_gen(irq_code_t *, hcd_t *, const hw_res_list_parsed_t *);
static int hcd_claim(hcd_t *, ddf_dev_t *);
static int hcd_start(hcd_t *, bool);
static int hcd_status(hcd_t *, uint32_t *);
static void hcd_interrupt(hcd_t *, uint32_t);
static int hcd_schedule(hcd_t *, usb_transfer_batch_t *);
static void hc_driver_fini(hcd_t *);

static const ddf_hc_driver_t xhci_ddf_hc_driver = {
	.name = "XHCI-PCI",
	.init = hc_driver_init,
	.irq_code_gen = hcd_irq_code_gen,
	.claim = hcd_claim,
	.start = hcd_start,
	.setup_root_hub = NULL,
	.fini = hc_driver_fini,
	.ops = {
		.schedule       = hcd_schedule,
		.irq_hook       = hcd_interrupt,
		.status_hook    = hcd_status,
	}
};

static int hc_driver_init(hcd_t *hcd, const hw_res_list_parsed_t *hw_res, ddf_dev_t *device)
{
	int err;

	xhci_hc_t *hc = malloc(sizeof(xhci_hc_t));
	if (!hc)
		return ENOMEM;

	if ((err = hc_init_mmio(hc, hw_res)))
		goto err;

	hc->hcd = hcd;

	if ((err = hc_init_memory(hc, device)))
		goto err;

	hcd_set_implementation(hcd, hc, &xhci_ddf_hc_driver.ops, &hc->bus.base);

	return EOK;
err:
	free(hc);
	return err;
}

static int hcd_irq_code_gen(irq_code_t *code, hcd_t *hcd, const hw_res_list_parsed_t *hw_res)
{
	xhci_hc_t *hc = hcd_get_driver_data(hcd);
	assert(hc);

	return hc_irq_code_gen(code, hc, hw_res);
}

static int hcd_claim(hcd_t *hcd, ddf_dev_t *dev)
{
	xhci_hc_t *hc = hcd_get_driver_data(hcd);
	assert(hc);

	return hc_claim(hc, dev);
}

static int hcd_start(hcd_t *hcd, bool irq)
{
	xhci_hc_t *hc = hcd_get_driver_data(hcd);
	assert(hc);

	return hc_start(hc, irq);
}

static int hcd_schedule(hcd_t *hcd, usb_transfer_batch_t *batch)
{
	xhci_hc_t *hc = hcd_get_driver_data(hcd);
	assert(hc);

	return hc_schedule(hc, batch);
}

static int hcd_status(hcd_t *hcd, uint32_t *status)
{
	xhci_hc_t *hc = hcd_get_driver_data(hcd);
	assert(hc);
	assert(status);

	return hc_status(hc, status);
}

static void hcd_interrupt(hcd_t *hcd, uint32_t status)
{
	xhci_hc_t *hc = hcd_get_driver_data(hcd);
	assert(hc);

	hc_interrupt(hc, status);
}

static void hc_driver_fini(hcd_t *hcd)
{
	xhci_hc_t *hc = hcd_get_driver_data(hcd);
	assert(hc);

	hc_fini(hc);

	free(hc);
}

/** Initializes a new ddf driver instance of XHCI hcd.
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
static int xhci_dev_add(ddf_dev_t *device)
{
	usb_log_info("Adding device %s", ddf_dev_get_name(device));
	return hcd_ddf_add_hc(device, &xhci_ddf_hc_driver);
}

static int xhci_fun_online(ddf_fun_t *fun)
{
	return hcd_ddf_device_online(fun);
}

static int xhci_fun_offline(ddf_fun_t *fun)
{
	return hcd_ddf_device_offline(fun);
}


static const driver_ops_t xhci_driver_ops = {
	.dev_add = xhci_dev_add,
	.fun_online = xhci_fun_online,
	.fun_offline = xhci_fun_offline
};

static const driver_t xhci_driver = {
	.name = NAME,
	.driver_ops = &xhci_driver_ops
};


/** Initializes global driver structures (NONE).
 *
 * @param[in] argc Nmber of arguments in argv vector (ignored).
 * @param[in] argv Cmdline argument vector (ignored).
 * @return Error code.
 *
 * Driver debug level is set here.
 */
int main(int argc, char *argv[])
{
	log_init(NAME);
	logctl_set_log_level(NAME, LVL_DEBUG2);
	return ddf_driver_main(&xhci_driver);
}

/**
 * @}
 */
