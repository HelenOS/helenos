/*
 * Copyright (c) 2012 Adam Hraska
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

/** @addtogroup liburcu
 * @{
 */
/**
 * @file
 */

#ifndef LIBURCU_RCU_H_
#define LIBURCU_RCU_H_

#include <compiler/barrier.h>
#include <libarch/barrier.h>
#include <stdbool.h>

/** Use to assign a pointer to newly initialized data to a rcu reader
 * accessible pointer.
 *
 * Example:
 * @code
 * typedef struct exam {
 *     struct exam *next;
 *     int grade;
 * } exam_t;
 *
 * exam_t *exam_list;
 * // ..
 *
 * // Insert at the beginning of the list.
 * exam_t *my_exam = malloc(sizeof(exam_t), 0);
 * my_exam->grade = 5;
 * my_exam->next = exam_list;
 * rcu_assign(exam_list, my_exam);
 *
 * // Changes properly propagate. Every reader either sees
 * // the old version of exam_list or the new version with
 * // the fully initialized my_exam.
 * rcu_synchronize();
 * // Now we can be sure every reader sees my_exam.
 *
 * @endcode
 */
#define rcu_assign(ptr, value) \
	do { \
		memory_barrier(); \
		(ptr) = (value); \
	} while (0)

/** Use to access RCU protected data in a reader section.
 *
 * Example:
 * @code
 * exam_t *exam_list;
 * // ...
 *
 * rcu_read_lock();
 * exam_t *first_exam = rcu_access(exam_list);
 * // We can now safely use first_exam, it won't change
 * // under us while we're using it.
 *
 * // ..
 * rcu_read_unlock();
 * @endcode
 */
#define rcu_access(ptr) ACCESS_ONCE(ptr)

typedef enum blocking_mode {
	BM_BLOCK_FIBRIL,
	BM_BLOCK_THREAD
} blocking_mode_t;

extern void rcu_register_fibril(void);
extern void rcu_deregister_fibril(void);

extern void rcu_read_lock(void);
extern void rcu_read_unlock(void);

extern bool rcu_read_locked(void);

#define rcu_synchronize() _rcu_synchronize(BM_BLOCK_FIBRIL)

extern void _rcu_synchronize(blocking_mode_t);

#endif

/** @}
 */
