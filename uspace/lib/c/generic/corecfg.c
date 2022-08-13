/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <async.h>
#include <assert.h>
#include <corecfg.h>
#include <errno.h>
#include <ipc/corecfg.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>

static async_sess_t *corecfg_sess = NULL;

errno_t corecfg_init(void)
{
	service_id_t corecfg_svc;
	errno_t rc;

	assert(corecfg_sess == NULL);

	rc = loc_service_get_id(SERVICE_NAME_CORECFG, &corecfg_svc,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return ENOENT;

	corecfg_sess = loc_service_connect(corecfg_svc, INTERFACE_CORECFG,
	    IPC_FLAG_BLOCKING);
	if (corecfg_sess == NULL)
		return ENOENT;

	return EOK;
}

/** Get core dump enable status. */
errno_t corecfg_get_enable(bool *renable)
{
	async_exch_t *exch = async_exchange_begin(corecfg_sess);
	sysarg_t enable;

	errno_t rc = async_req_0_1(exch, CORECFG_GET_ENABLE, &enable);

	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	*renable = enable;
	return EOK;
}

/** Enable or disable core dumps. */
errno_t corecfg_set_enable(bool enable)
{
	async_exch_t *exch = async_exchange_begin(corecfg_sess);
	errno_t rc = async_req_1_0(exch, CORECFG_SET_ENABLE, (sysarg_t) enable);

	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	return EOK;
}

/** @}
 */
