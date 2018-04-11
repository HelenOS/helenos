/*
 * Copyright (c) 2013 Jan Vesely
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
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver
 */

#include <assert.h>
#include <errno.h>
#include <mem.h>
#include <str_error.h>
#include <stdint.h>
#include <stddef.h>

#include <usb/classes/hub.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include <usb/request.h>
#include <usb/usb.h>

#include <usb/host/endpoint.h>
#include <usbvirt/device.h>

#include "ehci_rh.h"

enum {
	HUB_STATUS_CHANGE_PIPE = 1,
};

static usbvirt_device_ops_t ops;

/** Initialize internal USB HUB class descriptor.
 * @param instance EHCI root hub.
 * Use register based info to create accurate descriptor.
 */
static void ehci_rh_hub_desc_init(ehci_rh_t *instance, unsigned hcs)
{
	assert(instance);
	const unsigned dsize = sizeof(usb_hub_descriptor_header_t) +
	    STATUS_BYTES(instance->port_count) * 2;
	assert(dsize <= sizeof(instance->hub_descriptor));

	instance->hub_descriptor.header.length = dsize;
	instance->hub_descriptor.header.descriptor_type = USB_DESCTYPE_HUB;
	instance->hub_descriptor.header.port_count = instance->port_count;
	/* Bits 0,1 indicate power switching mode
	 * Bit 2 indicates device type (compound device)
	 * Bits 3,4 indicate over-current protection mode */
	instance->hub_descriptor.header.characteristics = 0 |
	    ((hcs & EHCI_CAPS_HCS_PPC_FLAG) ? 0x09 : 0x12) |
	    ((hcs & EHCI_CAPS_HCS_INDICATORS_FLAG) ? 0x80 : 0) |
	    (0x3 << 5); /* Need 32 FS bit times */ // TODO Implement
	instance->hub_descriptor.header.characteristics_reserved = 0;
	instance->hub_descriptor.header.power_good_time = 50;
	/* bHubContrCurrent, root hubs don't need no power. */
	instance->hub_descriptor.header.max_current = 0;

	/* Device removable and some legacy 1.0 stuff*/
	instance->hub_descriptor.rempow[0] = 0xff;
	instance->hub_descriptor.rempow[1] = 0xff;
	instance->hub_descriptor.rempow[2] = 0xff;
	instance->hub_descriptor.rempow[3] = 0xff;

}
/** Initialize EHCI root hub.
 * @param instance Place to initialize.
 * @param regs EHCI device registers.
 * @param name Device name.
 * return Error code, EOK on success.
 *
 * Selects preconfigured port powering mode, sets up descriptor, and
 * initializes internal virtual hub.
 */
errno_t ehci_rh_init(ehci_rh_t *instance, ehci_caps_regs_t *caps, ehci_regs_t *regs,
    fibril_mutex_t *guard, const char *name)
{
	assert(instance);
	instance->registers = regs;
	instance->port_count =
	    (EHCI_RD(caps->hcsparams) >> EHCI_CAPS_HCS_N_PORTS_SHIFT) &
	    EHCI_CAPS_HCS_N_PORTS_MASK;
	usb_log_debug2("RH(%p): hcsparams: %x.", instance,
	    EHCI_RD(caps->hcsparams));
	usb_log_info("RH(%p): Found %u ports.", instance,
	    instance->port_count);

	if (EHCI_RD(caps->hcsparams) & EHCI_CAPS_HCS_PPC_FLAG) {
		usb_log_info("RH(%p): Per-port power switching.", instance);
	} else {
		usb_log_info("RH(%p): No power switching.", instance);
	}
	for (unsigned i = 0; i < instance->port_count; ++i)
		usb_log_debug2("RH(%p-%u): status: %" PRIx32, instance, i,
		    EHCI_RD(regs->portsc[i]));

	for (unsigned i = 0; i < EHCI_MAX_PORTS; ++i) {
		instance->reset_flag[i] = false;
		instance->resume_flag[i] = false;
	}

	ehci_rh_hub_desc_init(instance, EHCI_RD(caps->hcsparams));
	instance->guard = guard;
	instance->status_change_endpoint = NULL;

	return virthub_base_init(&instance->base, name, &ops, instance,
	    NULL, &instance->hub_descriptor.header, HUB_STATUS_CHANGE_PIPE);
}

/** Schedule USB request.
 * @param instance OCHI root hub instance.
 * @param batch USB requst batch to schedule.
 * @return Always EOK.
 * Most requests complete even before this function returns,
 * status change requests might be postponed until there is something to report.
 */
