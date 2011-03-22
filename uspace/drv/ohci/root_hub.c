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
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */
#include <assert.h>
#include <errno.h>
#include <str_error.h>

#include <usb/debug.h>

#include "root_hub.h"
#include <usb/request.h>
#include <usb/classes/hub.h>


/** Root hub initialization
 * @return Error code.
 */
int rh_init(rh_t *instance, ddf_dev_t *dev, ohci_regs_t *regs)
{
	assert(instance);
	instance->address = -1;
	instance->registers = regs;
	instance->device = dev;


	usb_log_info("OHCI root hub with %d ports.\n", regs->rh_desc_a & 0xff);

	//start generic usb hub driver
	
	/* TODO: implement */
	return EOK;
}
/*----------------------------------------------------------------------------*/


static int process_get_port_status_request(rh_t *instance, uint16_t port,
		char * buffer){
	if(port<1 || port>instance->port_count)
		return EINVAL;
	uint32_t * uint32_buffer = (uint32_t*)buffer;
	uint32_buffer[0] = instance->registers->rh_port_status[port -1];
	return EOK;
}

static int process_get_hub_status_request(rh_t *instance,char * buffer){
	uint32_t * uint32_buffer = (uint32_t*)buffer;
	//bits, 0,1,16,17
	uint32_t mask = 1 & (1<<1) & (1<<16) & (1<<17);
	uint32_buffer[0] = mask & instance->registers->rh_status;
	return EOK;

}


static int process_get_status_request(rh_t *instance,char * buffer,
		size_t buffer_size, usb_device_request_setup_packet_t * request){

	usb_hub_bm_request_type_t request_type = request->request_type;
	if(buffer_size!=4) return EINVAL;
	if(request_type == USB_HUB_REQ_TYPE_GET_HUB_STATUS)
		return process_get_hub_status_request(instance, buffer);
	if(request_type == USB_HUB_REQ_TYPE_GET_PORT_STATUS)
		return process_get_port_status_request(instance, request->index,
				buffer);
	return ENOTSUP;
}

static int process_get_descriptor_request(rh_t *instance,
		usb_transfer_batch_t *request){
	/// \TODO
	usb_device_request_setup_packet_t * setup_request =
			(usb_device_request_setup_packet_t*)request->setup_buffer;
	if(setup_request->value == USB_DESCTYPE_HUB){
		//create hub descriptor
	}else if(setup_request->value == USB_DESCTYPE_HUB){
		//create std device descriptor
	}else{
		return EINVAL;
	}
	return EOK;
}

static int process_get_configuration_request(rh_t *instance, char * buffer,
	size_t buffer_size
){
	//set and get configuration requests do not have any meaning, only dummy
	//values are returned
	if(buffer_size != 1)
		return EINVAL;
	buffer[0] = 1;
	return EOK;
}

static int process_hub_feature_set_request(rh_t *instance,
		uint16_t feature, bool enable){
	if(feature > USB_HUB_FEATURE_C_HUB_OVER_CURRENT)
		return EINVAL;
	instance->registers->rh_status =
			enable ?
			(instance->registers->rh_status | (1<<feature))
			:
			(instance->registers->rh_status & (~(1<<feature)));
	/// \TODO any error?
	return EOK;
}

static int process_port_feature_set_request(rh_t *instance,
		uint16_t feature, uint16_t port, bool enable){
	if(feature > USB_HUB_FEATURE_C_PORT_RESET)
		return EINVAL;
	if(port<1 || port>instance->port_count)
		return EINVAL;
	instance->registers->rh_port_status[port - 1] =
			enable ?
			(instance->registers->rh_port_status[port - 1] | (1<<feature))
			:
			(instance->registers->rh_port_status[port - 1] & (~(1<<feature)));
	/// \TODO any error?
	return EOK;

}

