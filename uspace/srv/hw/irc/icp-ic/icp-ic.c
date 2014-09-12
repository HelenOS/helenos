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

/** @addtogroup icp-ic
 * @{
 */

/**
 * @file icp-ic.c
 * @brief IntegratorCP interrupt controller driver.
 */

#include <async.h>
#include <bitops.h>
#include <ddi.h>
#include <errno.h>
#include <io/log.h>
#include <ipc/services.h>
#include <ipc/irc.h>
#include <ns.h>
#include <sysinfo.h>
#include <stdio.h>
#include <stdint.h>
#include <str.h>

#include "icp-ic_hw.h"

#define NAME  "icp-ic"

enum {
	icp_pic_base = 0x14000000,
	icpic_max_irq = 32
};

static icpic_regs_t *icpic_regs;

static int icpic_enable_irq(sysarg_t irq)
{
	if (irq > icpic_max_irq)
		return EINVAL;

	log_msg(LOG_DEFAULT, LVL_NOTE, "Enable IRQ %d", irq);

	pio_write_32(&icpic_regs->irq_enableset, BIT_V(uint32_t, irq));
	return EOK;
}

/** Handle one connection to i8259.
 *
 * @param iid   Hash of the request that opened the connection.
 * @param icall Call data of the request that opened the connection.
 * @param arg	Local argument.
 */
static void icpic_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ipc_callid_t callid;
	ipc_call_t call;

	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(iid, EOK);

	while (true) {
		callid = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			/* The other side has hung up. */
			async_answer_0(callid, EOK);
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case IRC_ENABLE_INTERRUPT:
			async_answer_0(callid,
			    icpic_enable_irq(IPC_GET_ARG1(call)));
			break;
		case IRC_CLEAR_INTERRUPT:
			/* Noop */
			async_answer_0(callid, EOK);
			break;
		default:
			async_answer_0(callid, EINVAL);
			break;
		}
	}
}

static int icpic_init(void)
{
	char *platform = NULL;
	char *pstr = NULL;
	size_t platform_size;
	void *regs;
	int rc;

	platform = sysinfo_get_data("platform", &platform_size);
	if (platform == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error getting platform type.");
		rc = ENOENT;
		goto error;
	}

	pstr = str_ndup(platform, platform_size);
	if (pstr == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Out of memory.");
		rc = ENOMEM;
		goto error;
	}

	if (str_cmp(pstr, "integratorcp") != 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Platform '%s' is not 'integratorcp'.",

		    pstr);
		rc = ENOENT;
		goto error;
	}

	rc = pio_enable((void *)icp_pic_base, sizeof(icpic_regs_t), &regs);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error enabling PIO");
		goto error;
	}

	icpic_regs = (icpic_regs_t *)regs;

	async_set_client_connection(icpic_connection);
	service_register(SERVICE_IRC);

	free(platform);
	free(pstr);
	return EOK;
error:
	free(platform);
	free(pstr);
	return rc;
}

int main(int argc, char **argv)
{
	int rc;

	printf("%s: HelenOS IntegratorCP interrupt controller driver\n", NAME);

	rc = log_init(NAME);
	if (rc != EOK) {
		printf(NAME ": Error connecting logging service.");
		return 1;
	}

	if (icpic_init() != EOK)
		return -1;

	log_msg(LOG_DEFAULT, LVL_NOTE, "%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

/**
 * @}
 */