errno_t ehci_rh_schedule(ehci_rh_t *instance, usb_transfer_batch_t *batch)
{
	assert(instance);
	assert(batch);
	batch->error = virthub_base_request(&instance->base, batch->target,
	    batch->dir, (void *) batch->setup.buffer,
	    batch->dma_buffer.virt, batch->size,
	    &batch->transferred_size);
	if (batch->error == ENAK) {
		usb_log_debug("RH(%p): BATCH(%p) adding as unfinished",
		    instance, batch);

		/* Lock the HC guard */
		fibril_mutex_lock(instance->guard);
		const int err = endpoint_activate_locked(batch->ep, batch);
		if (err) {
			fibril_mutex_unlock(batch->ep->guard);
			return err;
		}

		/*
		 * Asserting that the HC do not run two instances of the status
		 * change endpoint - shall be true.
		 */
		assert(!instance->status_change_endpoint);

		endpoint_add_ref(batch->ep);
		instance->status_change_endpoint = batch->ep;
		fibril_mutex_unlock(instance->guard);
	} else {
		usb_log_debug("RH(%p): BATCH(%p) virtual request complete: %s",
		    instance, batch, str_error(batch->error));
		usb_transfer_batch_finish(batch);
	}
	return EOK;
}

/** Handle EHCI RHSC interrupt.
 * @param instance EHCI root hub isntance.
 * @return Always EOK.
 *
 * Interrupt means there is a change of status to report. It may trigger
 * processing of a postponed request.
 */
errno_t ehci_rh_interrupt(ehci_rh_t *instance)
{
	fibril_mutex_lock(instance->guard);
	endpoint_t *ep = instance->status_change_endpoint;
	if (!ep) {
		fibril_mutex_unlock(instance->guard);
		return EOK;
	}

	usb_transfer_batch_t *const batch = ep->active_batch;
	endpoint_deactivate_locked(ep);
	instance->status_change_endpoint = NULL;
	fibril_mutex_unlock(instance->guard);

	endpoint_del_ref(ep);

	if (batch) {
		usb_log_debug2("RH(%p): Interrupt. Processing batch: %p",
		    instance, batch);
		batch->error = virthub_base_request(&instance->base, batch->target,
		    batch->dir, (void *) batch->setup.buffer,
		    batch->dma_buffer.virt, batch->size,
		    &batch->transferred_size);
		usb_transfer_batch_finish(batch);
	}
	return EOK;
}

/* HUB ROUTINES IMPLEMENTATION */
#define TEST_SIZE_INIT(size, port, hub) \
do { \
	hub = virthub_get_data(device); \
	assert(hub);\
	if (uint16_usb2host(setup_packet->length) != size) \
		return ESTALL; \
	port = uint16_usb2host(setup_packet->index) - 1; \
	if (port > hub->port_count) \
		return EINVAL; \
} while (0)

/** Hub status request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static errno_t req_get_status(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	ehci_rh_t *hub = virthub_get_data(device);
	assert(hub);
	if (uint16_usb2host(setup_packet->length) != 4)
		return ESTALL;
	/* ECHI RH does not report global OC, and local power is always good */
	const uint32_t val = 0;
	memcpy(data, &val, sizeof(val));
	*act_size = sizeof(val);
	return EOK;
}

/** Hub set feature request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static errno_t req_clear_hub_feature(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	ehci_rh_t *hub = virthub_get_data(device);
	assert(hub);

	/*
	 * Chapter 11.16.2 specifies that only C_HUB_LOCAL_POWER and
	 * C_HUB_OVER_CURRENT are supported.
	 * C_HUB_LOCAL_POWER is not supported because root hubs do not support
	 * local power status feature.
	 * EHCI RH does not report global OC condition either
	 */
	return ESTALL;
}

#define BIT_VAL(val, bit)   ((val & bit) ? 1 : 0)
#define EHCI2USB(val, bit, mask)   (BIT_VAL(val, bit) ? mask : 0)

