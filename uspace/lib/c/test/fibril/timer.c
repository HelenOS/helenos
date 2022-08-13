/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
