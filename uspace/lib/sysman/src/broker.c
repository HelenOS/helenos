/*
 * Copyright (c) 2015 Michal Koutny
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
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

#include <async.h>
#include <ipc/sysman.h>
#include <sysman/broker.h>
#include <sysman/sysman.h>
#include <str.h>

int sysman_broker_register(void)
{
	async_exch_t *exch = sysman_exchange_begin(SYSMAN_PORT_BROKER);

	int rc = async_req_0_0(exch, SYSMAN_BROKER_REGISTER);
	sysman_exchange_end(exch);

	return rc;
}

void sysman_ipc_forwarded(task_id_t caller, const char *dst_unit_name)
{
	async_exch_t *exch = sysman_exchange_begin(SYSMAN_PORT_BROKER);

	aid_t req = async_send_1(exch, SYSMAN_BROKER_IPC_FWD, caller, NULL);
	(void)async_data_write_start(exch, dst_unit_name, str_size(dst_unit_name));
	sysman_exchange_end(exch);

	async_forget(req);
}

void sysman_main_exposee_added(const char *unit_name, task_id_t caller)
{
	async_exch_t *exch = sysman_exchange_begin(SYSMAN_PORT_BROKER);

	aid_t req = async_send_1(exch, SYSMAN_BROKER_MAIN_EXP_ADDED, caller, NULL);
	(void)async_data_write_start(exch, unit_name, str_size(unit_name));
	sysman_exchange_end(exch);

	async_forget(req);
}

void sysman_exposee_added(const char *exposee)
{
	async_exch_t *exch = sysman_exchange_begin(SYSMAN_PORT_BROKER);

	aid_t req = async_send_0(exch, SYSMAN_BROKER_EXP_ADDED, NULL);
	(void)async_data_write_start(exch, exposee, str_size(exposee));
	sysman_exchange_end(exch);

	async_forget(req);
}

void sysman_exposee_removed(const char *exposee)
{
	async_exch_t *exch = sysman_exchange_begin(SYSMAN_PORT_BROKER);

	aid_t req = async_send_0(exch, SYSMAN_BROKER_EXP_REMOVED, NULL);
	(void)async_data_write_start(exch, exposee, str_size(exposee));
	sysman_exchange_end(exch);

	async_forget(req);
}