/** Port status request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static errno_t req_get_port_status(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	ehci_rh_t *hub;
	unsigned port;
	TEST_SIZE_INIT(4, port, hub);
	if (setup_packet->value != 0)
		return EINVAL;

	const uint32_t reg = EHCI_RD(hub->registers->portsc[port]);
	const uint32_t status = uint32_host2usb(
	    EHCI2USB(reg, USB_PORTSC_CONNECT_FLAG, USB_HUB_PORT_STATUS_CONNECTION) |
	    EHCI2USB(reg, USB_PORTSC_ENABLED_FLAG, USB_HUB_PORT_STATUS_ENABLE) |
	    EHCI2USB(reg, USB_PORTSC_SUSPEND_FLAG, USB2_HUB_PORT_STATUS_SUSPEND) |
	    EHCI2USB(reg, USB_PORTSC_OC_ACTIVE_FLAG, USB_HUB_PORT_STATUS_OC) |
	    EHCI2USB(reg, USB_PORTSC_PORT_RESET_FLAG, USB_HUB_PORT_STATUS_RESET) |
	    EHCI2USB(reg, USB_PORTSC_PORT_POWER_FLAG, USB2_HUB_PORT_STATUS_POWER) |
	    (((reg & USB_PORTSC_LINE_STATUS_MASK) == USB_PORTSC_LINE_STATUS_K) ?
	    (USB2_HUB_PORT_STATUS_LOW_SPEED) : 0) |
	    ((reg & USB_PORTSC_PORT_OWNER_FLAG) ? 0 : USB2_HUB_PORT_STATUS_HIGH_SPEED) |
	    EHCI2USB(reg, USB_PORTSC_PORT_TEST_MASK, USB2_HUB_PORT_STATUS_TEST) |
	    EHCI2USB(reg, USB_PORTSC_INDICATOR_MASK, USB2_HUB_PORT_STATUS_INDICATOR) |
	    EHCI2USB(reg, USB_PORTSC_CONNECT_CH_FLAG, USB_HUB_PORT_STATUS_C_CONNECTION) |
	    EHCI2USB(reg, USB_PORTSC_EN_CHANGE_FLAG, USB2_HUB_PORT_STATUS_C_ENABLE) |
	    (hub->resume_flag[port] ? USB2_HUB_PORT_STATUS_C_SUSPEND : 0) |
	    EHCI2USB(reg, USB_PORTSC_OC_CHANGE_FLAG, USB_HUB_PORT_STATUS_C_OC) |
	    (hub->reset_flag[port] ? USB_HUB_PORT_STATUS_C_RESET : 0));
	/* Note feature numbers for test and indicator feature do not
	 * correspond to the port status bit locations */
	usb_log_debug2("RH(%p-%u) port status: %" PRIx32 "(%" PRIx32 ")", hub, port,
	    status, reg);
	memcpy(data, &status, sizeof(status));
	*act_size = sizeof(status);
	return EOK;
}

typedef struct {
	ehci_rh_t *hub;
	unsigned port;
} ehci_rh_job_t;

static errno_t stop_reset(void *arg)
{
	ehci_rh_job_t *job = arg;
	async_usleep(50000);
	usb_log_debug("RH(%p-%u): Clearing reset", job->hub, job->port);
	EHCI_CLR(job->hub->registers->portsc[job->port],
	    USB_PORTSC_PORT_RESET_FLAG);
	/* wait for reset to complete */
	while (EHCI_RD(job->hub->registers->portsc[job->port]) &
	    USB_PORTSC_PORT_RESET_FLAG) {
		async_usleep(1);
	}
	usb_log_debug("RH(%p-%u): Reset complete", job->hub, job->port);
	/* Handle port ownership, if the port is not enabled
	 * after reset it's a full speed device */
	if (!(EHCI_RD(job->hub->registers->portsc[job->port]) &
	    USB_PORTSC_ENABLED_FLAG)) {
		usb_log_info("RH(%p-%u): Port not enabled after reset (%" PRIX32
		    "), giving up ownership", job->hub, job->port,
		    EHCI_RD(job->hub->registers->portsc[job->port]));
		EHCI_SET(job->hub->registers->portsc[job->port],
		    USB_PORTSC_PORT_OWNER_FLAG);
	}
	job->hub->reset_flag[job->port] = true;
	ehci_rh_interrupt(job->hub);
	free(job);
	return 0;
}

static errno_t stop_resume(void *arg)
{
	ehci_rh_job_t *job = arg;
	async_usleep(20000);
	usb_log_debug("RH(%p-%u): Stopping resume", job->hub, job->port);
	EHCI_CLR(job->hub->registers->portsc[job->port],
	    USB_PORTSC_RESUME_FLAG);
	job->hub->resume_flag[job->port] = true;
	ehci_rh_interrupt(job->hub);
	free(job);
	return 0;
}

