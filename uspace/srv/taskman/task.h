/*
 * Copyright (c) 2009 Martin Decky
 * Copyright (c) 2015 Michal Koutny
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

/** @addtogroup taskman
 * @{
 */

#ifndef TASKMAN_TASK_H__
#define TASKMAN_TASK_H__

#include <abi/proc/task.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include <fibril_synch.h>
#include <ipc/common.h>

/** What type of retval from the task we have */
typedef enum {
	RVAL_UNSET,     /**< unset */
	RVAL_SET,       /**< retval set, e.g. by server */
	RVAL_SET_EXIT   /**< retval set, wait for expected task exit */
} retval_t;

/** Holds necessary information of each (registered) task. */
typedef struct {
	ht_link_t link;
	
	task_id_t id;          /**< Task id. */
	task_exit_t exit;      /**< Task's uspace exit status. */
	bool failed;           /**< Task failed (task can exit unexpectedly
				    even w/out failure). */
	retval_t retval_type;  /**< Task returned a value. */
	int retval;            /**< The return value. */

	link_t listeners;      /**< Link to listeners list. */
	async_sess_t *sess;    /**< Session for notifications to task. */
} task_t;

typedef bool (* task_walker_t)(task_t *, void *);

extern fibril_rwlock_t task_hash_table_lock;

extern int tasks_init(void);

extern task_t *task_get_by_id(task_id_t);

extern void task_foreach(task_walker_t, void *);

extern void task_remove(task_t **);

extern int task_intro(task_id_t);

#endif

/**
 * @}
 */
