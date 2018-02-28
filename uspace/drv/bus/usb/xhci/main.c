/*
 * Copyright (c) 2018 Ondrej Hlavaty, Petr Manek
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

static inline xhci_hc_t *hcd_to_hc(hc_device_t *hcd)
{
	assert(hcd);
	return (xhci_hc_t *) hcd;
}

static errno_t hcd_hc_add(hc_device_t *hcd, const hw_res_list_parsed_t *hw_res)
{
	errno_t err;
	xhci_hc_t *hc = hcd_to_hc(hcd);
	hc_device_setup(hcd, (bus_t *) &hc->bus);

	if ((err = hc_init_mmio(hc, hw_res)))
		return err;

	if ((err = hc_init_memory(hc, hcd->ddf_dev)))
		return err;

	return EOK;
}

static errno_t hcd_irq_code_gen(irq_code_t *code, hc_device_t *hcd,
    const hw_res_list_parsed_t *hw_res, int *irq)
{
	xhci_hc_t *hc = hcd_to_hc(hcd);
	return hc_irq_code_gen(code, hc, hw_res, irq);
}

static errno_t hcd_claim(hc_device_t *hcd)
{
	xhci_hc_t *hc = hcd_to_hc(hcd);
	return hc_claim(hc, hcd->ddf_dev);
}

static errno_t hcd_start(hc_device_t *hcd)
{
	xhci_hc_t *hc = hcd_to_hc(hcd);
	return hc_start(hc);
}

static errno_t hcd_hc_gone(hc_device_t *hcd)
{
	xhci_hc_t *hc = hcd_to_hc(hcd);
	hc_fini(hc);
	return EOK;
}

static const hc_driver_t xhci_driver = {
	.name = NAME,
	.hc_device_size = sizeof(xhci_hc_t),

	.hc_add = hcd_hc_add,
	.irq_code_gen = hcd_irq_code_gen,
	.claim = hcd_claim,
	.start = hcd_start,
	.hc_gone = hcd_hc_gone,
};

/** Initializes global driver structures (NONE).
 *
 * @param[in] argc Nmber of arguments in argv vector (ignored).
 * @param[in] argv Cmdline argument vector (ignored).
 * @return Error code.
 *
 * Driver debug level is set here.
 */
errno_t main(int argc, char *argv[])
{
	log_init(NAME);
	logctl_set_log_level(NAME, LVL_NOTE);
	return hc_driver_main(&xhci_driver);
}

/**
 * @}
 */