static errno_t delayed_job(errno_t (*func)(void *), ehci_rh_t *rh, unsigned port)
{
	ehci_rh_job_t *job = malloc(sizeof(*job));
	if (!job)
		return ENOMEM;
	job->hub = rh;
	job->port = port;
	fid_t fib = fibril_create(func, job);
	if (!fib) {
		free(job);
		return ENOMEM;
	}
	fibril_add_ready(fib);
	usb_log_debug2("RH(%p-%u): Scheduled delayed stop job.", rh, port);
	return EOK;
}


/** Port clear feature request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static errno_t req_clear_port_feature(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	ehci_rh_t *hub;
	unsigned port;
	TEST_SIZE_INIT(0, port, hub);
	const unsigned feature = uint16_usb2host(setup_packet->value);
	/* Enabled features to clear: see page 269 of USB specs */
	switch (feature) {
	case USB_HUB_FEATURE_PORT_POWER:          /*8*/
		usb_log_debug2("RH(%p-%u): Clear port power.", hub, port);
		EHCI_CLR(hub->registers->portsc[port],
		    USB_PORTSC_PORT_POWER_FLAG);
		return EOK;

	case USB2_HUB_FEATURE_PORT_ENABLE:         /*1*/
		usb_log_debug2("RH(%p-%u): Clear port enable.", hub, port);
		EHCI_CLR(hub->registers->portsc[port],
		    USB_PORTSC_ENABLED_FLAG);
		return EOK;

	case USB2_HUB_FEATURE_PORT_SUSPEND:        /*2*/
		usb_log_debug2("RH(%p-%u): Clear port suspend.", hub, port);
		/* If not in suspend it's noop */
		if ((EHCI_RD(hub->registers->portsc[port]) &
		    USB_PORTSC_SUSPEND_FLAG) == 0)
			return EOK;
		/* Host driven resume */
		EHCI_SET(hub->registers->portsc[port],
		    USB_PORTSC_RESUME_FLAG);
		//TODO: What if creating the delayed job fails?
		return delayed_job(stop_resume, hub, port);

	case USB_HUB_FEATURE_C_PORT_CONNECTION:   /*16*/
		usb_log_debug2("RH(%p-%u): Clear port connection change.",
		    hub, port);
		EHCI_SET(hub->registers->portsc[port],
		    USB_PORTSC_CONNECT_CH_FLAG);
		return EOK;
	case USB2_HUB_FEATURE_C_PORT_ENABLE:       /*17*/
		usb_log_debug2("RH(%p-%u): Clear port enable change.",
		    hub, port);
		EHCI_SET(hub->registers->portsc[port],
		    USB_PORTSC_CONNECT_CH_FLAG);
		return EOK;
	case USB_HUB_FEATURE_C_PORT_OVER_CURRENT: /*19*/
		usb_log_debug2("RH(%p-%u): Clear port OC change.",
		    hub, port);
		EHCI_SET(hub->registers->portsc[port],
		    USB_PORTSC_OC_CHANGE_FLAG);
		return EOK;
	case USB2_HUB_FEATURE_C_PORT_SUSPEND:      /*18*/
		usb_log_debug2("RH(%p-%u): Clear port suspend change.",
		    hub, port);
		hub->resume_flag[port] = false;
		return EOK;
	case USB_HUB_FEATURE_C_PORT_RESET:        /*20*/
		usb_log_debug2("RH(%p-%u): Clear port reset change.",
		    hub, port);
		hub->reset_flag[port] = false;
		return EOK;

	default:
		usb_log_warning("RH(%p-%u): Clear unknown feature: %u",
		    hub, port, feature);
		return ENOTSUP;
	}
}

/** Port set feature request handler.
 * @param device Virtual hub device
 * @param setup_packet USB setup stage data.
 * @param[out] data destination data buffer, size must be at least
 *             setup_packet->length bytes
 * @param[out] act_size Sized of the valid response part of the buffer.
 * @return Error code.
 */
