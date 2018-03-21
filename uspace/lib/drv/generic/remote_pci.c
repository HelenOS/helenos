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
#include <macros.h>

#include "pci_dev_iface.h"
#include "ddf/driver.h"

typedef enum {
	IPC_M_CONFIG_SPACE_READ_8,
	IPC_M_CONFIG_SPACE_READ_16,
	IPC_M_CONFIG_SPACE_READ_32,

	IPC_M_CONFIG_SPACE_WRITE_8,
	IPC_M_CONFIG_SPACE_WRITE_16,
	IPC_M_CONFIG_SPACE_WRITE_32
} pci_dev_iface_funcs_t;

errno_t pci_config_space_read_8(async_sess_t *sess, uint32_t address, uint8_t *val)
{
	sysarg_t res = 0;

	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_2_1(exch, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_READ_8, address, &res);
	async_exchange_end(exch);

	*val = (uint8_t) res;
	return rc;
}

errno_t pci_config_space_read_16(async_sess_t *sess, uint32_t address,
    uint16_t *val)
{
	sysarg_t res = 0;

	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_2_1(exch, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_READ_16, address, &res);
	async_exchange_end(exch);

	*val = (uint16_t) res;
	return rc;
}

errno_t pci_config_space_read_32(async_sess_t *sess, uint32_t address,
    uint32_t *val)
{
	sysarg_t res = 0;

	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_2_1(exch, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_READ_32, address, &res);
	async_exchange_end(exch);

	*val = (uint32_t) res;
	return rc;
}

errno_t pci_config_space_write_8(async_sess_t *sess, uint32_t address, uint8_t val)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_3_0(exch, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_WRITE_8, address, val);
	async_exchange_end(exch);

	return rc;
}

errno_t pci_config_space_write_16(async_sess_t *sess, uint32_t address,
    uint16_t val)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_3_0(exch, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_WRITE_16, address, val);
	async_exchange_end(exch);

	return rc;
}

errno_t pci_config_space_write_32(async_sess_t *sess, uint32_t address,
    uint32_t val)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t rc = async_req_3_0(exch, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_WRITE_32, address, val);
	async_exchange_end(exch);

	return rc;
}

static void remote_config_space_read_8(ddf_fun_t *, void *, cap_call_handle_t, ipc_call_t *);
static void remote_config_space_read_16(ddf_fun_t *, void *, cap_call_handle_t, ipc_call_t *);
static void remote_config_space_read_32(ddf_fun_t *, void *, cap_call_handle_t, ipc_call_t *);

static void remote_config_space_write_8(ddf_fun_t *, void *, cap_call_handle_t, ipc_call_t *);
static void remote_config_space_write_16(ddf_fun_t *, void *, cap_call_handle_t, ipc_call_t *);
static void remote_config_space_write_32(ddf_fun_t *, void *, cap_call_handle_t, ipc_call_t *);

/** Remote USB interface operations. */
static const remote_iface_func_ptr_t remote_pci_iface_ops [] = {
	[IPC_M_CONFIG_SPACE_READ_8] = remote_config_space_read_8,
	[IPC_M_CONFIG_SPACE_READ_16] = remote_config_space_read_16,
	[IPC_M_CONFIG_SPACE_READ_32] = remote_config_space_read_32,

	[IPC_M_CONFIG_SPACE_WRITE_8] = remote_config_space_write_8,
	[IPC_M_CONFIG_SPACE_WRITE_16] = remote_config_space_write_16,
	[IPC_M_CONFIG_SPACE_WRITE_32] = remote_config_space_write_32
};

/** Remote USB interface structure.
 */
const remote_iface_t remote_pci_iface = {
	.method_count = ARRAY_SIZE(remote_pci_iface_ops),
	.methods = remote_pci_iface_ops
};

