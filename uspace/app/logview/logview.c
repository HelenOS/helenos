/*
 * Copyright (c) 2012 Vojtech Horky
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

/** @addtogroup logview
 * @{
 */
/** @file Print logs.
 */
#include <stdio.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <ipc/logger.h>
#include <ipc/services.h>
#include <ns.h>
#include <io/console.h>

#define MAX_MESSAGE_LENGTH 8192

static async_exch_t *init_ipc_with_server(const char *namespace)
{
	async_sess_t *logger_session = service_connect_blocking(
		    EXCHANGE_SERIALIZE, SERVICE_LOGGER, LOGGER_INTERFACE_SOURCE, 0);
	if (logger_session == NULL) {
		fprintf(stderr, "Failed to connect to logger service.\n");
		return NULL;
	}

	async_exch_t *exchange = async_exchange_begin(logger_session);
	if (exchange == NULL) {
		fprintf(stderr, "Failed to start exchange with logger service.\n");
		return NULL;
	}

	aid_t reg_msg = async_send_0(exchange, LOGGER_CONNECT, NULL);
	int rc = async_data_write_start(exchange, namespace, str_size(namespace));
	sysarg_t reg_msg_rc;
	async_wait_for(reg_msg, &reg_msg_rc);

	if ((rc != EOK) || (reg_msg_rc != EOK)) {
		fprintf(stderr, "Failed to register with logger service: %s.\n",
		    str_error(rc == EOK ? (int) reg_msg_rc : rc));
		async_exchange_end(exchange);
		return NULL;
	}

	return exchange;
}

static bool quit_pressed(kbd_event_t event)
{
	return (event.type == KEY_PRESS) && (event.c == 'q');
}

static int parse_message_level(const char *str)
{
	if (str == NULL) {
		return 99;
	}
	char *tmp;
	int result = strtol(str, &tmp, 10);
	if (result < 0) {
		return 0;
	}
	return result;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <service-name> [max log level]\n", argv[0]);
		return 1;
	}

	async_exch_t *exchange = init_ipc_with_server(argv[1]);
	if (exchange == NULL) {
		return 1;
	}

	int display_message_level = parse_message_level(argv[2]);

	console_ctrl_t *console = console_init(stdin, stdout);

	bool terminate = false;
	while (!terminate) {
		ipc_call_t req_msg_data;
		aid_t req_msg = async_send_0(exchange, LOGGER_GET_MESSAGE, &req_msg_data);
		char message[MAX_MESSAGE_LENGTH];
		aid_t data_msg = async_data_read(exchange, &message, MAX_MESSAGE_LENGTH, NULL);

		while (true) {
			sysarg_t retval;
			int answer_arrived = async_wait_timeout(data_msg, &retval, 1);
			if (answer_arrived == EOK) {
				async_wait_for(req_msg, &retval);
				if (retval == EOK) {
					int level = (int) IPC_GET_ARG1(req_msg_data);
					if (display_message_level >= level) {
						printf("%2d: %s\n", level, message);
					}
					break;
				}
			}

			kbd_event_t kbd_event;
			suseconds_t timeout = 1;
			bool key_pressed = console_get_kbd_event_timeout(console, &kbd_event, &timeout);
			if (key_pressed && quit_pressed(kbd_event)) {
				printf("Terminating (q pressed)...\n");
				terminate = true;
				break;
			}
		}
	}

	async_exchange_end(exchange);
	console_done(console);

	return 0;
}

/** @}
 */
