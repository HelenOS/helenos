/*
 * Copyright (c) 2013 Jan Vesely
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

#include <adt/list.h>
#include <assert.h>
#include <async.h>
#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <device/hw_res_parsed.h>
#include <errno.h>
#include <str_error.h>
#include <usb/classes/classes.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/usb.h>
#include <usb_iface.h>
#include <usbhc_iface.h>

#include "bus.h"

#include "ddf_helpers.h"

typedef struct hc_dev {
	ddf_fun_t *ctl_fun;
	hcd_t hcd;
} hc_dev_t;

static hc_dev_t *dev_to_hc_dev(ddf_dev_t *dev)
{
	return ddf_dev_data_get(dev);
}

hcd_t *dev_to_hcd(ddf_dev_t *dev)
{
	hc_dev_t *hc_dev = dev_to_hc_dev(dev);
	if (!hc_dev) {
		usb_log_error("Invalid HCD device.\n");
		return NULL;
	}
	return &hc_dev->hcd;
}


static int hcd_ddf_new_device(hcd_t *hcd, ddf_dev_t *hc, device_t *hub_dev, unsigned port);
static int hcd_ddf_remove_device(ddf_dev_t *device, device_t *hub, unsigned port);


/* DDF INTERFACE */

/** Register endpoint interface function.
 * @param fun DDF function.
 * @param endpoint_desc Endpoint description.
 * @return Error code.
 */
static int register_endpoint(
	ddf_fun_t *fun, usb_endpoint_desc_t *endpoint_desc)
{
	assert(fun);
	hcd_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(hcd->bus);
	assert(dev);

	usb_log_debug("Register endpoint %d:%d %s-%s %zuB %ums.\n",
		dev->address, endpoint_desc->endpoint_no,
		usb_str_transfer_type(endpoint_desc->transfer_type),
		usb_str_direction(endpoint_desc->direction),
		endpoint_desc->max_packet_size, endpoint_desc->usb2.polling_interval);

	return bus_add_endpoint(hcd->bus, dev, endpoint_desc, NULL);
}

 /** Unregister endpoint interface function.
  * @param fun DDF function.
  * @param endpoint_desc Endpoint description.
  * @return Error code.
  */
static int unregister_endpoint(
	ddf_fun_t *fun, usb_endpoint_desc_t *endpoint_desc)
{
	assert(fun);
	hcd_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(hcd->bus);
	assert(dev);

	const usb_target_t target = {{
		.address = dev->address,
		.endpoint = endpoint_desc->endpoint_no
	}};

	usb_log_debug("Unregister endpoint %d:%d %s.\n",
		dev->address, endpoint_desc->endpoint_no,
		usb_str_direction(endpoint_desc->direction));

	endpoint_t *ep = bus_find_endpoint(hcd->bus, dev, target, endpoint_desc->direction);
	if (!ep)
		return ENOENT;

	return bus_remove_endpoint(hcd->bus, ep);
}

static int reserve_default_address(ddf_fun_t *fun, usb_speed_t speed)
{
	assert(fun);
	hcd_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(hcd->bus);
	assert(dev);

	usb_log_debug("Device %d requested default address at %s speed\n",
	    dev->address, usb_str_speed(speed));
	return bus_reserve_default_address(hcd->bus, speed);
}

static int release_default_address(ddf_fun_t *fun)
{
	assert(fun);
	hcd_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(hcd);
	assert(hcd->bus);
	assert(dev);

	usb_log_debug("Device %d released default address\n", dev->address);
	return bus_release_default_address(hcd->bus);
}

static int device_enumerate(ddf_fun_t *fun, unsigned port)
{
	assert(fun);
	ddf_dev_t *hc = ddf_fun_get_dev(fun);
	assert(hc);
	hcd_t *hcd = dev_to_hcd(hc);
	assert(hcd);
	device_t *hub = ddf_fun_data_get(fun);
	assert(hub);

	usb_log_debug("Hub %d reported a new USB device on port: %u\n",
	    hub->address, port);
	return hcd_ddf_new_device(hcd, hc, hub, port);
}

