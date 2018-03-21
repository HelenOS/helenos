/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty
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

/** @addtogroup libusbhost
 * @{
 */
/** @file
 *
 * Host controller driver framework. Encapsulates DDF device of HC to an
 * hc_device_t, which is passed to driver implementing hc_driver_t.
 */

#include <assert.h>
#include <async.h>
#include <ddf/interrupt.h>
#include <errno.h>
#include <macros.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/request.h>
#include <usb_iface.h>

#include "bus.h"
#include "ddf_helpers.h"
#include "endpoint.h"
#include "usb_transfer_batch.h"

#include "hcd.h"

int hc_dev_add(ddf_dev_t *);
int hc_dev_remove(ddf_dev_t *);
int hc_dev_gone(ddf_dev_t *);
int hc_fun_online(ddf_fun_t *);
int hc_fun_offline(ddf_fun_t *);

static driver_ops_t hc_driver_ops = {
	.dev_add = hc_dev_add,
	.dev_remove = hc_dev_remove,
	.dev_gone = hc_dev_gone,
	.fun_online = hc_fun_online,
	.fun_offline = hc_fun_offline,
};

static const hc_driver_t *hc_driver;

/**
 * The main HC driver routine.
 */
int hc_driver_main(const hc_driver_t *driver)
{
	driver_t ddf_driver = {
		.name = driver->name,
		.driver_ops = &hc_driver_ops,
	};

	/* Remember ops to call. */
	hc_driver = driver;

	return ddf_driver_main(&ddf_driver);
}

/**
 * IRQ handling callback. Call the bus operation.
 *
 * Currently, there is a bus ops lookup to find the interrupt handler. So far,
 * the mechanism is too flexible, as it allows different instances of HC to
 * have different IRQ handlers, disallowing us to optimize the lookup here.
 * TODO: Make the bus mechanism less flexible in irq handling and remove the
 * lookup.
 */
static void irq_handler(ipc_call_t *call, ddf_dev_t *dev)
{
	assert(dev);
	hc_device_t *hcd = dev_to_hcd(dev);

	const uint32_t status = IPC_GET_ARG1(*call);
	hcd->bus->ops->interrupt(hcd->bus, status);
}

/**
 * Worker for the HW interrupt replacement fibril.
 */
static errno_t interrupt_polling(void *arg)
{
	bus_t *bus = arg;
	assert(bus);

	if (!bus->ops->interrupt || !bus->ops->status)
		return ENOTSUP;

	uint32_t status = 0;
	while (bus->ops->status(bus, &status) == EOK) {
		bus->ops->interrupt(bus, status);
		status = 0;
		/* We should wait 1 frame - 1ms here, but this polling is a
		 * lame crutch anyway so don't hog the system. 10ms is still
		 * good enough for emergency mode */
		async_usleep(10000);
	}
	return EOK;
}

/**
 * Clean the IRQ code bottom-half.
 */
static inline void irq_code_clean(irq_code_t *code)
{
	if (code) {
		free(code->ranges);
		free(code->cmds);
		code->ranges = NULL;
		code->cmds = NULL;
		code->rangecount = 0;
		code->cmdcount = 0;
	}
}

/**
 * Register an interrupt handler. If there is a callback to setup the bottom half,
 * invoke it and register it. Register for notifications.
 *
 * If this method fails, a polling fibril is started instead.
 *
 * @param[in]  hcd         Host controller device.
 * @param[in]  hw_res      Resources to be used.
 * @param[out] irq_handle  Storage for the returned IRQ handle
 *
 * @return Error code.
 */
static errno_t hcd_ddf_setup_interrupts(hc_device_t *hcd,
    const hw_res_list_parsed_t *hw_res, cap_irq_handle_t *irq_handle)
{
	assert(hcd);
	irq_code_t irq_code = { 0 };

	if (!hc_driver->irq_code_gen)
		return ENOTSUP;

	int irq;
	errno_t ret;
	ret = hc_driver->irq_code_gen(&irq_code, hcd, hw_res, &irq);
	if (ret != EOK) {
		usb_log_error("Failed to generate IRQ code: %s.",
		    str_error(ret));
		return ret;
	}

	/* Register handler to avoid interrupt lockup */
	cap_irq_handle_t ihandle;
	ret = register_interrupt_handler(hcd->ddf_dev, irq, irq_handler,
	    &irq_code, &ihandle);
	irq_code_clean(&irq_code);
	if (ret != EOK) {
		usb_log_error("Failed to register interrupt handler: %s.",
		    str_error(ret));
		return ret;
	}

	/* Enable interrupts */
	ret = hcd_ddf_enable_interrupt(hcd, irq);
	if (ret != EOK) {
		usb_log_error("Failed to enable interrupts: %s.",
		    str_error(ret));
		unregister_interrupt_handler(hcd->ddf_dev, ihandle);
		return ret;
	}

	*irq_handle = ihandle;
	return EOK;
}

/**
 * Initialize HC in memory of the driver.
 *
 * This function does all the preparatory work for hc and rh drivers:
 *  - gets device's hw resources
 *  - attempts to enable interrupts
 *  - registers interrupt handler
 *  - calls driver specific initialization
 *  - registers root hub
 *
 * @param device DDF instance of the device to use
 * @return Error code
 */
