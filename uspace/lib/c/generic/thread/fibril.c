/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2007 Jakub Jermar
 * Copyright (c) 2018 CZ.NIC, z.s.p.o.
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

#include <adt/list.h>
#include <fibril.h>
#include <stack.h>
#include <tls.h>
#include <stdlib.h>
#include <as.h>
#include <context.h>
#include <assert.h>

#include <mem.h>
#include <str.h>
#include <ipc/ipc.h>
#include <libarch/faddr.h>

#include "../private/thread.h"
#include "../private/futex.h"
#include "../private/fibril.h"
#include "../private/libc.h"

#define DPRINTF(...) ((void)0)
#undef READY_DEBUG

/** Member of timeout_list. */
typedef struct {
	link_t link;
	struct timespec expires;
	fibril_event_t *event;
} _timeout_t;

typedef struct {
	errno_t rc;
	link_t link;
	ipc_call_t *call;
	fibril_event_t event;
} _ipc_waiter_t;

typedef struct {
	errno_t rc;
	link_t link;
	ipc_call_t call;
} _ipc_buffer_t;

typedef enum {
	SWITCH_FROM_DEAD,
	SWITCH_FROM_HELPER,
	SWITCH_FROM_YIELD,
	SWITCH_FROM_BLOCKED,
} _switch_type_t;

static bool multithreaded = false;

/* This futex serializes access to global data. */
static futex_t fibril_futex;
static futex_t ready_semaphore;
static long ready_st_count;

static LIST_INITIALIZE(ready_list);
static LIST_INITIALIZE(fibril_list);
static LIST_INITIALIZE(timeout_list);

static futex_t ipc_lists_futex;
static LIST_INITIALIZE(ipc_waiter_list);
static LIST_INITIALIZE(ipc_buffer_list);
static LIST_INITIALIZE(ipc_buffer_free_list);

/* Only used as unique markers for triggered events. */
static fibril_t _fibril_event_triggered;
static fibril_t _fibril_event_timed_out;
#define _EVENT_INITIAL   (NULL)
#define _EVENT_TRIGGERED (&_fibril_event_triggered)
#define _EVENT_TIMED_OUT (&_fibril_event_timed_out)

static inline void _ready_debug_check(void)
{
#ifdef READY_DEBUG
	assert(!multithreaded);
	long count = (long) list_count(&ready_list) +
	    (long) list_count(&ipc_buffer_free_list);
	assert(ready_st_count == count);
#endif
}

static inline void _ready_up(void)
{
	if (multithreaded) {
		futex_up(&ready_semaphore);
	} else {
		ready_st_count++;
		_ready_debug_check();
	}
}

static inline errno_t _ready_down(const struct timespec *expires)
{
	if (multithreaded)
		return futex_down_timeout(&ready_semaphore, expires);

	_ready_debug_check();
	ready_st_count--;
	return EOK;
}

static atomic_int threads_in_ipc_wait;

/** Function that spans the whole life-cycle of a fibril.
 *
 * Each fibril begins execution in this function. Then the function implementing
 * the fibril logic is called.  After its return, the return value is saved.
 * The fibril then switches to another fibril, which cleans up after it.
 *
 */
static void _fibril_main(void)
{
	/* fibril_futex is locked when a fibril is started. */
	futex_unlock(&fibril_futex);

	fibril_t *fibril = fibril_self();

	/* Call the implementing function. */
	fibril_exit(fibril->func(fibril->arg));

	/* Not reached */
}

/** Allocate a fibril structure and TCB, but don't do anything else with it. */
fibril_t *fibril_alloc(void)
{
	tcb_t *tcb = tls_make(__progsymbols.elfstart);
	if (!tcb)
		return NULL;

	fibril_t *fibril = calloc(1, sizeof(fibril_t));
	if (!fibril) {
		tls_free(tcb);
		return NULL;
	}

	tcb->fibril_data = fibril;
	fibril->tcb = tcb;
	fibril->is_freeable = true;

	fibril_setup(fibril);
	return fibril;
}

/**
 * Put the fibril into fibril_list.
 */
void fibril_setup(fibril_t *f)
{
	futex_lock(&fibril_futex);
	list_append(&f->all_link, &fibril_list);
	futex_unlock(&fibril_futex);
}

