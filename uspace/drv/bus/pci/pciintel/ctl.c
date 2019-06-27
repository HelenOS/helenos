/*
 * Copyright (c) 2019 Jiri Svoboda
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
 * @addtogroup pciintel
 * @{
 */

/** @file
 */

#include <async.h>
#include <ddf/log.h>
#include <ipc/pci.h>
#include <macros.h>
#include <types/pci.h>
#include "ctl.h"
#include "pci.h"

static void pci_ctl_get_devices_srv(pci_bus_t *, ipc_call_t *);
static void pci_ctl_dev_get_info_srv(pci_bus_t *, ipc_call_t *);
static errno_t pci_ctl_get_devices(pci_bus_t *, devman_handle_t *, size_t,
    size_t *);
static errno_t pci_ctl_dev_get_info(pci_fun_t *, pci_dev_info_t *);

/** Handle control service connection */
void pci_ctl_connection(ipc_call_t *icall, void *arg)
{
	ipc_call_t call;
	pci_bus_t *bus;

	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_accept_0(icall);

	bus = pci_bus(ddf_fun_get_dev((ddf_fun_t *) arg));

	while (true) {
		async_get_call(&call);

		if (!ipc_get_imethod(&call))
			break;

		switch (ipc_get_imethod(&call)) {
		case PCI_GET_DEVICES:
			pci_ctl_get_devices_srv(bus, &call);
			break;
		case PCI_DEV_GET_INFO:
			pci_ctl_dev_get_info_srv(bus, &call);
			break;
		default:
			async_answer_0(&call, EINVAL);
			break;
		}
	}
}

/** Handle request to get list of PCI devices.
 *
 * @param bus PCI bus
 * @param call Async request
 */
static void pci_ctl_get_devices_srv(pci_bus_t *bus, ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	size_t act_size;
	devman_handle_t *buf;
	errno_t rc;

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if ((size % sizeof(devman_handle_t)) != 0) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	buf = malloc(max(1, size));
	if (buf == NULL) {
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	rc = pci_ctl_get_devices(bus, buf, size, &act_size);
	if (rc != EOK) {
		free(buf);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	errno_t retval = async_data_read_finalize(&call, buf, size);

	free(buf);
	async_answer_1(icall, retval, act_size);
}

/** Handle request to get PCI device information.
 *
 * @param bus PCI bus
 * @param call Async request
 */
static void pci_ctl_dev_get_info_srv(pci_bus_t *bus, ipc_call_t *icall)
{
	devman_handle_t dev_handle;
	pci_fun_t *fun;
	pci_dev_info_t info;
	errno_t rc;

	dev_handle = ipc_get_arg1(icall);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "pci_dev_get_info_srv(%zu)",
	    dev_handle);
	fun = pci_fun_first(bus);
	while (fun != NULL) {
		if (ddf_fun_get_handle(fun->fnode) == dev_handle)
			break;

		fun = pci_fun_next(fun);
	}

	if (fun == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "pci_dev_get_info_srv: "
		    "device %zu not found", dev_handle);
		async_answer_0(icall, ENOENT);
		return;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "pci_dev_get_info_srv: "
	    "vol_volume_get_info");
	rc = pci_ctl_dev_get_info(fun, &info);
	if (rc != EOK) {
		async_answer_0(icall, EIO);
		return;
	}

	ipc_call_t call;
	size_t size;
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(pci_dev_info_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, &info,
	    min(size, sizeof(info)));
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_0(icall, EOK);
}

/** Get list of PCI devices.
 *
 * @param bus PCI bus
 * @param call Async request
 */
static errno_t pci_ctl_get_devices(pci_bus_t *bus, devman_handle_t *id_buf,
    size_t size, size_t *act_size)
{
	pci_fun_t *fun;
	size_t cnt;

	ddf_msg(LVL_NOTE, "pci_ctl_get_devices(%p, %p, %zu, %p)\n",
	    bus, id_buf, size, act_size);

	fun = pci_fun_first(bus);
	cnt = 0;
	while (fun != NULL) {
		if (sizeof(devman_handle_t) * cnt < size)
			id_buf[cnt] = ddf_fun_get_handle(fun->fnode);
		++cnt;
		fun = pci_fun_next(fun);
	}

	*act_size = sizeof(devman_handle_t) * cnt;
	return EOK;
}

/** Get PCI device information.
 *
 * @param fun PCI function
 * @param info Place to store information
 *
 * @return EOK on success or an error code
 */
static errno_t pci_ctl_dev_get_info(pci_fun_t *fun, pci_dev_info_t *info)
{
	info->dev_handle = ddf_fun_get_handle(fun->fnode);
	info->bus_num = fun->bus;
	info->dev_num = fun->dev;
	info->fn_num = fun->fn;
	info->vendor_id = fun->vendor_id;
	info->device_id = fun->device_id;
	return EOK;
}

/**
 * @}
 */
