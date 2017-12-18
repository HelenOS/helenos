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

/** @addtogroup libusbhost
 * @{
 */
/** @file
 *
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
	.fun_online = hc_fun_offline,
	.fun_offline = hc_fun_offline,
};

static const hc_driver_t *hc_driver;

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

/** IRQ handling callback, forward status from call to diver structure.
 *
 * @param[in] dev DDF instance of the device to use.
 * @param[in] iid (Unused).
 * @param[in] call Pointer to the call from kernel.
 */
static void irq_handler(ipc_callid_t iid, ipc_call_t *call, ddf_dev_t *dev)
{
	assert(dev);
	hc_device_t *hcd = dev_to_hcd(dev);

	const bus_ops_t *ops = BUS_OPS_LOOKUP(hcd->bus->ops, interrupt);
	assert(ops);

	const uint32_t status = IPC_GET_ARG1(*call);
	ops->interrupt(hcd->bus, status);
}

/** Worker for the HW interrupt replacement fibril.
 */
static int interrupt_polling(void *arg)
{
	hc_device_t *hcd = arg;
	assert(hcd);
	bus_t *bus = hcd->bus;

	const bus_ops_t *interrupt_ops = BUS_OPS_LOOKUP(bus->ops, interrupt);
	const bus_ops_t *status_ops = BUS_OPS_LOOKUP(bus->ops, status);
	if (!interrupt_ops || !status_ops)
		return ENOTSUP;

	uint32_t status = 0;
	while (status_ops->status(bus, &status) == EOK) {
		interrupt_ops->interrupt(bus, status);
		status = 0;
		/* We should wait 1 frame - 1ms here, but this polling is a
		 * lame crutch anyway so don't hog the system. 10ms is still
		 * good enough for emergency mode */
		async_usleep(10000);
	}
	return EOK;
}

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

/** Register interrupt handler
 *
 * @param[in] device Host controller DDF device
 * @param[in] regs Register range
 * @param[in] irq Interrupt number
 * @paran[in] handler Interrupt handler
 * @param[in] gen_irq_code IRQ code generator.
 *
 * @return IRQ capability handle on success.
 * @return Negative error code.
 */
static int hcd_ddf_setup_interrupts(hc_device_t *hcd, const hw_res_list_parsed_t *hw_res)
{
	assert(hcd);
	irq_code_t irq_code = {0};

	if (!hc_driver->irq_code_gen)
		return ENOTSUP;

	const int irq = hc_driver->irq_code_gen(&irq_code, hcd, hw_res);
	if (irq < 0) {
		usb_log_error("Failed to generate IRQ code: %s.\n",
		    str_error(irq));
		return irq;
	}

	/* Register handler to avoid interrupt lockup */
	const int irq_cap = register_interrupt_handler(hcd->ddf_dev, irq, irq_handler, &irq_code);
	irq_code_clean(&irq_code);
	if (irq_cap < 0) {
		usb_log_error("Failed to register interrupt handler: %s.\n",
		    str_error(irq_cap));
		return irq_cap;
	}

	/* Enable interrupts */
	int ret = hcd_ddf_enable_interrupt(hcd, irq);
	if (ret != EOK) {
		usb_log_error("Failed to enable interrupts: %s.\n",
		    str_error(ret));
		unregister_interrupt_handler(hcd->ddf_dev, irq_cap);
		return ret;
	}
	return irq_cap;
}

/** Initialize HC in memory of the driver.
 *
 * @param device DDF instance of the device to use
 * @return Error code
 *
 * This function does all the preparatory work for hc and rh drivers:
 *  - gets device's hw resources
 *  - attempts to enable interrupts
 *  - registers interrupt handler
 *  - calls driver specific initialization
 *  - registers root hub
 */
