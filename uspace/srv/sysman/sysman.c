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

#include <adt/hash_table.h>
#include <adt/list.h>
#include <errno.h>
#include <fibril_synch.h>
#include <stdlib.h>

#include "log.h"
#include "sysman.h"


/* Do not expose this generally named type */
typedef struct {
	link_t event_queue;

	event_handler_t handler;
	void *data;
} event_t;

typedef struct {
	link_t callbacks;

	callback_handler_t handler;
	void *data;
} obj_callback_t;

typedef struct {
	ht_link_t ht_link;

	void *object;
	list_t callbacks;
} observed_object_t;

static LIST_INITIALIZE(event_queue);
static fibril_mutex_t event_queue_mtx;
static fibril_condvar_t event_queue_cv;

static hash_table_t observed_objects;
static fibril_mutex_t observed_objects_mtx;
static fibril_condvar_t observed_objects_cv;

/* Hash table functions */
static size_t observed_objects_ht_hash(const ht_link_t *item)
{
	observed_object_t *callbacks =
	    hash_table_get_inst(item, observed_object_t, ht_link);

	return (size_t) callbacks->object;
}

static size_t observed_objects_ht_key_hash(void *key)
{
	void *object = *(void **) key;
	return (size_t) object;
}

static bool observed_objects_ht_key_equal(void *key, const ht_link_t *item)
{
	void *object = *(void **)key;
	return (
	    hash_table_get_inst(item, observed_object_t, ht_link)->object ==
	    object);
}

static hash_table_ops_t observed_objects_ht_ops = {
	.hash            = &observed_objects_ht_hash,
	.key_hash        = &observed_objects_ht_key_hash,
	.equal           = NULL,
	.key_equal       = &observed_objects_ht_key_equal,
	.remove_callback = NULL
};

static void notify_observers(void *object)
{
	ht_link_t *item = hash_table_find(&observed_objects, &object);
	if (item == NULL) {
		return;
	}
	observed_object_t *observed_object =
	    hash_table_get_inst(item, observed_object_t, ht_link);

	list_foreach_safe(observed_object->callbacks, cur_link, next_link) {
		obj_callback_t *callback =
		    list_get_instance(cur_link, obj_callback_t, callbacks);
		callback->handler(object, callback->data);
		list_remove(cur_link);
		free(callback);
	}
}

/*
 * Non-static functions
 */
void sysman_events_init(void)
{
	fibril_mutex_initialize(&event_queue_mtx);
	fibril_condvar_initialize(&event_queue_cv);

	bool table =
	    hash_table_create(&observed_objects, 0, 0, &observed_objects_ht_ops);
	if (!table) {
		sysman_log(LVL_FATAL, "%s: Failed initialization", __func__);
		abort();
	}
	fibril_mutex_initialize(&observed_objects_mtx);
	fibril_condvar_initialize(&observed_objects_cv);
}

int sysman_events_loop(void *unused)
{
	while (1) {
		/* Pop event */
		fibril_mutex_lock(&event_queue_mtx);
		while (list_empty(&event_queue)) {
			fibril_condvar_wait(&event_queue_cv, &event_queue_mtx);
		}

		link_t *li_event = list_first(&event_queue);
		list_remove(li_event);
		event_t *event =
		    list_get_instance(li_event, event_t, event_queue);
		fibril_mutex_unlock(&event_queue_mtx);

		/* Process event */
		sysman_log(LVL_DEBUG2, "process(%p, %p)", event->handler, event->data);
		event->handler(event->data);
		free(event);
	}
}

void sysman_raise_event(event_handler_t handler, void *data)
{
	sysman_log(LVL_DEBUG2, "%s(%p, %p)", __func__, handler, data);
	event_t *event = malloc(sizeof(event_t));
	if (event == NULL) {
		sysman_log(LVL_FATAL, "%s: cannot allocate event", __func__);
		// TODO think about aborting system critical task
		abort();
	}
	link_initialize(&event->event_queue);
	event->handler = handler;
	event->data = data;

	fibril_mutex_lock(&event_queue_mtx);
	list_append(&event->event_queue, &event_queue);
	/* There's only single event loop, broadcast is unnecessary */
	fibril_condvar_signal(&event_queue_cv);
	fibril_mutex_unlock(&event_queue_mtx);
}

/** Register single-use object observer callback
 *
 * TODO no one handles return value, it's quite fatal to lack memory for
 *      callbacks...  @return EOK on success
 * @return ENOMEM
 */
int sysman_object_observer(void *object, callback_handler_t handler, void *data)
{
	int rc;
	observed_object_t *observed_object = NULL;
	observed_object_t *new_observed_object = NULL;
	ht_link_t *ht_link = hash_table_find(&observed_objects, &object);

	if (ht_link == NULL) {
		observed_object = malloc(sizeof(observed_object_t));
		if (observed_object == NULL) {
			rc = ENOMEM;
			goto fail;
		}
		new_observed_object = observed_object;

		observed_object->object = object;
		list_initialize(&observed_object->callbacks);
		hash_table_insert(&observed_objects, &observed_object->ht_link);
	} else {
		observed_object =
		    hash_table_get_inst(ht_link, observed_object_t, ht_link);
	}

	obj_callback_t *obj_callback = malloc(sizeof(obj_callback_t));
	if (obj_callback == NULL) {
		rc = ENOMEM;
		goto fail;
	}

	obj_callback->handler = handler;
	obj_callback->data = data;
	list_append(&obj_callback->callbacks, &observed_object->callbacks);
	return EOK;

fail:
	free(new_observed_object);
	return rc;
}

/*
 * Event handlers
 */

// NOTE must run in main event loop fibril
void sysman_event_job_process(void *arg)
{
	job_t *job = arg;
	dyn_array_t job_closure;
	dyn_array_initialize(&job_closure, job_ptr_t, 0);

	int rc = job_create_closure(job, &job_closure);
	if (rc != EOK) {
		sysman_log(LVL_ERROR, "Cannot create closure for job %p (%i)",
		    job, rc);
		goto fail;
	}

	/*
	 * If jobs are queued, reference is passed from closure to the queue,
	 * otherwise, we still have the reference.
	 */
	rc = job_queue_add_jobs(&job_closure);
	if (rc != EOK) {
		goto fail;
	}
	/* We don't need job anymore */
	job_del_ref(&job);

	// TODO explain why calling asynchronously
	sysman_raise_event(&sysman_event_job_queue_run, NULL);
	return;

fail:
	job->retval = JOB_FAILED;
	job_finish(job);
	job_del_ref(&job);

	dyn_array_foreach(job_closure, job_ptr_t, closure_job) {
		job_del_ref(&(*closure_job));
	}
	dyn_array_destroy(&job_closure);
}


void sysman_event_job_queue_run(void *unused)
{
	job_t *job;
	while ((job = job_queue_pop_runnable())) {
		job_run(job);
		job_del_ref(&job);
	}
}

void sysman_event_job_changed(void *object)
{
	notify_observers(object);
	/* Unreference the event data */
	job_t *job = object;
	job_del_ref(&job);
}
