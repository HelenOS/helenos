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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_IO_TABLE_H_
#define LIBC_IO_TABLE_H_

#include <adt/list.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>

/** Table metrics */
typedef struct {
	/** Space to the left of the table */
	size_t margin_left;
} table_metrics_t;

/** Table cell */
typedef struct {
	/** Containing row */
	struct table_row *row;
	/** Link to table_row_t.cells */
	link_t lrow;
	/** Cell text */
	char *text;
} table_cell_t;

/** Table row */
typedef struct table_row {
	/** Containing table */
	struct table *table;
	/** Link to table_t.rows */
	link_t ltable;
	/** Cells of this row */
	list_t cells;
} table_row_t;

/** Table column */
typedef struct {
	/** Containing table */
	struct table *table;
	/** Link to table_t.columns */
	link_t ltable;
	/** Column width */
	size_t width;
} table_column_t;

/** Table */
typedef struct table {
	/** @c true if the first row is a header row */
	bool header_row;
	/** Encountered error while writing to table */
	errno_t error;
	/** Table rows */
	list_t rows; /* of table_row_t */
	/** Table columns */
	list_t columns; /* of table_column_t */
	/** Current row */
	table_row_t *wrow;
	/** Current cell */
	table_cell_t *wcell;
	/** Current column */
	table_column_t *wcolumn;
	/** Table metrics */
	table_metrics_t metrics;
} table_t;

extern errno_t table_create(table_t **);
extern void table_destroy(table_t *);
extern errno_t table_print_out(table_t *, FILE *);
extern void table_header_row(table_t *);
extern errno_t table_printf(table_t *, const char *, ...);
extern errno_t table_get_error(table_t *);
extern void table_set_margin_left(table_t *, size_t);

#endif

/** @}
 */