int hc_dev_add(ddf_dev_t *device)
{
	int ret = EOK;
	assert(device);

	if (!hc_driver->hc_add) {
		usb_log_error("Driver '%s' does not support adding devices.", hc_driver->name);
		return ENOTSUP;
	}

	ret = hcd_ddf_setup_hc(device, hc_driver->hc_device_size);
	if (ret != EOK) {
		usb_log_error("Failed to setup HC device.\n");
		return ret;
	}

	hc_device_t *hcd = dev_to_hcd(device);

	hw_res_list_parsed_t hw_res;
	ret = hcd_ddf_get_registers(hcd, &hw_res);
	if (ret != EOK) {
		usb_log_error("Failed to get register memory addresses "
		    "for `%s': %s.\n", ddf_dev_get_name(device),
		    str_error(ret));
		goto err_hcd;
	}

	ret = hc_driver->hc_add(hcd, &hw_res);
	if (ret != EOK) {
		usb_log_error("Failed to init HCD.\n");
		goto err_hw_res;
	}

	assert(hcd->bus);

	/* Setup interrupts  */
	hcd->irq_cap = hcd_ddf_setup_interrupts(hcd, &hw_res);
	if (hcd->irq_cap >= 0) {
		usb_log_debug("Hw interrupts enabled.\n");
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
		usb_log_error("Failed to start HCD: %s.\n", str_error(ret));
		goto err_irq;
	}

	const bus_ops_t *ops = BUS_OPS_LOOKUP(hcd->bus->ops, status);

	/* Need working irq replacement to setup root hub */
	if (hcd->irq_cap < 0 && ops) {
		hcd->polling_fibril = fibril_create(interrupt_polling, hcd->bus);
		if (!hcd->polling_fibril) {
			usb_log_error("Failed to create polling fibril\n");
			ret = ENOMEM;
			goto err_started;
		}
		fibril_add_ready(hcd->polling_fibril);
		usb_log_warning("Failed to enable interrupts: %s."
		    " Falling back to polling.\n", str_error(hcd->irq_cap));
	}

	/*
	 * Creating root hub registers a new USB device so HC
	 * needs to be ready at this time.
	 */
	if (hc_driver->setup_root_hub)
		ret = hc_driver->setup_root_hub(hcd);
	if (ret != EOK) {
		usb_log_error("Failed to setup HC root hub: %s.\n",
		    str_error(ret));
		goto err_polling;
	}

	usb_log_info("Controlling new `%s' device `%s'.\n",
	   hc_driver->name, ddf_dev_get_name(device));
	return EOK;

err_polling:
	// TODO: Stop the polling fibril (refactor the interrupt_polling func)
	//
err_started:
	if (hc_driver->stop)
		hc_driver->stop(hcd);
err_irq:
	unregister_interrupt_handler(device, hcd->irq_cap);
	if (hc_driver->hc_remove)
		hc_driver->hc_remove(hcd);
err_hw_res:
	hw_res_list_parsed_clean(&hw_res);
err_hcd:
	hcd_ddf_clean_hc(hcd);
	return ret;
}

int hc_dev_remove(ddf_dev_t *dev)
{
	int err;
	hc_device_t *hcd = dev_to_hcd(dev);

	if (hc_driver->stop)
		if ((err = hc_driver->stop(hcd)))
			return err;

	unregister_interrupt_handler(dev, hcd->irq_cap);

	if (hc_driver->hc_remove)
		if ((err = hc_driver->hc_remove(hcd)))
			return err;

	hcd_ddf_clean_hc(hcd);

	// TODO probably not complete

	return EOK;
}

int hc_dev_gone(ddf_dev_t *dev)
{
	int err = ENOTSUP;
	hc_device_t *hcd = dev_to_hcd(dev);

	if (hc_driver->hc_gone)
		err = hc_driver->hc_gone(hcd);

	hcd_ddf_clean_hc(hcd);

	return err;
}

int hc_fun_online(ddf_fun_t *fun)
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

/** Get max packet size for the control endpoint 0.
 *
 * For LS, HS, and SS devices this value is fixed. For FS devices we must fetch
 * the first 8B of the device descriptor to determine it.
 *
 * @return Max packet size for EP 0
 */
