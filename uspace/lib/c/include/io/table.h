/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IO_TABLE_H_
#define _LIBC_IO_TABLE_H_

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
