/*
 * Copyright (c) 2014 Agnieszka Tabaka
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

/* XXX Fix this */
#define _DDF_DATA_IMPLANT

#include <assert.h>
#include <errno.h>
#include <align.h>
#include <byteorder.h>
#include <libarch/barrier.h>

#include <as.h>
#include <ddf/log.h>
#include <ddf/interrupt.h>
#include <io/log.h>
#include <nic.h>
#include <pci_dev_iface.h>

#include <ipc/irc.h>
#include <sysinfo.h>
#include <ipc/ns.h>

#include <str.h>

#include "defs.h"
#include "driver.h"

/** Global mutex for work with shared irq structure */
FIBRIL_MUTEX_INITIALIZE(irq_reg_lock);

/** Network interface options for RTL8169 card driver */
static nic_iface_t rtl8169_nic_iface = {
	.set_address = &rtl8169_set_addr,
	.get_device_info = &rtl8169_get_device_info,
	.get_cable_state = &rtl8169_get_cable_state,
	.get_operation_mode = &rtl8169_get_operation_mode,
	.set_operation_mode = &rtl8169_set_operation_mode,

	.get_pause = &rtl8169_pause_get,
	.set_pause = &rtl8169_pause_set,

	.autoneg_enable = &rtl8169_autoneg_enable,
	.autoneg_disable = &rtl8169_autoneg_disable,
	.autoneg_probe = &rtl8169_autoneg_probe,
	.autoneg_restart = &rtl8169_autoneg_restart,

	.defective_get_mode = &rtl8169_defective_get_mode,
	.defective_set_mode = &rtl8169_defective_set_mode,
};

/** Basic device operations for RTL8169 driver */
static ddf_dev_ops_t rtl8169_dev_ops;

static int rtl8169_dev_add(ddf_dev_t *dev);

/** Basic driver operations for RTL8169 driver */
static driver_ops_t rtl8169_driver_ops = {
	.dev_add = &rtl8169_dev_add,
};

/** Driver structure for RTL8169 driver */
static driver_t rtl8169_driver = {
	.name = NAME,
	.driver_ops = &rtl8169_driver_ops
};

static int rtl8169_dev_add(ddf_dev_t *dev)
{
	ddf_fun_t *fun;
	int rc;

	assert(dev);
	ddf_msg(LVL_NOTE, "RTL8169_dev_add %s (handle = %zu)",
	    ddf_dev_get_name(dev), ddf_dev_get_handle(dev));

	/* Init structures */
	rc = rtl8169_dev_initialize(dev);
	if (rc != EOK)
		return rc;

	return EOK;
}

static int rtl8169_set_addr(ddf_fun_t *fun, const nic_address_t *addr)
{
	return EOK;
}

static int rtl8169_get_device_info(ddf_fun_t *fun, nic_device_info_t *info)
{
	return EOK;
}

static int rtl8169_get_cable_state(ddf_fun_t *fun, nic_cable_state_t *state)
{
	return EOK;
}

static int rtl8169_get_operation_mode(ddf_fun_t *fun, int *speed,
    nic_channel_mode_t *duplex, nic_role_t *role)
{
	return EOK;
}

static int rtl8169_set_operation_mode(ddf_fun_t *fun, int speed,
    nic_channel_mode_t duplex, nic_role_t role)
{
	return EOK;
}

static int rtl8169_pause_get(ddf_fun_t *fun, nic_result_t *we_send, 
    nic_result_t *we_receive, uint16_t *time)
{
	return EOK;
{

static int rtl8169_pause_set(ddf_fun_t *fun, int allow_send, int allow_receive, 
    uint16_t time)
{
	return EOK;
}

static int rtl8169_autoneg_enable(ddf_fun_t *fun, uint32_t advertisement)
{
	return EOK;
}

static int rtl8169_autoneg_disable(ddf_fun_t *fun)
{
	return EOK;
}

static int rtl8169_autoneg_probe(ddf_fun_t *fun, uint32_t *advertisement,
    uint32_t *their_adv, nic_result_t *result, nic_result_t *their_result)
{
	return EOK;
}

static int rtl8169_autoneg_restart(ddf_fun_t *fun)
{
	return EOK;
}

static int rtl8169_defective_get_mode(ddf_fun_t *fun, uint32_t *mode)
{
	return EOK;
}

static int rtl8169_defective_set_mode(ddf_fun_t *fun, uint32_t mode)
{
	return EOK;
}

static void rtl8169_send_frame(nic_t *nic_data, void *data, size_t size)
{

}

/** Main function of RTL8169 driver
*
 *  Just initialize the driver structures and
 *  put it into the device drivers interface
 */
int main(void)
{
	int rc = nic_driver_init(NAME);
	if (rc != EOK)
		return rc;
	nic_driver_implement(
		&rtl8169_driver_ops, &rtl8169_dev_ops, &rtl8169_nic_iface);

	ddf_log_init(NAME);
	ddf_msg(LVL_NOTE, "HelenOS RTL8169 driver started");
	return ddf_driver_main(&rtl8169_driver);
}