static int device_remove(ddf_fun_t *fun, unsigned port)
{
	assert(fun);
	ddf_dev_t *ddf_dev = ddf_fun_get_dev(fun);
	device_t *dev = ddf_fun_data_get(fun);
	assert(ddf_dev);
	assert(dev);
	usb_log_debug("Hub `%s' reported removal of device on port %u\n",
	    ddf_fun_get_name(fun), port);
	return hcd_ddf_remove_device(ddf_dev, dev, port);
}

/** Gets handle of the respective device.
 *
 * @param[in] fun Device function.
 * @param[out] handle Place to write the handle.
 * @return Error code.
 */
static int get_my_device_handle(ddf_fun_t *fun, devman_handle_t *handle)
{
	assert(fun);
	if (handle)
		*handle = ddf_fun_get_handle(fun);
	return EOK;
}

/** Inbound communication interface function.
 * @param fun DDF function.
 * @param target Communication target.
 * @param setup_data Data to use in setup stage (control transfers).
 * @param data Pointer to data buffer.
 * @param size Size of the data buffer.
 * @param callback Function to call on communication end.
 * @param arg Argument passed to the callback function.
 * @return Error code.
 */
static int dev_read(ddf_fun_t *fun, usb_target_t target,
    uint64_t setup_data, char *data, size_t size,
    usbhc_iface_transfer_callback_t callback, void *arg)
{
	assert(fun);
	hcd_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(dev);

	target.address = dev->address;

	return hcd_send_batch(hcd, dev, target, USB_DIRECTION_IN,
	    data, size, setup_data,
	    callback, arg, "READ");
}

/** Outbound communication interface function.
 * @param fun DDF function.
 * @param target Communication target.
 * @param setup_data Data to use in setup stage (control transfers).
 * @param data Pointer to data buffer.
 * @param size Size of the data buffer.
 * @param callback Function to call on communication end.
 * @param arg Argument passed to the callback function.
 * @return Error code.
 */
static int dev_write(ddf_fun_t *fun, usb_target_t target,
    uint64_t setup_data, const char *data, size_t size,
    usbhc_iface_transfer_callback_t callback, void *arg)
{
	assert(fun);
	hcd_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(dev);

	target.address = dev->address;

	return hcd_send_batch(hcd, dev, target, USB_DIRECTION_OUT,
	    (char *) data, size, setup_data,
	    callback, arg, "WRITE");
}

/** USB device interface */
static usb_iface_t usb_iface = {
	.get_my_device_handle = get_my_device_handle,
};

/** USB host controller interface */
static usbhc_iface_t usbhc_iface = {
	.reserve_default_address = reserve_default_address,
	.release_default_address = release_default_address,

	.device_enumerate = device_enumerate,
	.device_remove = device_remove,

	.register_endpoint = register_endpoint,
	.unregister_endpoint = unregister_endpoint,

	.read = dev_read,
	.write = dev_write,
};

/** Standard USB device interface) */
static ddf_dev_ops_t usb_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface,
	.interfaces[USBHC_DEV_IFACE] = &usbhc_iface,
};


/* DDF HELPERS */

