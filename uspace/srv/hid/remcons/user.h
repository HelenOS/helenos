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

/** @addtogroup remcons
 * @{
 */
/** @file
 */

#ifndef TELNET_USER_H_
#define TELNET_USER_H_

#include <fibril_synch.h>
#include <inttypes.h>
#include "remcons.h"

#define BUFFER_SIZE 1024

typedef struct {
	int id;
	int socket;
	service_id_t service_id;
	char *service_name;
	/** Producer-consumer of kbd_event_t. */
	prodcons_t in_events;
	link_t link;
	char socket_buffer[BUFFER_SIZE];
	size_t socket_buffer_len;
	size_t socket_buffer_pos;

	task_id_t task_id;

	/* Reference counting. */
	fibril_condvar_t refcount_cv;
	fibril_mutex_t refcount_mutex;
	bool task_finished;
	int locsrv_connection_count;
	bool socket_closed;
} telnet_user_t;

telnet_user_t *telnet_user_create(int socket);
void telnet_user_destroy(telnet_user_t *user);
telnet_user_t *telnet_user_get_for_client_connection(service_id_t id);
void telnet_user_notify_client_disconnected(telnet_user_t *user);

#endif

/**
 * @}
 */