void fibril_teardown(fibril_t *fibril)
{
	futex_lock(&fibril_futex);
	list_remove(&fibril->all_link);
	futex_unlock(&fibril_futex);

	if (fibril->is_freeable) {
		tls_free(fibril->tcb);
		free(fibril);
	}
}

/**
 * Event notification with a given reason.
 *
 * @param reason  Reason of the notification.
 *                Can be either _EVENT_TRIGGERED or _EVENT_TIMED_OUT.
 */
static fibril_t *_fibril_trigger_internal(fibril_event_t *event, fibril_t *reason)
{
	assert(reason != _EVENT_INITIAL);
	assert(reason == _EVENT_TIMED_OUT || reason == _EVENT_TRIGGERED);

	futex_assert_is_locked(&fibril_futex);

	if (event->fibril == _EVENT_INITIAL) {
		event->fibril = reason;
		return NULL;
	}

	if (event->fibril == _EVENT_TIMED_OUT) {
		assert(reason == _EVENT_TRIGGERED);
		event->fibril = reason;
		return NULL;
	}

	if (event->fibril == _EVENT_TRIGGERED) {
		/* Already triggered. Nothing to do. */
		return NULL;
	}

	fibril_t *f = event->fibril;
	event->fibril = reason;

	assert(f->sleep_event == event);
	return f;
}

static errno_t _ipc_wait(ipc_call_t *call, const struct timespec *expires)
{
	if (!expires)
		return ipc_wait(call, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE);

	if (expires->tv_sec == 0)
		return ipc_wait(call, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NON_BLOCKING);

	struct timespec now;
	getuptime(&now);

	if (ts_gteq(&now, expires))
		return ipc_wait(call, SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NON_BLOCKING);

	return ipc_wait(call, NSEC2USEC(ts_sub_diff(expires, &now)),
	    SYNCH_FLAGS_NONE);
}

/*
 * Waits until a ready fibril is added to the list, or an IPC message arrives.
 * Returns NULL on timeout and may also return NULL if returning from IPC
 * wait after new ready fibrils are added.
 */
static fibril_t *_ready_list_pop(const struct timespec *expires, bool locked)
{
	if (locked) {
		futex_assert_is_locked(&fibril_futex);
		assert(expires);
		/* Must be nonblocking. */
		assert(expires->tv_sec == 0);
	} else {
		futex_assert_is_not_locked(&fibril_futex);
	}

	errno_t rc = _ready_down(expires);
	if (rc != EOK)
		return NULL;

	/*
	 * Once we acquire a token from ready_semaphore, there are two options.
	 * Either there is a ready fibril in the list, or it's our turn to
	 * call `ipc_wait_cycle()`. There is one extra token on the semaphore
	 * for each entry of the call buffer.
	 */

	if (!locked)
		futex_lock(&fibril_futex);
	fibril_t *f = list_pop(&ready_list, fibril_t, link);
	if (!f)
		atomic_fetch_add_explicit(&threads_in_ipc_wait, 1,
		    memory_order_relaxed);
	if (!locked)
		futex_unlock(&fibril_futex);

	if (f)
		return f;

	if (!multithreaded)
		assert(list_empty(&ipc_buffer_list));

	/* No fibril is ready, IPC wait it is. */
	ipc_call_t call = { 0 };
	rc = _ipc_wait(&call, expires);

	atomic_fetch_sub_explicit(&threads_in_ipc_wait, 1,
	    memory_order_relaxed);

	if (rc != EOK && rc != ENOENT) {
		/* Return token. */
		_ready_up();
		return NULL;
	}

	/*
	 * We might get ENOENT due to a poke.
	 * In that case, we propagate the null call out of fibril_ipc_wait(),
	 * because poke must result in that call returning.
	 */

	/*
	 * If a fibril is already waiting for IPC, we wake up the fibril,
	 * and return the token to ready_semaphore.
	 * If there is no fibril waiting, we pop a buffer bucket and
	 * put our call there. The token then returns when the bucket is
	 * returned.
	 */

	if (!locked)
		futex_lock(&fibril_futex);

	futex_lock(&ipc_lists_futex);

	_ipc_waiter_t *w = list_pop(&ipc_waiter_list, _ipc_waiter_t, link);
	if (w) {
		*w->call = call;
		w->rc = rc;
		/* We switch to the woken up fibril immediately if possible. */
		f = _fibril_trigger_internal(&w->event, _EVENT_TRIGGERED);

		/* Return token. */
		_ready_up();
	} else {
		_ipc_buffer_t *buf = list_pop(&ipc_buffer_free_list, _ipc_buffer_t, link);
		assert(buf);
		*buf = (_ipc_buffer_t) { .call = call, .rc = rc };
		list_append(&buf->link, &ipc_buffer_list);
	}

	futex_unlock(&ipc_lists_futex);

	if (!locked)
		futex_unlock(&fibril_futex);

	return f;
}

