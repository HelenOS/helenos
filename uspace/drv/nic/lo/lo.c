/*
 * Copyright (c) 2011 Radim Vansa
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

/**
 * @addtogroup drv_lo
 * @brief Loopback virtual device driver
 * @{
 */
/**
 * @file
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <async.h>
#include <nic.h>
#include <packet_client.h>

#define NAME  "lo"

static nic_address_t lo_addr = {
	.address = {0, 0, 0, 0, 0, 0}
};

static ddf_dev_ops_t lo_dev_ops;

static nic_device_info_t lo_info = {
	.vendor_name = "HelenOS",
	.model_name = "loopback",
	.part_number = "N/A (virtual device)",
	.serial_number = "N/A (virtual device)"
};

static void lo_write_packet(nic_t *nic_data, packet_t *packet)
{
	nic_report_send_ok(nic_data, 1, packet_get_data_length(packet));
	nic_received_noneth_packet(nic_data, packet);
}

static int lo_set_address(ddf_fun_t *fun, const nic_address_t *address)
{
	printf("%s: Set loopback HW to " PRIMAC "\n", NAME,
	    ARGSMAC(address->address));
	return ENOTSUP;
}

static int lo_get_device_info(ddf_fun_t *fun, nic_device_info_t *info)
{
	assert(info);
	memcpy(info, &lo_info, sizeof(nic_device_info_t));
	return EOK;
}

static int lo_add_device(ddf_dev_t *dev)
{
	nic_t *nic_data = nic_create_and_bind(dev);
	if (nic_data == NULL) {
		printf("%s: Failed to initialize\n", NAME);
		return ENOMEM;
	}
	
	dev->driver_data = nic_data;
	nic_set_write_packet_handler(nic_data, lo_write_packet);
	
	int rc = nic_connect_to_services(nic_data);
	if (rc != EOK) {
		printf("%s: Failed to connect to services\n", NAME);
		nic_unbind_and_destroy(dev);
		return rc;
	}
	
	rc = nic_register_as_ddf_fun(nic_data, &lo_dev_ops);
	if (rc != EOK) {
		printf("%s: Failed to register as DDF function\n", NAME);
		nic_unbind_and_destroy(dev);
		return rc;
	}
	
	rc = nic_report_address(nic_data, &lo_addr);
	if (rc != EOK) {
		printf("%s: Failed to setup loopback address\n", NAME);
		nic_unbind_and_destroy(dev);
		return rc;
	}
	
	printf("%s: Adding loopback device '%s'\n", NAME, dev->name);
	return EOK;
}

static nic_iface_t lo_nic_iface;

static driver_ops_t lo_driver_ops = {
	.add_device = lo_add_device,
};

static driver_t lo_driver = {
	.name = NAME,
	.driver_ops = &lo_driver_ops
};

int main(int argc, char *argv[])
{
	nic_driver_init(NAME);
	nic_driver_implement(&lo_driver_ops, &lo_dev_ops, &lo_nic_iface);
	lo_nic_iface.set_address = lo_set_address;
	lo_nic_iface.get_device_info = lo_get_device_info;
	
	return ddf_driver_main(&lo_driver);
}

/** @}
 */