int hcd_get_ep0_max_packet_size(uint16_t *mps, bus_t *bus, device_t *dev)
{
	assert(mps);

	static const uint16_t mps_fixed [] = {
		[USB_SPEED_LOW] = 8,
		[USB_SPEED_HIGH] = 64,
		[USB_SPEED_SUPER] = 512,
	};

	if (dev->speed < ARRAY_SIZE(mps_fixed) && mps_fixed[dev->speed] != 0) {
		*mps = mps_fixed[dev->speed];
		return EOK;
	}

	const usb_target_t control_ep = {{
		.address = dev->address,
		.endpoint = 0,
	}};

	usb_standard_device_descriptor_t desc = { 0 };
	const usb_device_request_setup_packet_t get_device_desc_8 =
	    GET_DEVICE_DESC(CTRL_PIPE_MIN_PACKET_SIZE);

	usb_log_debug("Requesting first 8B of device descriptor to determine MPS.");
	ssize_t got = bus_device_send_batch_sync(dev, control_ep, USB_DIRECTION_IN,
	    (char *) &desc, CTRL_PIPE_MIN_PACKET_SIZE, *(uint64_t *)&get_device_desc_8,
	    "read first 8 bytes of dev descriptor");

	if (got != CTRL_PIPE_MIN_PACKET_SIZE) {
		const int err = got < 0 ? got : EOVERFLOW;
		usb_log_error("Failed to get 8B of dev descr: %s.", str_error(err));
		return err;
	}

	if (desc.descriptor_type != USB_DESCTYPE_DEVICE) {
		usb_log_error("The device responded with wrong device descriptor.");
		return EIO;
	}

	uint16_t version = uint16_usb2host(desc.usb_spec_version);
	if (version < 0x0300) {
		/* USB 2 and below have MPS raw in the field */
		*mps = desc.max_packet_size;
	} else {
		/* USB 3 have MPS as an 2-based exponent */
		*mps = (1 << desc.max_packet_size);
	}
	return EOK;
}

/**
 * Setup devices Transaction Translation.
 *
 * This applies for Low/Full speed devices under High speed hub only. Other
 * devices just inherit TT from the hub.
 *
 * Roothub must be handled specially.
 */
void hcd_setup_device_tt(device_t *dev)
{
	if (!dev->hub)
		return;

	if (dev->hub->speed == USB_SPEED_HIGH && usb_speed_is_11(dev->speed)) {
		/* For LS devices under HS hub */
		dev->tt.address = dev->hub->address;
		dev->tt.port = dev->port;
	}
	else {
		/* Inherit hub's TT */
		dev->tt = dev->hub->tt;
	}
}

/** Check setup packet data for signs of toggle reset.
 *
 * @param[in] requst Setup requst data.
 *
 * @retval -1 No endpoints need reset.
 * @retval 0 All endpoints need reset.
 * @retval >0 Specified endpoint needs reset.
 *
 */
toggle_reset_mode_t hcd_get_request_toggle_reset_mode(
    const usb_device_request_setup_packet_t *request)
{
	assert(request);
	switch (request->request)
	{
	/* Clear Feature ENPOINT_STALL */
	case USB_DEVREQ_CLEAR_FEATURE: /*resets only cleared ep */
		/* 0x2 ( HOST to device | STANDART | TO ENPOINT) */
		if ((request->request_type == 0x2) &&
		    (request->value == USB_FEATURE_ENDPOINT_HALT))
			return RESET_EP;
		break;
	case USB_DEVREQ_SET_CONFIGURATION:
	case USB_DEVREQ_SET_INTERFACE:
		/* Recipient must be device, this resets all endpoints,
		 * In fact there should be no endpoints but EP 0 registered
		 * as different interfaces use different endpoints,
		 * unless you're changing configuration or alternative
		 * interface of an already setup device. */
		if (!(request->request_type & SETUP_REQUEST_TYPE_DEVICE_TO_HOST))
			return RESET_ALL;
		break;
	default:
		break;
	}

	return RESET_NONE;
}


/**
 * @}
 */