static fibril_t *_ready_list_pop_nonblocking(bool locked)
{
	struct timespec tv = { .tv_sec = 0, .tv_nsec = 0 };
	return _ready_list_pop(&tv, locked);
}

static void _ready_list_push(fibril_t *f)
{
	if (!f)
		return;

	futex_assert_is_locked(&fibril_futex);

	/* Enqueue in ready_list. */
	list_append(&f->link, &ready_list);
	_ready_up();

	if (atomic_load_explicit(&threads_in_ipc_wait, memory_order_relaxed)) {
		DPRINTF("Poking.\n");
		/* Wakeup one thread sleeping in SYS_IPC_WAIT. */
		ipc_poke();
	}
}

/* Blocks the current fibril until an IPC call arrives. */
static errno_t _wait_ipc(ipc_call_t *call, const struct timespec *expires)
{
	futex_assert_is_not_locked(&fibril_futex);

	futex_lock(&ipc_lists_futex);
	_ipc_buffer_t *buf = list_pop(&ipc_buffer_list, _ipc_buffer_t, link);
	if (buf) {
		*call = buf->call;
		errno_t rc = buf->rc;

		/* Return to freelist. */
		list_append(&buf->link, &ipc_buffer_free_list);
		/* Return IPC wait token. */
		_ready_up();

		futex_unlock(&ipc_lists_futex);
		return rc;
	}

	_ipc_waiter_t w = { .call = call };
	list_append(&w.link, &ipc_waiter_list);
	futex_unlock(&ipc_lists_futex);

	errno_t rc = fibril_wait_timeout(&w.event, expires);
	if (rc == EOK)
		return w.rc;

	futex_lock(&ipc_lists_futex);
	if (link_in_use(&w.link))
		list_remove(&w.link);
	else
		rc = w.rc;
	futex_unlock(&ipc_lists_futex);
	return rc;
}

/** Fire all timeouts that expired. */
static struct timespec *_handle_expired_timeouts(struct timespec *next_timeout)
{
	struct timespec ts;
	getuptime(&ts);

	futex_lock(&fibril_futex);

	while (!list_empty(&timeout_list)) {
		link_t *cur = list_first(&timeout_list);
		_timeout_t *to = list_get_instance(cur, _timeout_t, link);

		if (ts_gt(&to->expires, &ts)) {
			*next_timeout = to->expires;
			futex_unlock(&fibril_futex);
			return next_timeout;
		}

		list_remove(&to->link);

		_ready_list_push(_fibril_trigger_internal(
		    to->event, _EVENT_TIMED_OUT));
	}

	futex_unlock(&fibril_futex);
	return NULL;
}

/**
 * Clean up after a dead fibril from which we restored context, if any.
 * Called after a switch is made and fibril_futex is unlocked.
 */
static void _fibril_cleanup_dead(void)
{
	fibril_t *srcf = fibril_self();
	if (!srcf->clean_after_me)
		return;

	void *stack = srcf->clean_after_me->stack;
	assert(stack);
	as_area_destroy(stack);
	fibril_teardown(srcf->clean_after_me);
	srcf->clean_after_me = NULL;
}

