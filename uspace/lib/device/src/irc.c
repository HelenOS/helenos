/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#include <assert.h>
#include <async.h>
#include <errno.h>
#include <fibril.h>
#include <ipc/irc.h>
#include <ipc/services.h>
#include <irc.h>
#include <loc.h>
#include <stdlib.h>
#include <sysinfo.h>

static async_sess_t *irc_sess;

/** Connect to IRC service.
 *
 * @return	EOK on success, EIO on failure
 */
static errno_t irc_init(void)
{
	category_id_t irc_cat;
	service_id_t *svcs;
	size_t count;
	errno_t rc;

	assert(irc_sess == NULL);
	rc = loc_category_get_id("irc", &irc_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return EIO;

	while (true) {
		rc = loc_category_get_svcs(irc_cat, &svcs, &count);
		if (rc != EOK)
			return EIO;

		if (count > 0)
			break;

		free(svcs);

		// XXX This is just a temporary hack
		fibril_usleep(500 * 1000);
	}

	irc_sess = loc_service_connect(svcs[0], INTERFACE_IRC,
	    IPC_FLAG_BLOCKING);
	free(svcs);

	if (irc_sess == NULL)
		return EIO;

	return EOK;
}

/** Enable interrupt.
 *
 * Allow interrupt delivery.
 *
 * @param irq	IRQ number
 */
errno_t irc_enable_interrupt(int irq)
{
	errno_t rc;

	if (irc_sess == NULL) {
		rc = irc_init();
		if (rc != EOK)
			return rc;
	}

	async_exch_t *exch = async_exchange_begin(irc_sess);
	rc = async_req_1_0(exch, IRC_ENABLE_INTERRUPT, irq);
	async_exchange_end(exch);

	return rc;
}

/** Disable interrupt.
 *
 * Disallow interrupt delivery.
 *
 * @param irq	IRQ number
 */
errno_t irc_disable_interrupt(int irq)
{
	errno_t rc;

	if (irc_sess == NULL) {
		rc = irc_init();
		if (rc != EOK)
			return rc;
	}

	async_exch_t *exch = async_exchange_begin(irc_sess);
	rc = async_req_1_0(exch, IRC_DISABLE_INTERRUPT, irq);
	async_exchange_end(exch);

	return rc;
}

/** Clear interrupt.
 *
 * Clear/acknowledge interrupt in interrupt controller so that
 * another interrupt can be delivered.
 *
 * @param irq	IRQ number
 */
errno_t irc_clear_interrupt(int irq)
{
	errno_t rc;

	if (irc_sess == NULL) {
		rc = irc_init();
		if (rc != EOK)
			return rc;
	}

	async_exch_t *exch = async_exchange_begin(irc_sess);
	rc = async_req_1_0(exch, IRC_CLEAR_INTERRUPT, irq);
	async_exchange_end(exch);

	return rc;
}

/** @}
 */
