/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_adt
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

#ifndef KERN_FIFO_H_
#define KERN_FIFO_H_

#include <stdlib.h>

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
	struct {					\
		t fifo[(itms)];				\
		size_t items;				\
		size_t head;				\
		size_t tail;				\
	} name = {					\
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
		size_t items;				\
		size_t head;				\
		size_t tail;				\
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
	name.fifo[name.tail = \
	    (name.tail + 1) < name.items ? (name.tail + 1) : 0] = (value)

/** Allocate memory for dynamic FIFO.
 *
 * @param name FIFO name.
 */
#define fifo_create(name) \
	name.fifo = malloc(sizeof(*name.fifo) * name.items)

#endif

/** @}
 */
