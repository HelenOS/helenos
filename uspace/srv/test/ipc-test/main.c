/*
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup ipc-test
 * @{
 */
/**
 * @file
 * @brief IPC test service
 *
 * If run as an initial task, this can be used to test sharing areas
 * backed by the ELF backend.
 */

#include <as.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <io/log.h>
#include <ipc/ipc_test.h>
#include <ipc/services.h>
#include <loc.h>
#include <mem.h>
#include <stdio.h>
#include <task.h>

#define NAME  "ipc-test"

static service_id_t svc_id;

/** Object in read-only memory area that will be shared.
 *
 * If the server is run as an initial task, the area should be backed
 * by ELF backend.
 */
static const char *ro_data = "Hello, world!";

/** Object in read-write memory area that will be shared.
 *
 * If the server is run as an initial task, the area should be backed
 * by ELF backend.
 */
static char rw_data[] = "Hello, world!";

static void ipc_test_get_ro_area_size_srv(ipc_call_t *icall)
{
	errno_t rc;
	as_area_info_t info;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ipc_test_get_ro_area_size_srv");

	rc = as_area_get_info((void *)ro_data, &info);
	if (rc != EOK) {
		async_answer_0(icall, EIO);
		log_msg(LOG_DEFAULT, LVL_ERROR, "as_area_get_info failed");
		return;
	}

	async_answer_1(icall, EOK, info.size);
}

static void ipc_test_get_rw_area_size_srv(ipc_call_t *icall)
{
	errno_t rc;
	as_area_info_t info;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ipc_test_get_rw_area_size_srv");

	rc = as_area_get_info(rw_data, &info);
	if (rc != EOK) {
		async_answer_0(icall, EIO);
		log_msg(LOG_DEFAULT, LVL_ERROR, "as_area_get_info failed");
		return;
	}

	async_answer_1(icall, EOK, info.size);
}

static void ipc_test_share_in_ro_srv(ipc_call_t *icall)
{
	ipc_call_t call;
	errno_t rc;
	size_t size;
	as_area_info_t info;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ipc_test_share_in_ro_srv");
	if (!async_share_in_receive(&call, &size)) {
		async_answer_0(icall, EINVAL);
		log_msg(LOG_DEFAULT, LVL_ERROR, "share_in_receive failed");
		return;
	}

	rc = as_area_get_info((void *)ro_data, &info);
	if (rc != EOK) {
		async_answer_0(icall, EINVAL);
		log_msg(LOG_DEFAULT, LVL_ERROR, "as_area_get_info failed");
		return;
	}

	if (size != info.size) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "size(%zu) != %zu",
		    size, info.size);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_share_in_finalize(&call, (void *) info.start_addr,
	    AS_AREA_READ);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    " - async_share_in_finalize failed");
		async_answer_0(icall, EINVAL);
		return;
	}

	async_answer_0(icall, EOK);
}

static void ipc_test_share_in_rw_srv(ipc_call_t *icall)
{
	ipc_call_t call;
	errno_t rc;
	size_t size;
	as_area_info_t info;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ipc_test_share_in_ro_srv");
	if (!async_share_in_receive(&call, &size)) {
		async_answer_0(icall, EINVAL);
		log_msg(LOG_DEFAULT, LVL_ERROR, "share_in_receive failed");
		return;
	}

	rc = as_area_get_info(rw_data, &info);
	if (rc != EOK) {
		async_answer_0(icall, EINVAL);
		log_msg(LOG_DEFAULT, LVL_ERROR, "as_area_get_info failed");
		return;
	}

	if (size != info.size) {
		log_msg(LOG_DEFAULT, LVL_ERROR, " size(%zu) != %zu",
		    size, info.size);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_share_in_finalize(&call, (void *) info.start_addr,
	    AS_AREA_READ | AS_AREA_WRITE);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "async_share_in_finalize failed");
		async_answer_0(icall, EINVAL);
		return;
	}

	async_answer_0(icall, EOK);
}

static void ipc_test_connection(ipc_call_t *icall, void *arg)
{
	/* Accept connection */
	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			break;
		}

		switch (ipc_get_imethod(&call)) {
		case IPC_TEST_PING:
			async_answer_0(&call, EOK);
			break;
		case IPC_TEST_GET_RO_AREA_SIZE:
			ipc_test_get_ro_area_size_srv(&call);
			break;
		case IPC_TEST_GET_RW_AREA_SIZE:
			ipc_test_get_rw_area_size_srv(&call);
			break;
		case IPC_TEST_SHARE_IN_RO:
			ipc_test_share_in_ro_srv(&call);
			break;
		case IPC_TEST_SHARE_IN_RW:
			ipc_test_share_in_rw_srv(&call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	errno_t rc;

	printf("%s: IPC test service\n", NAME);
	async_set_fallback_port_handler(ipc_test_connection, NULL);

	rc = log_init(NAME);
	if (rc != EOK) {
		printf(NAME ": Failed to initialize log.\n");
		return rc;
	}

	rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server. (%s)\n",
		    str_error(rc));
		return rc;
	}

	rc = loc_service_register(SERVICE_NAME_IPC_TEST, &svc_id);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service. (%s)\n",
		    str_error(rc));
		return rc;
	}

	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

/**
 * @}
 */
