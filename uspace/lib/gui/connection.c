/*
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup gui
 * @{
 */
/**
 * @file
 */

#include <mem.h>
#include <stdlib.h>
#include <adt/list.h>
#include <adt/prodcons.h>
#include <fibril_synch.h>
#include <io/window.h>

#include "window.h"
#include "widget.h"
#include "connection.h"

typedef struct {
	link_t link;
	widget_t *widget;
	slot_t slot;
} slot_node_t;

typedef struct {
	link_t link;
	signal_t *signal;
	list_t slots;
} signal_node_t;

static FIBRIL_RWLOCK_INITIALIZE(connection_guard);
static LIST_INITIALIZE(connection_list);

void sig_connect(signal_t *signal, widget_t *widget, slot_t slot)
{
	fibril_rwlock_write_lock(&connection_guard);

	signal_node_t *sig_node = NULL;
	list_foreach(connection_list, link, signal_node_t, cur) {
		if (cur->signal == signal) {
			sig_node = cur;
			break;
		}
	}

	if (!sig_node) {
		sig_node = (signal_node_t *) malloc(sizeof(signal_node_t));
		if (!sig_node) {
			fibril_rwlock_write_unlock(&connection_guard);
			return;
		}

		sig_node->signal = signal;
		link_initialize(&sig_node->link);
		list_initialize(&sig_node->slots);

		list_append(&sig_node->link, &connection_list);
	}

	slot_node_t *slt_node = NULL;
	list_foreach(sig_node->slots, link, slot_node_t, cur) {
		if (cur->widget == widget && cur->slot == slot) {
			slt_node = cur;
			fibril_rwlock_write_unlock(&connection_guard);
			break;
		}
	}

	if (!slt_node) {
		slt_node = (slot_node_t *) malloc(sizeof(slot_node_t));
		if (!slt_node) {
			fibril_rwlock_write_unlock(&connection_guard);
			return;
		}

		slt_node->widget = widget;
		slt_node->slot = slot;
		link_initialize(&slt_node->link);

		list_append(&slt_node->link, &sig_node->slots);
	} else {
		/* Already connected. */
	}

	fibril_rwlock_write_unlock(&connection_guard);
}

void sig_disconnect(signal_t *signal, widget_t *widget, slot_t slot)
{
	fibril_rwlock_write_lock(&connection_guard);

	signal_node_t *sig_node = NULL;
	list_foreach(connection_list, link, signal_node_t, cur) {
		if (cur->signal == signal) {
			sig_node = cur;
			break;
		}
	}

	if (!sig_node) {
		fibril_rwlock_write_unlock(&connection_guard);
		return;
	}

	slot_node_t *slt_node = NULL;
	list_foreach(sig_node->slots, link, slot_node_t, cur) {
		if (cur->widget == widget && cur->slot == slot) {
			slt_node = cur;
			break;
		}
	}

	if (!slt_node) {
		fibril_rwlock_write_unlock(&connection_guard);
		return;
	}

	list_remove(&slt_node->link);
	free(slt_node);

	if (list_empty(&sig_node->slots)) {
		list_remove(&sig_node->link);
		free(sig_node);
	}

	fibril_rwlock_write_unlock(&connection_guard);
}

void sig_send(signal_t *signal, void *data)
{
	fibril_rwlock_read_lock(&connection_guard);

	signal_node_t *sig_node = NULL;
	list_foreach(connection_list, link, signal_node_t, cur) {
		if (cur->signal == signal) {
			sig_node = cur;
			break;
		}
	}

	if (!sig_node) {
		fibril_rwlock_read_unlock(&connection_guard);
		return;
	}

	list_foreach(sig_node->slots, link, slot_node_t, cur) {
		cur->slot(cur->widget, data);
	}

	fibril_rwlock_read_unlock(&connection_guard);
}

void sig_post(signal_t *signal, void *data, size_t data_size)
{
	fibril_rwlock_read_lock(&connection_guard);

	signal_node_t *sig_node = NULL;
	list_foreach(connection_list, link, signal_node_t, cur) {
		if (cur->signal == signal) {
			sig_node = cur;
			break;
		}
	}

	if (!sig_node) {
		fibril_rwlock_read_unlock(&connection_guard);
		return;
	}

	list_foreach(sig_node->slots, link, slot_node_t, cur) {
		void *data_copy = NULL;
		if (data != NULL)
			data_copy = malloc(data_size);

		if (data_copy != NULL)
			memcpy(data_copy, data, data_size);

		window_event_t *event =
		    (window_event_t *) malloc(sizeof(window_event_t));

		if (event) {
			link_initialize(&event->link);
			event->type = ET_SIGNAL_EVENT;
			event->data.signal.object = (sysarg_t) cur->widget;
			event->data.signal.slot = (sysarg_t) cur->slot;
			event->data.signal.argument = (sysarg_t) data_copy;
			prodcons_produce(&cur->widget->window->events, &event->link);
		} else {
			if (data_copy != NULL)
				free(data_copy);
		}
	}

	fibril_rwlock_read_unlock(&connection_guard);
}

/** @}
 */
