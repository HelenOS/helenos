/*
 * Copyright (c) 2017 Jakub Jermar
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

/** @addtogroup generic
 * @{
 */
/** @file
 */

/*
 * HelenOS capabilities are task-local names for references to kernel objects.
 * Kernel objects are reference-counted wrappers for a select group of objects
 * allocated in and by the kernel that can be made accessible to userspace in a
 * controlled way via integer handles.
 *
 * A kernel object (kobject_t) encapsulates one of the following raw objects:
 *
 * - IPC call
 * - IPC phone
 * - IRQ object
 *
 * A capability (cap_t) is either free, allocated or published. Free
 * capabilities can be allocated, which reserves the capability handle in the
 * task-local capability space. Allocated capabilities can be published, which
 * associates them with an existing kernel object. Userspace can only access
 * published capabilities.
 *
 * A published capability may get unpublished, which disassociates it from the
 * underlying kernel object and puts it back into the allocated state. An
 * allocated capability can be freed to become available for future use.
 *
 * There is a 1:1 correspondence between a kernel object (kobject_t) and the
 * actual raw object it encapsulates. A kernel object (kobject_t) may have
 * multiple references, either implicit from one or more capabilities (cap_t),
 * even from capabilities in different tasks, or explicit as a result of
 * creating a new reference from a capability handle using kobject_get(), or
 * creating a new reference from an already existing reference by
 * kobject_add_ref() or as a result of unpublishing a capability and
 * disassociating it from its kobject_t using cap_unpublish().
 *
 * As kernel objects are reference-counted, they get automatically destroyed
 * when their last reference is dropped in kobject_put(). The idea is that
 * whenever a kernel object is inserted into some sort of a container (e.g. a
 * list or hash table), its reference count should be incremented via
 * kobject_get() or kobject_add_ref(). When the kernel object is removed from
 * the container, the reference count should go down via a call to
 * kobject_put().
 */

#include <cap/cap.h>
#include <abi/cap.h>
#include <proc/task.h>
#include <synch/mutex.h>
#include <abi/errno.h>
#include <mm/slab.h>
#include <adt/list.h>

#include <stdint.h>

#define CAPS_START	(CAP_NIL + 1)
#define CAPS_SIZE	(INT_MAX - CAPS_START)
#define CAPS_LAST	(CAPS_SIZE - 1)

static slab_cache_t *cap_cache;

static size_t caps_hash(const ht_link_t *item)
{
	cap_t *cap = hash_table_get_inst(item, cap_t, caps_link);
	return hash_mix(cap->handle);
}

static size_t caps_key_hash(void *key)
{
	cap_handle_t *handle = (cap_handle_t *) key;
	return hash_mix(*handle);
}

static bool caps_key_equal(void *key, const ht_link_t *item)
{
	cap_handle_t *handle = (cap_handle_t *) key;
	cap_t *cap = hash_table_get_inst(item, cap_t, caps_link);
	return *handle == cap->handle;
}

static hash_table_ops_t caps_ops = {
	.hash = caps_hash,
	.key_hash = caps_key_hash,
	.key_equal = caps_key_equal
};

void caps_init(void)
{
	cap_cache = slab_cache_create("cap_t", sizeof(cap_t), 0, NULL,
	    NULL, 0);
}

/** Allocate the capability info structure
 *
 * @param task  Task for which to allocate the info structure.
 */
errno_t caps_task_alloc(task_t *task)
{
	task->cap_info = (cap_info_t *) malloc(sizeof(cap_info_t),
	    FRAME_ATOMIC);
	if (!task->cap_info)
		return ENOMEM;
	task->cap_info->handles = ra_arena_create();
	if (!task->cap_info->handles)
		goto error_handles;
	if (!ra_span_add(task->cap_info->handles, CAPS_START, CAPS_SIZE))
		goto error_span;
	if (!hash_table_create(&task->cap_info->caps, 0, 0, &caps_ops))
		goto error_span;
	return EOK;

error_span:
	ra_arena_destroy(task->cap_info->handles);
error_handles:
	free(task->cap_info);
	return ENOMEM;
}

/** Initialize the capability info structure
 *
 * @param task  Task for which to initialize the info structure.
 */