static errno_t req_set_port_feature(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	ehci_rh_t *hub;
	unsigned port;
	TEST_SIZE_INIT(0, port, hub);
	const unsigned feature = uint16_usb2host(setup_packet->value);
	switch (feature) {
	case USB2_HUB_FEATURE_PORT_ENABLE:  /*1*/
		usb_log_debug2("RH(%p-%u): Set port enable.", hub, port);
		EHCI_SET(hub->registers->portsc[port],
		    USB_PORTSC_ENABLED_FLAG);
		return EOK;
	case USB2_HUB_FEATURE_PORT_SUSPEND: /*2*/
		usb_log_debug2("RH(%p-%u): Set port suspend.", hub, port);
		EHCI_SET(hub->registers->portsc[port],
		    USB_PORTSC_SUSPEND_FLAG);
		return EOK;
	case USB_HUB_FEATURE_PORT_RESET:   /*4*/
		usb_log_debug2("RH(%p-%u): Set port reset.", hub, port);
		EHCI_SET(hub->registers->portsc[port],
		    USB_PORTSC_PORT_RESET_FLAG);
		//TODO: What if creating the delayed job fails?
		return delayed_job(stop_reset, hub, port);
	case USB_HUB_FEATURE_PORT_POWER:   /*8*/
		usb_log_debug2("RH(%p-%u): Set port power.", hub, port);
		EHCI_SET(hub->registers->portsc[port],
		    USB_PORTSC_PORT_POWER_FLAG);
		return EOK;
	default:
		usb_log_warning("RH(%p-%u): Set unknown feature: %u",
		    hub, port, feature);
		return ENOTSUP;
	}
}

/** Status change handler.
 * @param device Virtual hub device
 * @param endpoint Endpoint number
 * @param tr_type Transfer type
 * @param buffer Response destination
 * @param buffer_size Bytes available in buffer
 * @param actual_size Size us the used part of the dest buffer.
 *
 * Produces status mask. Bit 0 indicates hub status change the other bits
 * represent port status change. Endian does not matter as UHCI root hubs
 * only need 1 byte.
 */
static errno_t req_status_change_handler(usbvirt_device_t *device,
    usb_endpoint_t endpoint, usb_transfer_type_t tr_type,
    void *buffer, size_t buffer_size, size_t *actual_size)
{
	ehci_rh_t *hub = virthub_get_data(device);
	assert(hub);

	if (buffer_size < STATUS_BYTES(hub->port_count))
		return ESTALL;

	uint16_t mask = 0;
	for (unsigned port = 0; port < hub->port_count; ++port) {
		/* Write-clean bits are those that indicate change */
		uint32_t status = EHCI_RD(hub->registers->portsc[port]);
		if ((status & USB_PORTSC_WC_MASK) || hub->reset_flag[port]) {
			/* Ignore new LS device */
			if ((status & USB_PORTSC_CONNECT_CH_FLAG) &&
			    (status & USB_PORTSC_LINE_STATUS_MASK) ==
			    USB_PORTSC_LINE_STATUS_K)
				EHCI_SET(hub->registers->portsc[port],
				    USB_PORTSC_PORT_OWNER_FLAG);
			else
				mask |= (2 << port);
		}
	}

	usb_log_debug2("RH(%p): root hub interrupt mask: %" PRIx16, hub, mask);

	if (mask == 0)
		return ENAK;
	mask = uint16_host2usb(mask);
	memcpy(buffer, &mask, STATUS_BYTES(hub->port_count));
	*actual_size = STATUS_BYTES(hub->port_count);
	return EOK;
}

/** EHCI root hub request handlers */
static const usbvirt_control_request_handler_t control_transfer_handlers[] = {
	{
		STD_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_DEVREQ_GET_DESCRIPTOR),
		.name = "GetDescriptor",
		.callback = virthub_base_get_hub_descriptor,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_DEVREQ_GET_DESCRIPTOR),
		.name = "GetDescriptor",
		.callback = virthub_base_get_hub_descriptor,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_GET_DESCRIPTOR),
		.name = "GetHubDescriptor",
		.callback = virthub_base_get_hub_descriptor,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetPortStatus",
		.callback = req_get_port_status,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_CLEAR_FEATURE),
		.name = "ClearHubFeature",
		.callback = req_clear_hub_feature,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_CLEAR_FEATURE),
		.name = "ClearPortFeature",
		.callback = req_clear_port_feature,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetHubStatus",
		.callback = req_get_status,
	},
	{
		CLASS_REQ_IN(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_GET_STATUS),
		.name = "GetPortStatus",
		.callback = req_get_port_status,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_DEVICE, USB_HUB_REQUEST_SET_FEATURE),
		.name = "SetHubFeature",
		.callback = req_nop,
	},
	{
		CLASS_REQ_OUT(USB_REQUEST_RECIPIENT_OTHER, USB_HUB_REQUEST_SET_FEATURE),
		.name = "SetPortFeature",
		.callback = req_set_port_feature,
	},
	{
		.callback = NULL
	}
};

/** Virtual EHCI root hub ops */
static usbvirt_device_ops_t ops = {
	.control = control_transfer_handlers,
	.data_in[HUB_STATUS_CHANGE_PIPE] = req_status_change_handler,
};