/** Switch to a fibril. */
static void _fibril_switch_to(_switch_type_t type, fibril_t *dstf, bool locked)
{
	assert(fibril_self()->rmutex_locks == 0);

	if (!locked)
		futex_lock(&fibril_futex);
	else
		futex_assert_is_locked(&fibril_futex);

	fibril_t *srcf = fibril_self();
	assert(srcf);
	assert(dstf);

	switch (type) {
	case SWITCH_FROM_YIELD:
		_ready_list_push(srcf);
		break;
	case SWITCH_FROM_DEAD:
		dstf->clean_after_me = srcf;
		break;
	case SWITCH_FROM_HELPER:
	case SWITCH_FROM_BLOCKED:
		break;
	}

	dstf->thread_ctx = srcf->thread_ctx;
	srcf->thread_ctx = NULL;

	/* Just some bookkeeping to allow better debugging of futex locks. */
	futex_give_to(&fibril_futex, dstf);

	/* Swap to the next fibril. */
	context_swap(&srcf->ctx, &dstf->ctx);

	assert(srcf == fibril_self());
	assert(srcf->thread_ctx);

	if (!locked) {
		/* Must be after context_swap()! */
		futex_unlock(&fibril_futex);
		_fibril_cleanup_dead();
	}
}

/**
 * Main function for a helper fibril.
 * The helper fibril executes on threads in the lightweight fibril pool when
 * there is no fibril ready to run. Its only purpose is to block until
 * another fibril is ready, or a timeout expires, or an IPC message arrives.
 *
 * There is at most one helper fibril per thread.
 *
 */
static errno_t _helper_fibril_fn(void *arg)
{
	/* Set itself as the thread's own context. */
	fibril_self()->thread_ctx = fibril_self();

	(void) arg;

	struct timespec next_timeout;
	while (true) {
		struct timespec *to = _handle_expired_timeouts(&next_timeout);
		fibril_t *f = _ready_list_pop(to, false);
		if (f) {
			_fibril_switch_to(SWITCH_FROM_HELPER, f, false);
		}
	}

	return EOK;
}

/** Create a new fibril.
 *
 * @param func Implementing function of the new fibril.
 * @param arg Argument to pass to func.
 * @param stksz Stack size in bytes.
 *
 * @return 0 on failure or TLS of the new fibril.
 *
 */
fid_t fibril_create_generic(errno_t (*func)(void *), void *arg, size_t stksz)
{
	fibril_t *fibril;

	fibril = fibril_alloc();
	if (fibril == NULL)
		return 0;

	fibril->stack_size = stksz;
	fibril->stack = as_area_create(AS_AREA_ANY, fibril->stack_size,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE | AS_AREA_GUARD |
	    AS_AREA_LATE_RESERVE, AS_AREA_UNPAGED);
	if (fibril->stack == AS_MAP_FAILED) {
		fibril_teardown(fibril);
		return 0;
	}

	fibril->func = func;
	fibril->arg = arg;

	context_create_t sctx = {
		.fn = _fibril_main,
		.stack_base = fibril->stack,
		.stack_size = fibril->stack_size,
		.tls = fibril->tcb,
	};

	context_create(&fibril->ctx, &sctx);
	return (fid_t) fibril;
}

fid_t fibril_create(errno_t (*func)(void *), void *arg)
{
	return fibril_create_generic(func, arg, stack_size_get());
}

/** Delete a fibril that has never run.
 *
 * Free resources of a fibril that has been created with fibril_create()
 * but never started using fibril_start().
 *
 * @param fid Pointer to the fibril structure of the fibril to be
 *            added.
 */
void fibril_destroy(fid_t fid)
{
	fibril_t *fibril = (fibril_t *) fid;

	assert(!fibril->is_running);
	assert(fibril->stack);
	as_area_destroy(fibril->stack);
	fibril_teardown(fibril);
}

static void _insert_timeout(_timeout_t *timeout)
{
	futex_assert_is_locked(&fibril_futex);
	assert(timeout);

	link_t *tmp = timeout_list.head.next;
	while (tmp != &timeout_list.head) {
		_timeout_t *cur = list_get_instance(tmp, _timeout_t, link);

		if (ts_gteq(&cur->expires, &timeout->expires))
			break;

		tmp = tmp->next;
	}

	list_insert_before(&timeout->link, tmp);
}

/**
 * Same as `fibril_wait_for()`, except with a timeout.
 *
 * It is guaranteed that timing out cannot cause another thread's
 * `fibril_notify()` to be lost. I.e. the function returns success if and
 * only if `fibril_notify()` was called after the last call to
 * wait/wait_timeout returned, and before the call timed out.
 *
 * @return ETIMEOUT if timed out. EOK otherwise.
 */
