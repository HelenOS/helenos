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

/** @addtogroup libdevice
 * @{
 */
/** @file PCI control service API
 */

#include <abi/ipc/interfaces.h>
#include <errno.h>
#include <ipc/devman.h>
#include <ipc/services.h>
#include <ipc/pci.h>
#include <loc.h>
#include <pci.h>
#include <stdlib.h>
#include <types/pci.h>

/** Open PCI service.
 *
 * @param svc_id PCI sevice ID
 * @param rpci   Place to store pointer to PCI service object.
 *
 * @return EOK on success, ENOMEM if out of memory, EIO if service
 *         cannot be contacted.
 */
errno_t pci_open(service_id_t svc_id, pci_t **rpci)
{
	pci_t *pci;
	errno_t rc;

	pci = calloc(1, sizeof(pci_t));
	if (pci == NULL) {
		rc = ENOMEM;
		goto error;
	}

	pci->sess = loc_service_connect(svc_id, INTERFACE_PCI, 0);
	if (pci->sess == NULL) {
		rc = EIO;
		goto error;
	}

	*rpci = pci;
	return EOK;
error:
	free(pci);
	return rc;
}

/** Close PCI service.
 *
 * @param pci PCI service object or @c NULL
 */
void pci_close(pci_t *pci)
{
	if (pci == NULL)
		return;

	async_hangup(pci->sess);
	free(pci);
}

/** Get list of IDs into a buffer of fixed size.
 *
 * @param pci      PCI service
 * @param method   IPC method
 * @param arg1     First argument
 * @param id_buf   Buffer to store IDs
 * @param buf_size Buffer size
 * @param act_size Place to store actual size of complete data.
 *
 * @return EOK on success or an error code.
 */
static errno_t pci_get_ids_once(pci_t *pci, sysarg_t method, sysarg_t arg1,
    sysarg_t *id_buf, size_t buf_size, size_t *act_size)
{
	async_exch_t *exch = async_exchange_begin(pci->sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, method, arg1, &answer);
	errno_t rc = async_data_read_start(exch, id_buf, buf_size);

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK) {
		return retval;
	}

	*act_size = ipc_get_arg1(&answer);
	return EOK;
}

/** Get list of IDs.
 *
 * Returns an allocated array of service IDs.
 *
 * @param pci    PCI service
 * @param method IPC method
 * @param arg1   IPC argument 1
 * @param data   Place to store pointer to array of IDs
 * @param count  Place to store number of IDs
 * @return       EOK on success or an error code
 */
static errno_t pci_get_ids_internal(pci_t *pci, sysarg_t method, sysarg_t arg1,
    sysarg_t **data, size_t *count)
{
	*data = NULL;
	*count = 0;

	size_t act_size = 0;
	errno_t rc = pci_get_ids_once(pci, method, arg1, NULL, 0, &act_size);
	if (rc != EOK)
		return rc;

	size_t alloc_size = act_size;
	service_id_t *ids = malloc(alloc_size);
	if (ids == NULL)
		return ENOMEM;

	while (true) {
		rc = pci_get_ids_once(pci, method, arg1, ids, alloc_size,
		    &act_size);
		if (rc != EOK)
			return rc;

		if (act_size <= alloc_size)
			break;

		alloc_size = act_size;
		service_id_t *tmp = realloc(ids, alloc_size);
		if (tmp == NULL) {
			free(ids);
			return ENOMEM;
		}
		ids = tmp;
	}

	*count = act_size / sizeof(service_id_t);
	*data = ids;
	return EOK;
}

/** Get list of PCI devices as array of devman handles.
 *
 * @param pci PCI service
 * @param data Place to store pointer to array
 * @param count Place to store length of array (number of entries)
 *
 * @return EOK on success or an error code
 */
errno_t pci_get_devices(pci_t *pci, devman_handle_t **data, size_t *count)
{
	return pci_get_ids_internal(pci, PCI_GET_DEVICES, 0, data, count);
}

extern errno_t pci_dev_get_info(pci_t *, devman_handle_t, pci_dev_info_t *);

/** Get PCI device information.
 *
 * @param pci PCI service
 * @param dev_handle Device handle
 * @param info Place to store device information
 * @return EOK on success or an error code
 */
errno_t pci_dev_get_info(pci_t *pci, devman_handle_t dev_handle,
    pci_dev_info_t *info)
{
	async_exch_t *exch;
	errno_t retval;
	ipc_call_t answer;

	exch = async_exchange_begin(pci->sess);
	aid_t req = async_send_1(exch, PCI_DEV_GET_INFO, dev_handle, &answer);

	errno_t rc = async_data_read_start(exch, info, sizeof(pci_dev_info_t));
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return EIO;
	}

	async_wait_for(req, &retval);
	if (retval != EOK)
		return EIO;

	return EOK;
}

/** @}
 */
