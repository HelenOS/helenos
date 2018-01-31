/*
 * Copyright (c) 2014 Jiri Svoboda
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
/** @file
 */

#include <assert.h>
#include <async.h>
#include <errno.h>
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
		async_usleep(500 * 1000);
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
