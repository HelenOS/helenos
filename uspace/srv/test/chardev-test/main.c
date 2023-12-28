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

/** @addtogroup chardev-test
 * @{
 */
/**
 * @file
 * @brief Character device interface test service
 */

#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <io/chardev_srv.h>
#include <ipc/services.h>
#include <loc.h>
#include <mem.h>
#include <stdio.h>
#include <task.h>

#define NAME  "chardev-test"

static errno_t smallx_open(chardev_srvs_t *, chardev_srv_t *);
static errno_t smallx_close(chardev_srv_t *);
static errno_t smallx_write(chardev_srv_t *, const void *, size_t, size_t *);
static errno_t smallx_read(chardev_srv_t *, void *, size_t, size_t *,
    chardev_flags_t);

static errno_t largex_open(chardev_srvs_t *, chardev_srv_t *);
static errno_t largex_close(chardev_srv_t *);
static errno_t largex_write(chardev_srv_t *, const void *, size_t, size_t *);
static errno_t largex_read(chardev_srv_t *, void *, size_t, size_t *,
    chardev_flags_t);

static errno_t partialx_open(chardev_srvs_t *, chardev_srv_t *);
static errno_t partialx_close(chardev_srv_t *);
static errno_t partialx_write(chardev_srv_t *, const void *, size_t, size_t *);
static errno_t partialx_read(chardev_srv_t *, void *, size_t, size_t *,
    chardev_flags_t);

static service_id_t smallx_svc_id;
static chardev_srvs_t smallx_srvs;

static service_id_t largex_svc_id;
static chardev_srvs_t largex_srvs;

static service_id_t partialx_svc_id;
static chardev_srvs_t partialx_srvs;

static chardev_ops_t chardev_test_smallx_ops = {
	.open = smallx_open,
	.close = smallx_close,
	.write = smallx_write,
	.read = smallx_read
};

static chardev_ops_t chardev_test_largex_ops = {
	.open = largex_open,
	.close = largex_close,
	.write = largex_write,
	.read = largex_read
};

static chardev_ops_t chardev_test_partialx_ops = {
	.open = partialx_open,
	.close = partialx_close,
	.write = partialx_write,
	.read = partialx_read
};

static void chardev_test_connection(ipc_call_t *icall, void *arg)
{
	chardev_srvs_t *svc;
	sysarg_t svcid;

	svcid = ipc_get_arg2(icall);
	if (svcid == smallx_svc_id) {
		svc = &smallx_srvs;
	} else if (svcid == largex_svc_id) {
		svc = &largex_srvs;
	} else if (svcid == partialx_svc_id) {
		svc = &partialx_srvs;
	} else {
		async_answer_0(icall, ENOENT);
		return;
	}

	chardev_conn(icall, svc);
}

int main(int argc, char *argv[])
{
	errno_t rc;
	loc_srv_t *srv;

	printf("%s: Character device test service\n", NAME);
	async_set_fallback_port_handler(chardev_test_connection, NULL);

	rc = loc_server_register(NAME, &srv);
	if (rc != EOK) {
		printf("%s: Failed registering server.: %s\n", NAME, str_error(rc));
		return rc;
	}

	chardev_srvs_init(&smallx_srvs);
	smallx_srvs.ops = &chardev_test_smallx_ops;
	smallx_srvs.sarg = NULL;

	chardev_srvs_init(&largex_srvs);
	largex_srvs.ops = &chardev_test_largex_ops;
	largex_srvs.sarg = NULL;

	chardev_srvs_init(&partialx_srvs);
	partialx_srvs.ops = &chardev_test_partialx_ops;
	partialx_srvs.sarg = NULL;

	rc = loc_service_register(srv, SERVICE_NAME_CHARDEV_TEST_SMALLX,
	    &smallx_svc_id);
	if (rc != EOK) {
		printf("%s: Failed registering service.: %s\n", NAME, str_error(rc));
		goto error;
	}

	rc = loc_service_register(srv, SERVICE_NAME_CHARDEV_TEST_LARGEX,
	    &largex_svc_id);
	if (rc != EOK) {
		printf("%s: Failed registering service.: %s\n", NAME, str_error(rc));
		goto error;
	}

	rc = loc_service_register(srv, SERVICE_NAME_CHARDEV_TEST_PARTIALX,
	    &partialx_svc_id);
	if (rc != EOK) {
		printf("%s: Failed registering service.: %s\n", NAME, str_error(rc));
		goto error;
	}

	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
error:
	if (smallx_svc_id != 0)
		loc_service_unregister(srv, smallx_svc_id);
	if (largex_svc_id != 0)
		loc_service_unregister(srv, largex_svc_id);
	if (partialx_svc_id != 0)
		loc_service_unregister(srv, partialx_svc_id);
	loc_server_unregister(srv);
	return rc;
}

static errno_t smallx_open(chardev_srvs_t *srvs, chardev_srv_t *srv)
{
	return EOK;
}

static errno_t smallx_close(chardev_srv_t *srv)
{
	return EOK;
}

static errno_t smallx_write(chardev_srv_t *srv, const void *data, size_t size,
    size_t *nwritten)
{
	if (size < 1) {
		*nwritten = 0;
		return EOK;
	}

	*nwritten = 1;
	return EOK;
}

static errno_t smallx_read(chardev_srv_t *srv, void *buf, size_t size,
    size_t *nread, chardev_flags_t flags)
{
	if (size < 1) {
		*nread = 0;
		return EOK;
	}

	memset(buf, 0, 1);
	*nread = 1;
	return EOK;
}

static errno_t largex_open(chardev_srvs_t *srvs, chardev_srv_t *srv)
{
	return EOK;
}

static errno_t largex_close(chardev_srv_t *srv)
{
	return EOK;
}

static errno_t largex_write(chardev_srv_t *srv, const void *data, size_t size,
    size_t *nwritten)
{
	if (size < 1) {
		*nwritten = 0;
		return EOK;
	}

	*nwritten = size;
	return EOK;
}

static errno_t largex_read(chardev_srv_t *srv, void *buf, size_t size,
    size_t *nread, chardev_flags_t flags)
{
	if (size < 1) {
		*nread = 0;
		return EOK;
	}

	memset(buf, 0, size);
	*nread = size;
	return EOK;
}

static errno_t partialx_open(chardev_srvs_t *srvs, chardev_srv_t *srv)
{
	return EOK;
}

static errno_t partialx_close(chardev_srv_t *srv)
{
	return EOK;
}

static errno_t partialx_write(chardev_srv_t *srv, const void *data, size_t size,
    size_t *nwritten)
{
	if (size < 1) {
		*nwritten = 0;
		return EOK;
	}

	*nwritten = 1;
	return EIO;
}

static errno_t partialx_read(chardev_srv_t *srv, void *buf, size_t size,
    size_t *nread, chardev_flags_t flags)
{
	if (size < 1) {
		*nread = 0;
		return EOK;
	}

	memset(buf, 0, 1);
	*nread = 1;
	return EIO;
}

/**
 * @}
 */
