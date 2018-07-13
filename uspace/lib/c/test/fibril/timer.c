/*
 * Copyright (c) 2017 Jiri Svoboda
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

#include <async.h>
#include <fibril_synch.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(fibril_timer);

static void test_timeout_fn(void *arg)
{
	int *i;

	i = (int *)arg;
	++*i;
}

PCUT_TEST(create_destroy)
{
	fibril_timer_t *t;

	t = fibril_timer_create(NULL);
	PCUT_ASSERT_NOT_NULL(t);
	fibril_timer_destroy(t);
}

PCUT_TEST(create_destroy_user_lock)
{
	fibril_mutex_t lock;
	fibril_timer_t *t;

	fibril_mutex_initialize(&lock);
	t = fibril_timer_create(&lock);
	PCUT_ASSERT_NOT_NULL(t);
	fibril_timer_destroy(t);
}

PCUT_TEST(set_clear_locked)
{
	fibril_mutex_t lock;
	fibril_timer_t *t;
	fibril_timer_state_t fts;
	int cnt;

	fibril_mutex_initialize(&lock);
	t = fibril_timer_create(&lock);
	PCUT_ASSERT_NOT_NULL(t);

	fibril_mutex_lock(&lock);
	cnt = 0;

	fibril_timer_set_locked(t, 100 * 1000 * 1000, test_timeout_fn, &cnt);
	fibril_usleep(1000);
	fts = fibril_timer_clear_locked(t);
	PCUT_ASSERT_INT_EQUALS(fts_active, fts);

	PCUT_ASSERT_INT_EQUALS(0, cnt);
	fibril_mutex_unlock(&lock);

	fibril_timer_destroy(t);
}

PCUT_TEST(set_clear_not_locked)
{
	fibril_mutex_t lock;
	fibril_timer_t *t;
	fibril_timer_state_t fts;
	int cnt;

	fibril_mutex_initialize(&lock);
	t = fibril_timer_create(&lock);
	PCUT_ASSERT_NOT_NULL(t);

	cnt = 0;
	fibril_timer_set(t, 100 * 1000 * 1000, test_timeout_fn, &cnt);
	fibril_usleep(1000);
	fts = fibril_timer_clear(t);
	PCUT_ASSERT_INT_EQUALS(fts_active, fts);

	PCUT_ASSERT_INT_EQUALS(0, cnt);

	fibril_timer_destroy(t);
}

PCUT_TEST(fire)
{
	fibril_mutex_t lock;
	fibril_timer_t *t;
	fibril_timer_state_t fts;
	int cnt;

	fibril_mutex_initialize(&lock);
	t = fibril_timer_create(&lock);
	PCUT_ASSERT_NOT_NULL(t);

	fibril_mutex_lock(&lock);
	cnt = 0;

	fibril_timer_set_locked(t, 100, test_timeout_fn, &cnt);
	fibril_mutex_unlock(&lock);

	fibril_usleep(1000);

	fibril_mutex_lock(&lock);
	fts = fibril_timer_clear_locked(t);
	PCUT_ASSERT_INT_EQUALS(fts_fired, fts);

	PCUT_ASSERT_INT_EQUALS(1, cnt);
	fibril_mutex_unlock(&lock);

	fibril_timer_destroy(t);
}

PCUT_EXPORT(fibril_timer);
