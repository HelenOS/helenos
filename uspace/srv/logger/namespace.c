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

/** @addtogroup logger
 * @{
 */
#include <assert.h>
#include <malloc.h>
#include <str.h>
#include <stdio.h>
#include "logger.h"

/** @file
 * Logging namespaces.
 */

struct logging_namespace {
	fibril_mutex_t guard;
	bool has_writer;
	bool has_reader;
	const char *name;
	link_t link;
	prodcons_t messages;
};

static FIBRIL_MUTEX_INITIALIZE(namespace_list_guard);
static LIST_INITIALIZE(namespace_list);

log_message_t *message_create(const char *name, log_level_t level)
{
	log_message_t *message = malloc(sizeof(log_message_t));
	if (message == NULL)
		return NULL;

	message->message = str_dup(name);
	if (message->message == NULL) {
		free(message);
		return NULL;
	}

	message->level = level;
	link_initialize(&message->link);

	return message;
}

void message_destroy(log_message_t *message)
{
	assert(message);
	free(message->message);
	free(message);
}

static logging_namespace_t *namespace_find_no_lock(const char *name)
{
	list_foreach(namespace_list, it) {
		logging_namespace_t *namespace = list_get_instance(it, logging_namespace_t, link);
		if (str_cmp(namespace->name, name) == 0) {
			return namespace;
		}
	}

	return NULL;
}

logging_namespace_t *namespace_create(const char *name)
{
	fibril_mutex_lock(&namespace_list_guard);
	logging_namespace_t *existing = namespace_find_no_lock(name);
	if (existing != NULL) {
		fibril_mutex_unlock(&namespace_list_guard);
		return NULL;
	}

	logging_namespace_t *namespace = malloc(sizeof(logging_namespace_t));
	if (namespace == NULL) {
		fibril_mutex_unlock(&namespace_list_guard);
		return NULL;
	}

	namespace->name = str_dup(name);
	if (namespace->name == NULL) {
		fibril_mutex_unlock(&namespace_list_guard);
		free(namespace);
		return NULL;
	}

	fibril_mutex_initialize(&namespace->guard);
	prodcons_initialize(&namespace->messages);
	namespace->has_reader = false;
	namespace->has_writer = true;
	link_initialize(&namespace->link);

	list_append(&namespace->link, &namespace_list);
	fibril_mutex_unlock(&namespace_list_guard);

	return namespace;
}

const char *namespace_get_name(logging_namespace_t *namespace)
{
	assert(namespace);
	return namespace->name;
}

static void namespace_destroy_careful(logging_namespace_t *namespace)
{
	assert(namespace);
	fibril_mutex_lock(&namespace_list_guard);

	fibril_mutex_lock(&namespace->guard);
	if (namespace->has_reader || namespace->has_writer) {
		fibril_mutex_unlock(&namespace->guard);
		fibril_mutex_unlock(&namespace_list_guard);
		return;
	}

	list_remove(&namespace->link);

	fibril_mutex_unlock(&namespace->guard);
	fibril_mutex_unlock(&namespace_list_guard);

	// TODO - destroy pending messages
	free(namespace->name);
	free(namespace);
}

void namespace_destroy(logging_namespace_t *namespace)
{
	namespace_destroy_careful(namespace);
}

logging_namespace_t *namespace_reader_attach(const char *name)
{
	logging_namespace_t *namespace = NULL;

	fibril_mutex_lock(&namespace_list_guard);

	namespace = namespace_find_no_lock(name);

	if (namespace != NULL) {
		fibril_mutex_lock(&namespace->guard);
		namespace->has_reader = true;
		fibril_mutex_unlock(&namespace->guard);
	}

	fibril_mutex_unlock(&namespace_list_guard);

	return namespace;
}

void namespace_reader_detach(logging_namespace_t *namespace)
{
	fibril_mutex_lock(&namespace->guard);
	namespace->has_reader = false;
	fibril_mutex_unlock(&namespace->guard);

	namespace_destroy_careful(namespace);
}

void namespace_writer_detach(logging_namespace_t *namespace)
{
	fibril_mutex_lock(&namespace->guard);
	namespace->has_writer = false;
	fibril_mutex_unlock(&namespace->guard);

	namespace_destroy_careful(namespace);
}



void namespace_add_message(logging_namespace_t *namespace, const char *message, log_level_t level)
{
	if (level <= DEFAULT_LOGGING_LEVEL) {
		printf("[%s %d]: %s\n", namespace->name, level, message);
	}

	fibril_mutex_lock(&namespace->guard);
	if (namespace->has_reader) {
		log_message_t *msg = message_create(message, level);
		if (msg != NULL) {
			prodcons_produce(&namespace->messages, &msg->link);
		}
	}
	fibril_mutex_unlock(&namespace->guard);
}

log_message_t *namespace_get_next_message(logging_namespace_t *namespace)
{
	link_t *message = prodcons_consume(&namespace->messages);

	return list_get_instance(message, log_message_t, link);
}


/**
 * @}
 */