void caps_task_init(task_t *task)
{
	mutex_initialize(&task->cap_info->lock, MUTEX_RECURSIVE);

	for (kobject_type_t t = 0; t < KOBJECT_TYPE_MAX; t++)
		list_initialize(&task->cap_info->type_list[t]);
}

/** Deallocate the capability info structure
 *
 * @param task  Task from which to deallocate the info structure.
 */
void caps_task_free(task_t *task)
{
	hash_table_destroy(&task->cap_info->caps);
	ra_arena_destroy(task->cap_info->handles);
	free(task->cap_info);
}

/** Invoke callback function on task's capabilites of given type
 *
 * @param task  Task where the invocation should take place.
 * @param type  Kernel object type of the task's capabilities that will be
 *              subject to the callback invocation.
 * @param cb    Callback function.
 * @param arg   Argument for the callback function.
 *
 * @return True if the callback was called on all matching capabilities.
 * @return False if the callback was applied only partially.
 */
bool caps_apply_to_kobject_type(task_t *task, kobject_type_t type,
    bool (*cb)(cap_t *, void *), void *arg)
{
	bool done = true;

	mutex_lock(&task->cap_info->lock);
	list_foreach_safe(task->cap_info->type_list[type], cur, next) {
		cap_t *cap = list_get_instance(cur, cap_t, type_link);
		done = cb(cap, arg);
		if (!done)
			break;
	}
	mutex_unlock(&task->cap_info->lock);

	return done;
}

/** Initialize capability and associate it with its handle
 *
 * @param cap     Address of the capability.
 * @param task    Backling to the owning task.
 * @param handle  Capability handle.
 */
static void cap_initialize(cap_t *cap, task_t *task, cap_handle_t handle)
{
	cap->state = CAP_STATE_FREE;
	cap->task = task;
	cap->handle = handle;
	link_initialize(&cap->type_link);
}

/** Get capability using capability handle
 *
 * @param task    Task whose capability to get.
 * @param handle  Capability handle of the desired capability.
 * @param state   State in which the capability must be.
 *
 * @return Address of the desired capability if it exists and its state matches.
 * @return NULL if no such capability exists or it's in a different state.
 */
static cap_t *cap_get(task_t *task, cap_handle_t handle, cap_state_t state)
{
	assert(mutex_locked(&task->cap_info->lock));

	if ((handle < CAPS_START) || (handle > CAPS_LAST))
		return NULL;
	ht_link_t *link = hash_table_find(&task->cap_info->caps, &handle);
	if (!link)
		return NULL;
	cap_t *cap = hash_table_get_inst(link, cap_t, caps_link);
	if (cap->state != state)
		return NULL;
	return cap;
}

/** Allocate new capability
 *
 * @param task  Task for which to allocate the new capability.
 *
 * @param[out] handle  New capability handle on success.
 *
 * @return An error code in case of error.
 */
errno_t cap_alloc(task_t *task, cap_handle_t *handle)
{
	mutex_lock(&task->cap_info->lock);
	cap_t *cap = slab_alloc(cap_cache, FRAME_ATOMIC);
	if (!cap) {
		mutex_unlock(&task->cap_info->lock);
		return ENOMEM;
	}
	uintptr_t hbase;
	if (!ra_alloc(task->cap_info->handles, 1, 1, &hbase)) {
		slab_free(cap_cache, cap);
		mutex_unlock(&task->cap_info->lock);
		return ENOMEM;
	}
	cap_initialize(cap, task, (cap_handle_t) hbase);
	hash_table_insert(&task->cap_info->caps, &cap->caps_link);

	cap->state = CAP_STATE_ALLOCATED;
	*handle = cap->handle;
	mutex_unlock(&task->cap_info->lock);

	return EOK;
}

/** Publish allocated capability
 *
 * The kernel object is moved into the capability. In other words, its reference
 * is handed over to the capability. Once published, userspace can access and
 * manipulate the capability.
 *
 * @param task    Task in which to publish the capability.
 * @param handle  Capability handle.
 * @param kobj    Kernel object.
 */