errno_t fibril_wait_timeout(fibril_event_t *event,
    const struct timespec *expires)
{
	assert(fibril_self()->rmutex_locks == 0);

	DPRINTF("### Fibril %p sleeping on event %p.\n", fibril_self(), event);

	if (!fibril_self()->thread_ctx) {
		fibril_self()->thread_ctx =
		    fibril_create_generic(_helper_fibril_fn, NULL, PAGE_SIZE);
		if (!fibril_self()->thread_ctx)
			return ENOMEM;
	}

	futex_lock(&fibril_futex);

	if (event->fibril == _EVENT_TRIGGERED) {
		DPRINTF("### Already triggered. Returning. \n");
		event->fibril = _EVENT_INITIAL;
		futex_unlock(&fibril_futex);
		return EOK;
	}

	assert(event->fibril == _EVENT_INITIAL);

	fibril_t *srcf = fibril_self();
	fibril_t *dstf = NULL;

	/*
	 * We cannot block here waiting for another fibril becoming
	 * ready, since that would require unlocking the fibril_futex,
	 * and that in turn would allow another thread to restore
	 * the source fibril before this thread finished switching.
	 *
	 * Instead, we switch to an internal "helper" fibril whose only
	 * job is to wait for an event, freeing the source fibril for
	 * wakeups. There is always one for each running thread.
	 */

	dstf = _ready_list_pop_nonblocking(true);
	if (!dstf) {
		// XXX: It is possible for the _ready_list_pop_nonblocking() to
		//      check for IPC, find a pending message, and trigger the
		//      event on which we are currently trying to sleep.
		if (event->fibril == _EVENT_TRIGGERED) {
			event->fibril = _EVENT_INITIAL;
			futex_unlock(&fibril_futex);
			return EOK;
		}

		dstf = srcf->thread_ctx;
		assert(dstf);
	}

	_timeout_t timeout = { 0 };
	if (expires) {
		timeout.expires = *expires;
		timeout.event = event;
		_insert_timeout(&timeout);
	}

	assert(srcf);

	event->fibril = srcf;
	srcf->sleep_event = event;

	assert(event->fibril != _EVENT_INITIAL);

	_fibril_switch_to(SWITCH_FROM_BLOCKED, dstf, true);

	assert(event->fibril != srcf);
	assert(event->fibril != _EVENT_INITIAL);
	assert(event->fibril == _EVENT_TIMED_OUT || event->fibril == _EVENT_TRIGGERED);

	list_remove(&timeout.link);
	errno_t rc = (event->fibril == _EVENT_TIMED_OUT) ? ETIMEOUT : EOK;
	event->fibril = _EVENT_INITIAL;

	futex_unlock(&fibril_futex);
	_fibril_cleanup_dead();
	return rc;
}

void fibril_wait_for(fibril_event_t *event)
{
	assert(fibril_self()->rmutex_locks == 0);

	(void) fibril_wait_timeout(event, NULL);
}

/**
 * Wake up the fibril waiting for the given event.
 * Up to one wakeup is remembered if the fibril is not currently waiting.
 *
 * This function is safe for use under restricted mutex lock.
 */
void fibril_notify(fibril_event_t *event)
{
	futex_lock(&fibril_futex);
	_ready_list_push(_fibril_trigger_internal(event, _EVENT_TRIGGERED));
	futex_unlock(&fibril_futex);
}

/** Start a fibril that has not been running yet. */
void fibril_start(fibril_t *fibril)
{
	futex_lock(&fibril_futex);
	assert(!fibril->is_running);
	fibril->is_running = true;

	if (!link_in_use(&fibril->all_link))
		list_append(&fibril->all_link, &fibril_list);

	_ready_list_push(fibril);

	futex_unlock(&fibril_futex);
}

/** Start a fibril that has not been running yet. (obsolete) */
void fibril_add_ready(fibril_t *fibril)
{
	fibril_start(fibril);
}

/** @return the currently running fibril. */
fibril_t *fibril_self(void)
{
	assert(__tcb_is_set());
	tcb_t *tcb = __tcb_get();
	assert(tcb->fibril_data);
	return tcb->fibril_data;
}

/**
 * Obsolete, use fibril_self().
 *
 * @return ID of the currently running fibril.
 */
fid_t fibril_get_id(void)
{
	return (fid_t) fibril_self();
}