errno_t hc_dev_add(ddf_dev_t *device)
{
	errno_t ret = EOK;
	assert(device);

	if (!hc_driver->hc_add) {
		usb_log_error("Driver '%s' does not support adding devices.",
		    hc_driver->name);
		return ENOTSUP;
	}

	ret = hcd_ddf_setup_hc(device, hc_driver->hc_device_size);
	if (ret != EOK) {
		usb_log_error("Failed to setup HC device.");
		return ret;
	}

	hc_device_t *hcd = dev_to_hcd(device);

	hw_res_list_parsed_t hw_res;
	ret = hcd_ddf_get_registers(hcd, &hw_res);
	if (ret != EOK) {
		usb_log_error("Failed to get register memory addresses "
		    "for `%s': %s.", ddf_dev_get_name(device),
		    str_error(ret));
		goto err_hcd;
	}

	ret = hc_driver->hc_add(hcd, &hw_res);
	if (ret != EOK) {
		usb_log_error("Failed to init HCD.");
		goto err_hw_res;
	}

	assert(hcd->bus);

	/* Setup interrupts  */
	hcd->irq_handle = CAP_NIL;
	errno_t irqerr = hcd_ddf_setup_interrupts(hcd, &hw_res,
	    &hcd->irq_handle);
	if (irqerr == EOK) {
		usb_log_debug("Hw interrupts enabled.");
	}

	/* Claim the device from BIOS */
	if (hc_driver->claim)
		ret = hc_driver->claim(hcd);
	if (ret != EOK) {
		usb_log_error("Failed to claim `%s' for `%s': %s",
		    ddf_dev_get_name(device), hc_driver->name, str_error(ret));
		goto err_irq;
	}

	/* Start hw */
	if (hc_driver->start)
		ret = hc_driver->start(hcd);
	if (ret != EOK) {
		usb_log_error("Failed to start HCD: %s.", str_error(ret));
		goto err_irq;
	}

	const bus_ops_t *ops = hcd->bus->ops;

	/* Need working irq replacement to setup root hub */
	if (irqerr != EOK && ops->status) {
		hcd->polling_fibril = fibril_create(interrupt_polling, hcd->bus);
		if (!hcd->polling_fibril) {
			usb_log_error("Failed to create polling fibril");
			ret = ENOMEM;
			goto err_started;
		}
		fibril_add_ready(hcd->polling_fibril);
		usb_log_warning("Failed to enable interrupts: %s."
		    " Falling back to polling.", str_error(irqerr));
	}

	/*
	 * Creating root hub registers a new USB device so HC
	 * needs to be ready at this time.
	 */
	if (hc_driver->setup_root_hub)
		ret = hc_driver->setup_root_hub(hcd);
	if (ret != EOK) {
		usb_log_error("Failed to setup HC root hub: %s.",
		    str_error(ret));
		goto err_polling;
	}

	usb_log_info("Controlling new `%s' device `%s'.",
	    hc_driver->name, ddf_dev_get_name(device));
	return EOK;

err_polling:
	// TODO: Stop the polling fibril (refactor the interrupt_polling func)
	//
err_started:
	if (hc_driver->stop)
		hc_driver->stop(hcd);
err_irq:
	unregister_interrupt_handler(device, hcd->irq_handle);
	if (hc_driver->hc_remove)
		hc_driver->hc_remove(hcd);
err_hw_res:
	hw_res_list_parsed_clean(&hw_res);
err_hcd:
	hcd_ddf_clean_hc(hcd);
	return ret;
}

errno_t hc_dev_remove(ddf_dev_t *dev)
{
	errno_t err;
	hc_device_t *hcd = dev_to_hcd(dev);

	if (hc_driver->stop)
		if ((err = hc_driver->stop(hcd)))
			return err;

	unregister_interrupt_handler(dev, hcd->irq_handle);

	if (hc_driver->hc_remove)
		if ((err = hc_driver->hc_remove(hcd)))
			return err;

	hcd_ddf_clean_hc(hcd);

	// TODO probably not complete

	return EOK;
}

errno_t hc_dev_gone(ddf_dev_t *dev)
{
	errno_t err = ENOTSUP;
	hc_device_t *hcd = dev_to_hcd(dev);

	if (hc_driver->hc_gone)
		err = hc_driver->hc_gone(hcd);

	hcd_ddf_clean_hc(hcd);

	return err;
}

errno_t hc_fun_online(ddf_fun_t *fun)
{
	assert(fun);

	device_t *dev = ddf_fun_data_get(fun);
	assert(dev);

	usb_log_info("Device(%d): Requested to be brought online.", dev->address);
	return bus_device_online(dev);
}

int hc_fun_offline(ddf_fun_t *fun)
{
	assert(fun);

	device_t *dev = ddf_fun_data_get(fun);
	assert(dev);

	usb_log_info("Device(%d): Requested to be taken offline.", dev->address);
	return bus_device_offline(dev);
}


/**
 * @}
 */