void
cap_publish(task_t *task, cap_handle_t handle, kobject_t *kobj)
{
	mutex_lock(&task->cap_info->lock);
	cap_t *cap = cap_get(task, handle, CAP_STATE_ALLOCATED);
	assert(cap);
	cap->state = CAP_STATE_PUBLISHED;
	/* Hand over kobj's reference to cap */
	cap->kobject = kobj;
	list_append(&cap->type_link, &task->cap_info->type_list[kobj->type]);
	mutex_unlock(&task->cap_info->lock);
}

/** Unpublish published capability
 *
 * The kernel object is moved out of the capability. In other words, the
 * capability's reference to the objects is handed over to the kernel object
 * pointer returned by this function. Once unpublished, the capability does not
 * refer to any kernel object anymore.
 *
 * @param task    Task in which to unpublish the capability.
 * @param handle  Capability handle.
 * @param type    Kernel object type of the object associated with the
 *                capability.
 */
kobject_t *cap_unpublish(task_t *task, cap_handle_t handle, kobject_type_t type)
{
	kobject_t *kobj = NULL;

	mutex_lock(&task->cap_info->lock);
	cap_t *cap = cap_get(task, handle, CAP_STATE_PUBLISHED);
	if (cap) {
		if (cap->kobject->type == type) {
			/* Hand over cap's reference to kobj */
			kobj = cap->kobject;
			cap->kobject = NULL;
			list_remove(&cap->type_link);
			cap->state = CAP_STATE_ALLOCATED;
		}
	}
	mutex_unlock(&task->cap_info->lock);

	return kobj;
}

/** Free allocated capability
 *
 * @param task    Task in which to free the capability.
 * @param handle  Capability handle.
 */
void cap_free(task_t *task, cap_handle_t handle)
{
	assert(handle >= CAPS_START);
	assert(handle <= CAPS_LAST);

	mutex_lock(&task->cap_info->lock);
	cap_t *cap = cap_get(task, handle, CAP_STATE_ALLOCATED);

	assert(cap);

	hash_table_remove_item(&task->cap_info->caps, &cap->caps_link);
	ra_free(task->cap_info->handles, handle, 1);
	slab_free(cap_cache, cap);
	mutex_unlock(&task->cap_info->lock);
}

/** Initialize kernel object
 *
 * @param kobj  Kernel object to initialize.
 * @param type  Type of the kernel object.
 * @param raw   Raw pointer to the encapsulated object.
 * @param ops   Pointer to kernel object operations for the respective type.
 */
void kobject_initialize(kobject_t *kobj, kobject_type_t type, void *raw,
    kobject_ops_t *ops)
{
	atomic_set(&kobj->refcnt, 1);
	kobj->type = type;
	kobj->raw = raw;
	kobj->ops = ops;
}

/** Get new reference to kernel object from capability
 *
 * @param task    Task from which to get the reference.
 * @param handle  Capability handle.
 * @param type    Kernel object type of the object associated with the
 *                capability referenced by handle.
 *
 * @return Kernel object with incremented reference count on success.
 * @return NULL if there is no matching capability or kernel object.
 */
kobject_t *
kobject_get(struct task *task, cap_handle_t handle, kobject_type_t type)
{
	kobject_t *kobj = NULL;

	mutex_lock(&task->cap_info->lock);
	cap_t *cap = cap_get(task, handle, CAP_STATE_PUBLISHED);
	if (cap) {
		if (cap->kobject->type == type) {
			kobj = cap->kobject;
			atomic_inc(&kobj->refcnt);
		}
	}
	mutex_unlock(&task->cap_info->lock);

	return kobj;
}

/** Record new reference
 *
 * @param kobj  Kernel object from which the new reference is created.
 */
void kobject_add_ref(kobject_t *kobj)
{
	atomic_inc(&kobj->refcnt);
}

/** Drop reference to kernel object
 *
 * The encapsulated object and the kobject_t wrapper are both destroyed when the
 * last reference is dropped.
 *
 * @param kobj  Kernel object whose reference to drop.
 */
void kobject_put(kobject_t *kobj)
{
	if (atomic_postdec(&kobj->refcnt) == 1) {
		kobj->ops->destroy(kobj->raw);
		free(kobj);
	}
}

/** @}
 */