void remote_config_space_read_8(ddf_fun_t *fun, void *iface, cap_call_handle_t chandle, ipc_call_t *call)
{
	assert(iface);
	pci_dev_iface_t *pci_iface = (pci_dev_iface_t *)iface;
	if (pci_iface->config_space_read_8 == NULL) {
		async_answer_0(chandle, ENOTSUP);
		return;
	}
	uint32_t address = DEV_IPC_GET_ARG1(*call);
	uint8_t value;
	errno_t ret = pci_iface->config_space_read_8(fun, address, &value);
	if (ret != EOK) {
		async_answer_0(chandle, ret);
	} else {
		async_answer_1(chandle, EOK, value);
	}
}

void remote_config_space_read_16(ddf_fun_t *fun, void *iface, cap_call_handle_t chandle, ipc_call_t *call)
{
	assert(iface);
	pci_dev_iface_t *pci_iface = (pci_dev_iface_t *)iface;
	if (pci_iface->config_space_read_16 == NULL) {
		async_answer_0(chandle, ENOTSUP);
		return;
	}
	uint32_t address = DEV_IPC_GET_ARG1(*call);
	uint16_t value;
	errno_t ret = pci_iface->config_space_read_16(fun, address, &value);
	if (ret != EOK) {
		async_answer_0(chandle, ret);
	} else {
		async_answer_1(chandle, EOK, value);
	}
}
void remote_config_space_read_32(ddf_fun_t *fun, void *iface, cap_call_handle_t chandle, ipc_call_t *call)
{
	assert(iface);
	pci_dev_iface_t *pci_iface = (pci_dev_iface_t *)iface;
	if (pci_iface->config_space_read_32 == NULL) {
		async_answer_0(chandle, ENOTSUP);
		return;
	}
	uint32_t address = DEV_IPC_GET_ARG1(*call);
	uint32_t value;
	errno_t ret = pci_iface->config_space_read_32(fun, address, &value);
	if (ret != EOK) {
		async_answer_0(chandle, ret);
	} else {
		async_answer_1(chandle, EOK, value);
	}
}

void remote_config_space_write_8(ddf_fun_t *fun, void *iface, cap_call_handle_t chandle, ipc_call_t *call)
{
	assert(iface);
	pci_dev_iface_t *pci_iface = (pci_dev_iface_t *)iface;
	if (pci_iface->config_space_write_8 == NULL) {
		async_answer_0(chandle, ENOTSUP);
		return;
	}
	uint32_t address = DEV_IPC_GET_ARG1(*call);
	uint8_t value = DEV_IPC_GET_ARG2(*call);
	errno_t ret = pci_iface->config_space_write_8(fun, address, value);
	if (ret != EOK) {
		async_answer_0(chandle, ret);
	} else {
		async_answer_0(chandle, EOK);
	}
}

void remote_config_space_write_16(ddf_fun_t *fun, void *iface, cap_call_handle_t chandle, ipc_call_t *call)
{
	assert(iface);
	pci_dev_iface_t *pci_iface = (pci_dev_iface_t *)iface;
	if (pci_iface->config_space_write_16 == NULL) {
		async_answer_0(chandle, ENOTSUP);
		return;
	}
	uint32_t address = DEV_IPC_GET_ARG1(*call);
	uint16_t value = DEV_IPC_GET_ARG2(*call);
	errno_t ret = pci_iface->config_space_write_16(fun, address, value);
	if (ret != EOK) {
		async_answer_0(chandle, ret);
	} else {
		async_answer_0(chandle, EOK);
	}
}

void remote_config_space_write_32(ddf_fun_t *fun, void *iface, cap_call_handle_t chandle, ipc_call_t *call)
{
	assert(iface);
	pci_dev_iface_t *pci_iface = (pci_dev_iface_t *)iface;
	if (pci_iface->config_space_write_32 == NULL) {
		async_answer_0(chandle, ENOTSUP);
		return;
	}
	uint32_t address = DEV_IPC_GET_ARG1(*call);
	uint32_t value = DEV_IPC_GET_ARG2(*call);
	errno_t ret = pci_iface->config_space_write_32(fun, address, value);
	if (ret != EOK) {
		async_answer_0(chandle, ret);
	} else {
		async_answer_0(chandle, EOK);
	}
}


/**
 * @}
 */

