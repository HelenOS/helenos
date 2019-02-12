/*
 * Copyright (c) 2006 Jakub Jermar
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

/*
 * This implementation of FIFO stores values in an array
 * (static or dynamic). As such, these FIFOs have upper bound
 * on number of values they can store. Push and pop operations
 * are done via accessing the array through head and tail indices.
 * Because of better operation ordering in fifo_pop(), the access
 * policy for these two indices is to 'increment (mod size of FIFO)
 * and use'.
 */

#ifndef _LIBC_FIFO_H_
#define _LIBC_FIFO_H_

#include <stdlib.h>

typedef unsigned long fifo_count_t;
typedef unsigned long fifo_index_t;

#define FIFO_CREATE_STATIC(name, t, itms) \
	struct { \
		t fifo[(itms)]; \
		fifo_count_t items; \
		fifo_index_t head; \
		fifo_index_t tail; \
	} name

/** Create and initialize static FIFO.
 *
 * FIFO is allocated statically.
 * This macro is suitable for creating smaller FIFOs.
 *
 * @param name Name of FIFO.
 * @param t Type of values stored in FIFO.
 * @param itms Number of items that can be stored in FIFO.
 */
#define FIFO_INITIALIZE_STATIC(name, t, itms)		\
	FIFO_CREATE_STATIC(name, t, itms) = {		\
		.items = (itms),			\
		.head = 0,				\
		.tail = 0				\
	}

/** Create and prepare dynamic FIFO.
 *
 * FIFO is allocated dynamically.
 * This macro is suitable for creating larger FIFOs.
 *
 * @param name Name of FIFO.
 * @param t Type of values stored in FIFO.
 * @param itms Number of items that can be stored in FIFO.
 */
#define FIFO_INITIALIZE_DYNAMIC(name, t, itms)		\
	struct {					\
		t *fifo;				\
		fifo_count_t items;			\
		fifo_index_t head;			\
		fifo_index_t tail;			\
	} name = {					\
		.fifo = NULL,				\
		.items = (itms),			\
		.head = 0,				\
		.tail = 0				\
	}

/** Pop value from head of FIFO.
 *
 * @param name FIFO name.
 *
 * @return Leading value in FIFO.
 */
#define fifo_pop(name) \
	name.fifo[name.head = (name.head + 1) < name.items ? (name.head + 1) : 0]

/** Push value to tail of FIFO.
 *
 * @param name FIFO name.
 * @param value Value to be appended to FIFO.
 *
 */
#define fifo_push(name, value) \
	name.fifo[name.tail = (name.tail + 1) < name.items ? (name.tail + 1) : 0] = (value)

/** Allocate memory for dynamic FIFO.
 *
 * @param name FIFO name.
 */
#define fifo_create(name) \
	name.fifo = malloc(sizeof(*name.fifo) * name.items)

#endif

/** @}
 */