#define ADD_MATCHID_OR_RETURN(list, sc, str, ...) \
do { \
	match_id_t *mid = malloc(sizeof(match_id_t)); \
	if (!mid) { \
		clean_match_ids(list); \
		return ENOMEM; \
	} \
	char *id = NULL; \
	int ret = asprintf(&id, str, ##__VA_ARGS__); \
	if (ret < 0) { \
		clean_match_ids(list); \
		free(mid); \
		return ENOMEM; \
	} \
	mid->score = sc; \
	mid->id = id; \
	add_match_id(list, mid); \
} while (0)

/* This is a copy of lib/usbdev/src/recognise.c */
static int create_match_ids(match_id_list_t *l,
    usb_standard_device_descriptor_t *d)
{
	assert(l);
	assert(d);

	if (d->vendor_id != 0) {
		/* First, with release number. */
		ADD_MATCHID_OR_RETURN(l, 100,
		    "usb&vendor=%#04x&product=%#04x&release=%x.%x",
		    d->vendor_id, d->product_id, (d->device_version >> 8),
		    (d->device_version & 0xff));

		/* Next, without release number. */
		ADD_MATCHID_OR_RETURN(l, 90, "usb&vendor=%#04x&product=%#04x",
		    d->vendor_id, d->product_id);
	}

	/* Class match id */
	ADD_MATCHID_OR_RETURN(l, 50, "usb&class=%s",
	    usb_str_class(d->device_class));

	/* As a last resort, try fallback driver. */
	ADD_MATCHID_OR_RETURN(l, 10, "usb&fallback");

	return EOK;
}

static int hcd_ddf_remove_device(ddf_dev_t *device, device_t *hub,
    unsigned port)
{
	assert(device);

	hcd_t *hcd = dev_to_hcd(device);
	assert(hcd);
	assert(hcd->bus);

	hc_dev_t *hc_dev = dev_to_hc_dev(device);
	assert(hc_dev);

	fibril_mutex_lock(&hub->guard);

	device_t *victim = NULL;

	list_foreach(hub->devices, link, device_t, it) {
		if (it->port == port) {
			victim = it;
			break;
		}
	}
	if (victim) {
		assert(victim->fun);
		assert(victim->port == port);
		assert(victim->hub == hub);
		list_remove(&victim->link);
		fibril_mutex_unlock(&hub->guard);
		const int ret = ddf_fun_unbind(victim->fun);
		if (ret == EOK) {
			usb_address_t address = victim->address;
			bus_remove_device(hcd->bus, hcd, victim);
			ddf_fun_destroy(victim->fun);
			bus_release_address(hcd->bus, address);
		} else {
			usb_log_warning("Failed to unbind device `%s': %s\n",
			    ddf_fun_get_name(victim->fun), str_error(ret));
		}
		return EOK;
	}
	fibril_mutex_unlock(&hub->guard);
	return ENOENT;
}

device_t *hcd_ddf_device_create(ddf_dev_t *hc, size_t device_size)
{
	/* Create DDF function for the new device */
	ddf_fun_t *fun = ddf_fun_create(hc, fun_inner, NULL);
	if (!fun)
		return NULL;

	ddf_fun_set_ops(fun, &usb_ops);

	/* Create USB device node for the new device */
	device_t *dev = ddf_fun_data_alloc(fun, device_size);
	if (!dev) {
		ddf_fun_destroy(fun);
		return NULL;
	}

	device_init(dev);
	dev->fun = fun;
	return dev;
}

void hcd_ddf_device_destroy(device_t *dev)
{
	assert(dev);
	assert(dev->fun);
	ddf_fun_destroy(dev->fun);
}

int hcd_ddf_device_explore(hcd_t *hcd, device_t *device)
{
	int err;
	match_id_list_t mids;
	usb_standard_device_descriptor_t desc = { 0 };

	init_match_ids(&mids);

	const usb_target_t control_ep = {{
		.address = device->address,
		.endpoint = 0,
	}};

	/* Get std device descriptor */
	const usb_device_request_setup_packet_t get_device_desc =
	    GET_DEVICE_DESC(sizeof(desc));

	usb_log_debug("Device(%d): Requesting full device descriptor.",
	    device->address);
	ssize_t got = hcd_send_batch_sync(hcd, device, control_ep, USB_DIRECTION_IN,
	    (char *) &desc, sizeof(desc), *(uint64_t *)&get_device_desc,
	    "read device descriptor");
	if (got < 0) {
		err = got < 0 ? got : EOVERFLOW;
		usb_log_error("Device(%d): Failed to set get dev descriptor: %s",
		    device->address, str_error(err));
		goto out;
	}

	/* Create match ids from the device descriptor */
	usb_log_debug("Device(%d): Creating match IDs.", device->address);
	if ((err = create_match_ids(&mids, &desc))) {
		usb_log_error("Device(%d): Failed to create match ids: %s", device->address, str_error(err));
		goto out;
	}

	list_foreach(mids.ids, link, const match_id_t, mid) {
		ddf_fun_add_match_id(device->fun, mid->id, mid->score);
	}

out:
	clean_match_ids(&mids);
	return err;
}

int hcd_ddf_device_online(ddf_fun_t *fun)
{
	assert(fun);

	hcd_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(dev);
	assert(hcd->bus);

	usb_log_info("Device(%d): Requested to be brought online.", dev->address);

	return bus_online_device(hcd->bus, hcd, dev);
}

int hcd_ddf_device_offline(ddf_fun_t *fun)
{
	assert(fun);

	hcd_t *hcd = dev_to_hcd(ddf_fun_get_dev(fun));
	device_t *dev = ddf_fun_data_get(fun);
	assert(dev);
	assert(hcd->bus);

	usb_log_info("Device(%d): Requested to be taken offline.", dev->address);

	return bus_offline_device(hcd->bus, hcd, dev);
}

static int hcd_ddf_new_device(hcd_t *hcd, ddf_dev_t *hc, device_t *hub, unsigned port)
{
	int err;
	assert(hcd);
	assert(hcd->bus);
	assert(hub);
	assert(hc);

	device_t *dev = hcd_ddf_device_create(hc, hcd->bus->device_size);
	if (!dev) {
		usb_log_error("Failed to create USB device function.");
		return ENOMEM;
	}

	dev->hub = hub;
	dev->port = port;

	if ((err = bus_enumerate_device(hcd->bus, hcd, dev))) {
		usb_log_error("Failed to initialize USB dev memory structures.");
		return err;
	}

	/* If the driver didn't name the dev when enumerating,
	 * do it in some generic way.
	 */
	if (!ddf_fun_get_name(dev->fun)) {
		device_set_default_name(dev);
	}

	if ((err = ddf_fun_bind(dev->fun))) {
		usb_log_error("Device(%d): Failed to register: %s.", dev->address, str_error(err));
		goto err_usb_dev;
	}

	fibril_mutex_lock(&hub->guard);
	list_append(&dev->link, &hub->devices);
	fibril_mutex_unlock(&hub->guard);

	return EOK;

err_usb_dev:
	hcd_ddf_device_destroy(dev);
	return err;
}

/** Announce root hub to the DDF
 *
 * @param[in] device Host controller ddf device
 * @return Error code
 */
int hcd_setup_virtual_root_hub(hcd_t *hcd, ddf_dev_t *hc)
{
	int err;

	assert(hc);
	assert(hcd);
	assert(hcd->bus);

	if ((err = bus_reserve_default_address(hcd->bus, USB_SPEED_MAX))) {
		usb_log_error("Failed to reserve default address for roothub setup: %s", str_error(err));
		return err;
	}

	device_t *dev = hcd_ddf_device_create(hc, hcd->bus->device_size);
	if (!dev) {
		usb_log_error("Failed to create function for the root hub.");
		goto err_default_address;
	}

	ddf_fun_set_name(dev->fun, "roothub");

	/* Assign an address to the device */
	if ((err = bus_enumerate_device(hcd->bus, hcd, dev))) {
		usb_log_error("Failed to enumerate roothub device: %s", str_error(err));
		goto err_usb_dev;
	}

	if ((err = ddf_fun_bind(dev->fun))) {
		usb_log_error("Failed to register roothub: %s.", str_error(err));
		goto err_usb_dev;
	}

	bus_release_default_address(hcd->bus);
	return EOK;

err_usb_dev:
	hcd_ddf_device_destroy(dev);
err_default_address:
	bus_release_default_address(hcd->bus);
	return err;
}

/** Initialize hc structures.
 *
 * @param[in] device DDF instance of the device to use.
 * @param[in] max_speed Maximum supported USB speed.
 * @param[in] bw available bandwidth.
 * @param[in] bw_count Function to compute required ep bandwidth.
 *
 * @return Error code.
 * This function does all the ddf work for hc driver.
 */
int hcd_ddf_setup_hc(ddf_dev_t *device)
{
	assert(device);

	hc_dev_t *instance = ddf_dev_data_alloc(device, sizeof(hc_dev_t));
	if (instance == NULL) {
		usb_log_error("Failed to allocate HCD ddf structure.\n");
		return ENOMEM;
	}
	hcd_init(&instance->hcd);

	int ret = ENOMEM;
	instance->ctl_fun = ddf_fun_create(device, fun_exposed, "ctl");
	if (!instance->ctl_fun) {
		usb_log_error("Failed to create HCD ddf fun.\n");
		goto err_destroy_fun;
	}

	ret = ddf_fun_bind(instance->ctl_fun);
	if (ret != EOK) {
		usb_log_error("Failed to bind ctl_fun: %s.\n", str_error(ret));
		goto err_destroy_fun;
	}

	ret = ddf_fun_add_to_category(instance->ctl_fun, USB_HC_CATEGORY);
	if (ret != EOK) {
		usb_log_error("Failed to add fun to category: %s.\n",
		    str_error(ret));
		ddf_fun_unbind(instance->ctl_fun);
		goto err_destroy_fun;
	}

	/* HC should be ok at this point (except it can't do anything) */
	return EOK;

err_destroy_fun:
	ddf_fun_destroy(instance->ctl_fun);
	instance->ctl_fun = NULL;
	return ret;
}

void hcd_ddf_clean_hc(ddf_dev_t *device)
{
	assert(device);
	hc_dev_t *hc = dev_to_hc_dev(device);
	assert(hc);
	const int ret = ddf_fun_unbind(hc->ctl_fun);
	if (ret == EOK)
		ddf_fun_destroy(hc->ctl_fun);
}

//TODO: Cache parent session in HCD
/** Call the parent driver with a request to enable interrupt
 *
 * @param[in] device Device asking for interrupts
 * @param[in] inum Interrupt number
 * @return Error code.
 */
int hcd_ddf_enable_interrupt(ddf_dev_t *device, int inum)
{
	async_sess_t *parent_sess = ddf_dev_parent_sess_get(device);
	if (parent_sess == NULL)
		return EIO;

	return hw_res_enable_interrupt(parent_sess, inum);
}

//TODO: Cache parent session in HCD
int hcd_ddf_get_registers(ddf_dev_t *device, hw_res_list_parsed_t *hw_res)
{
	async_sess_t *parent_sess = ddf_dev_parent_sess_get(device);
	if (parent_sess == NULL)
		return EIO;

	hw_res_list_parsed_init(hw_res);
	const int ret = hw_res_get_list_parsed(parent_sess, hw_res, 0);
	if (ret != EOK)
		hw_res_list_parsed_clean(hw_res);
	return ret;
}

// TODO: move this someplace else
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
int hcd_ddf_setup_interrupts(ddf_dev_t *device,
    const hw_res_list_parsed_t *hw_res,
    interrupt_handler_t handler,
    irq_code_gen_t gen_irq_code)
{
	assert(device);

	hcd_t *hcd = dev_to_hcd(device);

	if (!handler || !gen_irq_code)
		return ENOTSUP;

	irq_code_t irq_code = {0};

	const int irq = gen_irq_code(&irq_code, hcd, hw_res);
	if (irq < 0) {
		usb_log_error("Failed to generate IRQ code: %s.\n",
		    str_error(irq));
		return irq;
	}

	/* Register handler to avoid interrupt lockup */
	const int irq_cap = register_interrupt_handler(device, irq, handler,
	    &irq_code);
	irq_code_clean(&irq_code);
	if (irq_cap < 0) {
		usb_log_error("Failed to register interrupt handler: %s.\n",
		    str_error(irq_cap));
		return irq_cap;
	}

	/* Enable interrupts */
	int ret = hcd_ddf_enable_interrupt(device, irq);
	if (ret != EOK) {
		usb_log_error("Failed to enable interrupts: %s.\n",
		    str_error(ret));
		unregister_interrupt_handler(device, irq_cap);
		return ret;
	}
	return irq_cap;
}

/** IRQ handling callback, forward status from call to diver structure.
 *
 * @param[in] dev DDF instance of the device to use.
 * @param[in] iid (Unused).
 * @param[in] call Pointer to the call from kernel.
 */
void ddf_hcd_gen_irq_handler(ipc_callid_t iid, ipc_call_t *call, ddf_dev_t *dev)
{
	assert(dev);
	hcd_t *hcd = dev_to_hcd(dev);
	if (!hcd || !hcd->ops.irq_hook) {
		usb_log_error("Interrupt on not yet initialized device.\n");
		return;
	}
	const uint32_t status = IPC_GET_ARG1(*call);
	hcd->ops.irq_hook(hcd, status);
}

static int interrupt_polling(void *arg)
{
	hcd_t *hcd = arg;
	assert(hcd);
	if (!hcd->ops.status_hook || !hcd->ops.irq_hook)
		return ENOTSUP;
	uint32_t status = 0;
	while (hcd->ops.status_hook(hcd, &status) == EOK) {
		hcd->ops.irq_hook(hcd, status);
		status = 0;
		/* We should wait 1 frame - 1ms here, but this polling is a
		 * lame crutch anyway so don't hog the system. 10ms is still
		 * good enough for emergency mode */
		async_usleep(10000);
	}
	return EOK;
}

/** Initialize hc and rh DDF structures and their respective drivers.
 *
 * @param device DDF instance of the device to use
 * @param speed Maximum supported speed
 * @param bw Available bandwidth (arbitrary units)
 * @param bw_count Bandwidth computing function
 * @param irq_handler IRQ handling function
 * @param gen_irq_code Function to generate IRQ pseudocode
 *                     (it needs to return used irq number)
 * @param driver_init Function to initialize HC driver
 * @param driver_fini Function to cleanup HC driver
 * @return Error code
 *
 * This function does all the preparatory work for hc and rh drivers:
 *  - gets device's hw resources
 *  - attempts to enable interrupts
 *  - registers interrupt handler
 *  - calls driver specific initialization
 *  - registers root hub
 */
int hcd_ddf_add_hc(ddf_dev_t *device, const ddf_hc_driver_t *driver)
{
	assert(driver);

	int ret = EOK;

	hw_res_list_parsed_t hw_res;
	ret = hcd_ddf_get_registers(device, &hw_res);
	if (ret != EOK) {
		usb_log_error("Failed to get register memory addresses "
		    "for `%s': %s.\n", ddf_dev_get_name(device),
		    str_error(ret));
		return ret;
	}

	ret = hcd_ddf_setup_hc(device);
	if (ret != EOK) {
		usb_log_error("Failed to setup generic HCD.\n");
		goto err_hw_res;
	}

	hcd_t *hcd = dev_to_hcd(device);

	if (driver->init)
		ret = driver->init(hcd, &hw_res, device);
	if (ret != EOK) {
		usb_log_error("Failed to init HCD.\n");
		goto err_hcd;
	}

	/* Setup interrupts  */
	interrupt_handler_t *irq_handler =
	    driver->irq_handler ? driver->irq_handler : ddf_hcd_gen_irq_handler;
	const int irq_cap = hcd_ddf_setup_interrupts(device, &hw_res,
	    irq_handler, driver->irq_code_gen);
	bool irqs_enabled = !(irq_cap < 0);
	if (irqs_enabled) {
		usb_log_debug("Hw interrupts enabled.\n");
	}

	/* Claim the device from BIOS */
	if (driver->claim)
		ret = driver->claim(hcd, device);
	if (ret != EOK) {
		usb_log_error("Failed to claim `%s' for driver `%s': %s",
		    ddf_dev_get_name(device), driver->name, str_error(ret));
		goto err_irq;
	}

	/* Start hw driver */
	if (driver->start)
		ret = driver->start(hcd, irqs_enabled);
	if (ret != EOK) {
		usb_log_error("Failed to start HCD: %s.\n", str_error(ret));
		goto err_irq;
	}

	/* Need working irq replacement to setup root hub */
	if (!irqs_enabled && hcd->ops.status_hook) {
		hcd->polling_fibril = fibril_create(interrupt_polling, hcd);
		if (hcd->polling_fibril == 0) {
			usb_log_error("Failed to create polling fibril\n");
			ret = ENOMEM;
			goto err_started;
		}
		fibril_add_ready(hcd->polling_fibril);
		usb_log_warning("Failed to enable interrupts: %s."
		    " Falling back to polling.\n", str_error(irq_cap));
	}

	/*
	 * Creating root hub registers a new USB device so HC
	 * needs to be ready at this time.
	 */
	if (driver->setup_root_hub)
		ret = driver->setup_root_hub(hcd, device);
	if (ret != EOK) {
		usb_log_error("Failed to setup HC root hub: %s.\n",
		    str_error(ret));
		goto err_polling;
	}

	usb_log_info("Controlling new `%s' device `%s'.\n",
	    driver->name, ddf_dev_get_name(device));
	return EOK;

err_polling:
	// TODO: Stop the polling fibril (refactor the interrupt_polling func)
	//
err_started:
	if (driver->stop)
		driver->stop(hcd);
err_irq:
	unregister_interrupt_handler(device, irq_cap);
	if (driver->fini)
		driver->fini(hcd);
err_hcd:
	hcd_ddf_clean_hc(device);
err_hw_res:
	hw_res_list_parsed_clean(&hw_res);
	return ret;
}

/**
 * @}
 */
