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

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#include <assert.h>
#include <async.h>
#include <errno.h>

#include "pci_dev_iface.h"
#include "ddf/driver.h"

static void remote_config_space_read_8(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_config_space_read_16(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_config_space_read_32(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

static void remote_config_space_write_8(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_config_space_write_16(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_config_space_write_32(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote USB interface operations. */
static remote_iface_func_ptr_t remote_pci_iface_ops [] = {
	remote_config_space_read_8,
	remote_config_space_read_16,
	remote_config_space_read_32,

	remote_config_space_write_8,
	remote_config_space_write_16,
	remote_config_space_write_32
};

/** Remote USB interface structure.
 */
remote_iface_t remote_pci_iface = {
	.method_count = sizeof(remote_pci_iface_ops) /
	    sizeof(remote_pci_iface_ops[0]),
	.methods = remote_pci_iface_ops
};

void remote_config_space_read_8(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	assert(iface);
	pci_dev_iface_t *pci_iface = (pci_dev_iface_t *)iface;
	if (pci_iface->config_space_read_8 == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	uint32_t address = DEV_IPC_GET_ARG1(*call);
	uint8_t value;
	int ret = pci_iface->config_space_read_8(fun, address, &value);
	if (ret != EOK) {
		async_answer_0(callid, ret);
	} else {
		async_answer_1(callid, EOK, value);
	}
}

void remote_config_space_read_16(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	assert(iface);
	pci_dev_iface_t *pci_iface = (pci_dev_iface_t *)iface;
	if (pci_iface->config_space_read_16 == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	uint32_t address = DEV_IPC_GET_ARG1(*call);
	uint16_t value;
	int ret = pci_iface->config_space_read_16(fun, address, &value);
	if (ret != EOK) {
		async_answer_0(callid, ret);
	} else {
		async_answer_1(callid, EOK, value);
	}
}
void remote_config_space_read_32(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	assert(iface);
	pci_dev_iface_t *pci_iface = (pci_dev_iface_t *)iface;
	if (pci_iface->config_space_read_32 == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	uint32_t address = DEV_IPC_GET_ARG1(*call);
	uint32_t value;
	int ret = pci_iface->config_space_read_32(fun, address, &value);
	if (ret != EOK) {
		async_answer_0(callid, ret);
	} else {
		async_answer_1(callid, EOK, value);
	}
}

void remote_config_space_write_8(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	assert(iface);
	pci_dev_iface_t *pci_iface = (pci_dev_iface_t *)iface;
	if (pci_iface->config_space_write_8 == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	uint32_t address = DEV_IPC_GET_ARG1(*call);
	uint8_t value = DEV_IPC_GET_ARG2(*call);
	int ret = pci_iface->config_space_write_8(fun, address, value);
	if (ret != EOK) {
		async_answer_0(callid, ret);
	} else {
		async_answer_0(callid, EOK);
	}
}

void remote_config_space_write_16(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	assert(iface);
	pci_dev_iface_t *pci_iface = (pci_dev_iface_t *)iface;
	if (pci_iface->config_space_write_16 == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	uint32_t address = DEV_IPC_GET_ARG1(*call);
	uint16_t value = DEV_IPC_GET_ARG2(*call);
	int ret = pci_iface->config_space_write_16(fun, address, value);
	if (ret != EOK) {
		async_answer_0(callid, ret);
	} else {
		async_answer_0(callid, EOK);
	}
}

void remote_config_space_write_32(ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	assert(iface);
	pci_dev_iface_t *pci_iface = (pci_dev_iface_t *)iface;
	if (pci_iface->config_space_write_32 == NULL) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	uint32_t address = DEV_IPC_GET_ARG1(*call);
	uint32_t value = DEV_IPC_GET_ARG2(*call);
	int ret = pci_iface->config_space_write_32(fun, address, value);
	if (ret != EOK) {
		async_answer_0(callid, ret);
	} else {
		async_answer_0(callid, EOK);
	}
}


/**
 * @}
 */