/**
 * Switch to another fibril, if one is ready to run.
 * Has no effect on a heavy fibril.
 */
void fibril_yield(void)
{
	if (fibril_self()->rmutex_locks > 0)
		return;

	fibril_t *f = _ready_list_pop_nonblocking(false);
	if (f)
		_fibril_switch_to(SWITCH_FROM_YIELD, f, false);
}

static errno_t _runner_fn(void *arg)
{
	_helper_fibril_fn(arg);
	return EOK;
}

/**
 * Spawn a given number of runners (i.e. OS threads) immediately, and
 * unconditionally. This is meant to be used for tests and debugging.
 * Regular programs should just use `fibril_enable_multithreaded()`.
 *
 * @param n  Number of runners to spawn.
 * @return   Number of runners successfully spawned.
 */
int fibril_test_spawn_runners(int n)
{
	assert(fibril_self()->rmutex_locks == 0);

	if (!multithreaded) {
		_ready_debug_check();
		if (futex_initialize(&ready_semaphore, ready_st_count) != EOK)
			abort();
		multithreaded = true;
	}

	errno_t rc;

	for (int i = 0; i < n; i++) {
		rc = thread_create(_runner_fn, NULL, "fibril runner");
		if (rc != EOK)
			return i;
	}

	return n;
}

/**
 * Opt-in to have more than one runner thread.
 *
 * Currently, a task only ever runs in one thread because multithreading
 * might break some existing code.
 *
 * Eventually, the number of runner threads for a given task should become
 * configurable in the environment and this function becomes no-op.
 */
void fibril_enable_multithreaded(void)
{
	// TODO: Implement better.
	//       For now, 4 total runners is a sensible default.
	if (!multithreaded) {
		fibril_test_spawn_runners(3);
	}
}

/**
 * Detach a fibril.
 */
void fibril_detach(fid_t f)
{
	// TODO: Currently all fibrils are detached by default, but they
	//       won't always be. Code that explicitly spawns fibrils with
	//       limited lifetime should call this function.
}

/**
 * Exit a fibril. Never returns.
 *
 * @param retval  Value to return from fibril_join() called on this fibril.
 */
_Noreturn void fibril_exit(long retval)
{
	// TODO: implement fibril_join() and remember retval
	(void) retval;

	fibril_t *f = _ready_list_pop_nonblocking(false);
	if (!f)
		f = fibril_self()->thread_ctx;

	_fibril_switch_to(SWITCH_FROM_DEAD, f, false);
	__builtin_unreachable();
}

void __fibrils_init(void)
{
	if (futex_initialize(&fibril_futex, 1) != EOK)
		abort();
	if (futex_initialize(&ipc_lists_futex, 1) != EOK)
		abort();

	/*
	 * We allow a fixed, small amount of parallelism for IPC reads, but
	 * since IPC is currently serialized in kernel, there's not much
	 * we can get from more threads reading messages.
	 */

#define IPC_BUFFER_COUNT 1024
	static _ipc_buffer_t buffers[IPC_BUFFER_COUNT];

	for (int i = 0; i < IPC_BUFFER_COUNT; i++) {
		list_append(&buffers[i].link, &ipc_buffer_free_list);
		_ready_up();
	}
}

void __fibrils_fini(void)
{
	futex_destroy(&fibril_futex);
	futex_destroy(&ipc_lists_futex);
}

void fibril_usleep(usec_t timeout)
{
	struct timespec expires;
	getuptime(&expires);
	ts_add_diff(&expires, USEC2NSEC(timeout));

	fibril_event_t event = FIBRIL_EVENT_INIT;
	fibril_wait_timeout(&event, &expires);
}

void fibril_sleep(sec_t sec)
{
	struct timespec expires;
	getuptime(&expires);
	expires.tv_sec += sec;

	fibril_event_t event = FIBRIL_EVENT_INIT;
	fibril_wait_timeout(&event, &expires);
}

void fibril_ipc_poke(void)
{
	DPRINTF("Poking.\n");
	/* Wakeup one thread sleeping in SYS_IPC_WAIT. */
	ipc_poke();
}

errno_t fibril_ipc_wait(ipc_call_t *call, const struct timespec *expires)
{
	return _wait_ipc(call, expires);
}

/** @}
 */
