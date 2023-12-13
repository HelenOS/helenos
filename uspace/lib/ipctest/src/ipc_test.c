/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/** @file IPC test service API
 */

#include <abi/ipc/interfaces.h>
#include <as.h>
#include <errno.h>
#include <ipc/services.h>
#include <ipc/ipc_test.h>
#include <loc.h>
#include <stdlib.h>
#include <ipc_test.h>

/** Create IPC test service session.
 *
 * @param rvol Place to store pointer to volume service session.
 * @return     EOK on success, ENOMEM if out of memory, EIO if service
 *             cannot be contacted.
 */
errno_t ipc_test_create(ipc_test_t **rtest)
{
	ipc_test_t *test;
	service_id_t test_svcid;
	errno_t rc;

	test = calloc(1, sizeof(ipc_test_t));
	if (test == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = loc_service_get_id(SERVICE_NAME_IPC_TEST, &test_svcid, 0);
	if (rc != EOK) {
		rc = ENOENT;
		goto error;
	}

	test->sess = loc_service_connect(test_svcid, INTERFACE_IPC_TEST, 0);
	if (test->sess == NULL) {
		rc = EIO;
		goto error;
	}

	*rtest = test;
	return EOK;
error:
	free(test);
	return rc;
}

/** Destroy IPC test service session.
 *
 * @param test IPC test service session
 */
void ipc_test_destroy(ipc_test_t *test)
{
	if (test == NULL)
		return;

	async_hangup(test->sess);
	free(test);
}

/** Simple ping.
 *
 * @param test IPC test service
 * @return EOK on success or an error code
 */
errno_t ipc_test_ping(ipc_test_t *test)
{
	async_exch_t *exch;
	errno_t retval;

	exch = async_exchange_begin(test->sess);
	retval = async_req_0_0(exch, IPC_TEST_PING);
	async_exchange_end(exch);

	if (retval != EOK)
		return retval;

	return EOK;
}

/** Get size of shared read-only memory area.
 *
 * @param test IPC test service
 * @param rsize Place to store size of the shared area
 * @return EOK on success or an error code
 */
errno_t ipc_test_get_ro_area_size(ipc_test_t *test, size_t *rsize)
{
	async_exch_t *exch;
	errno_t retval;
	sysarg_t size;

	exch = async_exchange_begin(test->sess);
	retval = async_req_0_1(exch, IPC_TEST_GET_RO_AREA_SIZE, &size);
	async_exchange_end(exch);

	if (retval != EOK)
		return retval;

	*rsize = size;
	return EOK;
}

/** Get size of shared read-write memory area.
 *
 * @param test IPC test service
 * @param rsize Place to store size of the shared area
 * @return EOK on success or an error code
 */
errno_t ipc_test_get_rw_area_size(ipc_test_t *test, size_t *rsize)
{
	async_exch_t *exch;
	errno_t retval;
	sysarg_t size;

	exch = async_exchange_begin(test->sess);
	retval = async_req_0_1(exch, IPC_TEST_GET_RW_AREA_SIZE, &size);
	async_exchange_end(exch);

	if (retval != EOK)
		return retval;

	*rsize = size;
	return EOK;
}

/** Test share-in of read-only area.
 *
 * @param test IPC test service
 * @param size Size of the shared area
 * @param rptr Place to store pointer to the shared-in area
 * @return EOK on success or an error code
 */
errno_t ipc_test_share_in_ro(ipc_test_t *test, size_t size, const void **rptr)
{
	async_exch_t *exch;
	ipc_call_t answer;
	aid_t req;
	void *dst;
	errno_t rc;

	exch = async_exchange_begin(test->sess);
	req = async_send_0(exch, IPC_TEST_SHARE_IN_RO, &answer);

	dst = NULL;
	rc = async_share_in_start_0_0(exch, size, &dst);
	if (rc != EOK || dst == AS_MAP_FAILED) {
		async_exchange_end(exch);
		async_forget(req);
		return ENOMEM;
	}

	async_exchange_end(exch);
	async_wait_for(req, NULL);
	*rptr = dst;
	return EOK;
}

/** Test share-in of read-write area.
 *
 * @param test IPC test service
 * @param size Size of the shared area
 * @param rptr Place to store pointer to the shared-in area
 * @return EOK on success or an error code
 */
errno_t ipc_test_share_in_rw(ipc_test_t *test, size_t size, void **rptr)
{
	async_exch_t *exch;
	ipc_call_t answer;
	aid_t req;
	void *dst;
	errno_t rc;

	exch = async_exchange_begin(test->sess);
	req = async_send_0(exch, IPC_TEST_SHARE_IN_RW, &answer);

	rc = async_share_in_start_0_0(exch, size, &dst);
	if (rc != EOK || dst == AS_MAP_FAILED) {
		async_exchange_end(exch);
		async_forget(req);
		return ENOMEM;
	}

	async_exchange_end(exch);
	async_wait_for(req, NULL);
	*rptr = dst;
	return EOK;
}

/** Set server-side read/write buffer size.
 *
 * @param test IPC test service
 * @param size Requested read/write buffer size
 * @return EOK on success or an error code
 */
errno_t ipc_test_set_rw_buf_size(ipc_test_t *test, size_t size)
{
	async_exch_t *exch;
	errno_t retval;

	exch = async_exchange_begin(test->sess);
	retval = async_req_1_0(exch, IPC_TEST_SET_RW_BUF_SIZE, size);
	async_exchange_end(exch);

	if (retval != EOK)
		return retval;

	return EOK;
}

/** Test IPC read.
 *
 * @param test IPC test service
 * @param dest Destination buffer
 * @param size Number of bytes to read / size of destination buffer
 * @return EOK on success or an error code
 */
errno_t ipc_test_read(ipc_test_t *test, void *dest, size_t size)
{
	async_exch_t *exch;
	ipc_call_t answer;
	aid_t req;
	errno_t rc;

	exch = async_exchange_begin(test->sess);
	req = async_send_0(exch, IPC_TEST_READ, &answer);

	rc = async_data_read_start(exch, dest, size);
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	async_exchange_end(exch);
	async_wait_for(req, NULL);
	return EOK;
}

/** Test IPC write.
 *
 * @param test IPC test service
 * @param data Source buffer
 * @param size Number of bytes to write
 * @return EOK on success or an error code
 */
errno_t ipc_test_write(ipc_test_t *test, const void *data, size_t size)
{
	async_exch_t *exch;
	ipc_call_t answer;
	aid_t req;
	errno_t rc;

	exch = async_exchange_begin(test->sess);
	req = async_send_0(exch, IPC_TEST_WRITE, &answer);

	rc = async_data_write_start(exch, data, size);
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	async_exchange_end(exch);
	async_wait_for(req, NULL);
	return EOK;
}

/** @}
 */