static int process_request_with_output(rh_t *instance,
		usb_transfer_batch_t *request){
	usb_device_request_setup_packet_t * setup_request =
			(usb_device_request_setup_packet_t*)request->setup_buffer;
	if(setup_request->request == USB_DEVREQ_GET_STATUS){
		return process_get_status_request(instance, request->buffer,
				request->buffer_size,
				setup_request);
	}
	if(setup_request->request == USB_DEVREQ_GET_DESCRIPTOR){
		return process_get_descriptor_request(instance, request);
	}
	if(setup_request->request == USB_DEVREQ_GET_CONFIGURATION){
		return process_get_configuration_request(instance, request->buffer,
				request->buffer_size);
	}
	return ENOTSUP;
}

static int process_request_with_input(rh_t *instance,
		usb_transfer_batch_t *request){
	usb_device_request_setup_packet_t * setup_request =
			(usb_device_request_setup_packet_t*)request->setup_buffer;
	if(setup_request->request == USB_DEVREQ_SET_DESCRIPTOR){
		return ENOTSUP;
	}
	if(setup_request->request == USB_DEVREQ_SET_CONFIGURATION){
		//set and get configuration requests do not have any meaning,
		//only dummy values are returned
		return EOK;
	}
	return ENOTSUP;
}


static int process_request_without_data(rh_t *instance,
		usb_transfer_batch_t *request){
	usb_device_request_setup_packet_t * setup_request =
			(usb_device_request_setup_packet_t*)request->setup_buffer;
	if(setup_request->request_type == USB_HUB_REQ_TYPE_SET_HUB_FEATURE){
		return process_hub_feature_set_request(instance, setup_request->value,
				setup_request->request == USB_DEVREQ_SET_FEATURE);
	}
	if(setup_request->request_type == USB_HUB_REQ_TYPE_SET_PORT_FEATURE){
		return process_port_feature_set_request(instance, setup_request->value,
				setup_request->index,
				setup_request->request == USB_DEVREQ_SET_FEATURE);
	}
	return ENOTSUP;
}


/**
 *
 * @param instance
 * @param request
 * @return
 */
int rh_request(rh_t *instance, usb_transfer_batch_t *request)
{
	assert(instance);
	assert(request);
	int opResult;
	if (request->setup_buffer) {
		usb_log_info("Root hub got SETUP packet: %s.\n",
		    usb_debug_str_buffer((const uint8_t *)request->setup_buffer, 8, 8));
		if(sizeof(usb_device_request_setup_packet_t)>request->setup_size){
			usb_log_error("setup packet too small\n");
			return EINVAL;
		}
		usb_device_request_setup_packet_t * setup_request =
				(usb_device_request_setup_packet_t*)request->setup_buffer;
		if(
			setup_request->request == USB_DEVREQ_GET_STATUS
			|| setup_request->request == USB_DEVREQ_GET_DESCRIPTOR
			|| setup_request->request == USB_DEVREQ_GET_CONFIGURATION
		){
			usb_log_debug("processing request with output\n");
			opResult = process_request_with_output(instance,request);
		}else if(
			setup_request->request == USB_DEVREQ_CLEAR_FEATURE
			|| setup_request->request == USB_DEVREQ_SET_FEATURE
		){
			usb_log_debug("processing request without additional data\n");
			opResult = process_request_without_data(instance,request);
		}else if(setup_request->request == USB_DEVREQ_SET_DESCRIPTOR
				|| setup_request->request == USB_DEVREQ_SET_CONFIGURATION
		){
			usb_log_debug("processing request with input\n");
			opResult = process_request_with_input(instance,request);
		}else{
			usb_log_warning("received unsuported request: %d\n",
					setup_request->request
					);
			opResult = ENOTSUP;
		}
	}else{
		usb_log_error("root hub received empty transaction?");
		opResult = EINVAL;
	}
	usb_transfer_batch_finish(request, opResult);
	return EOK;
}
/*----------------------------------------------------------------------------*/
//is this status change?
void rh_interrupt(rh_t *instance)
{
	usb_log_error("Root hub interrupt not implemented.\n");
	/* TODO: implement */
}
/**
 * @}
 */
