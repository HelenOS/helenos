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
/**
 * @file
 * @brief Tabulate text
 */

#include <errno.h>
#include <io/table.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>

static table_column_t *table_column_first(table_t *);
static table_column_t *table_column_next(table_column_t *);

/** Add a new row at the end of the table.
 *
 * @param table Table
 * @param rrow Place to store pointer to new row or @c NULL
 *
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t table_add_row(table_t *table, table_row_t **rrow)
{
	table_row_t *row;

	row = calloc(1, sizeof(table_row_t));
	if (row == NULL)
		return ENOMEM;

	row->table = table;
	list_initialize(&row->cells);
	list_append(&row->ltable, &table->rows);

	if (rrow != NULL)
		*rrow = row;
	return EOK;
}

/** Add a new cell at the end of the row.
 *
 * @param row Row
 * @param rcell Place to store pointer to new cell or @c NULL
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t table_row_add_cell(table_row_t *row, table_cell_t **rcell)
{
	table_cell_t *cell;

	cell = calloc(1, sizeof(table_cell_t));
	if (cell == NULL)
		return ENOMEM;

	cell->text = NULL;

	cell->row = row;
	list_append(&cell->lrow, &row->cells);

	if (rcell != NULL)
		*rcell = cell;
	return EOK;
}

/** Add a new column at the right side of the table.
 *
 * @param table Table
 * @param rcolumn Place to store pointer to new column or @c NULL
 *
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t table_add_column(table_t *table, table_column_t **rcolumn)
{
	table_column_t *column;

	column = calloc(1, sizeof(table_column_t));
	if (column == NULL)
		return ENOMEM;

	column->table = table;
	column->width = 0;
	list_append(&column->ltable, &table->columns);

	if (rcolumn != NULL)
		*rcolumn = column;
	return EOK;
}

/** Start writing next table cell.
 *
 * @param table Table
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t table_write_next_cell(table_t *table)
{
	errno_t rc;

	rc = table_row_add_cell(table->wrow, &table->wcell);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		return rc;
	}

	if (list_count(&table->wrow->cells) == 1) {
		/* First column */
		table->wcolumn = table_column_first(table);
	} else {
		/* Next column */
		table->wcolumn = table_column_next(table->wcolumn);
	}

	if (table->wcolumn == NULL) {
		rc = table_add_column(table, &table->wcolumn);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			return rc;
		}
	}

	return EOK;
}

/** Start writing next table row.
 *
 * @param table Table
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t table_write_next_row(table_t *table)
{
	errno_t rc;

	rc = table_add_row(table, &table->wrow);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		return rc;
	}

	table->wcell = NULL;
	return EOK;
}

/** Get first table row.
 *
 * @param table Table
 * @return First table row or @c NULL if none
 */
static table_row_t *table_row_first(table_t *table)
{
	link_t *link;

	link = list_first(&table->rows);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, table_row_t, ltable);
}

/** Get next table row.
 *
 * @param cur Current row
 * @return Next table row or @c NULL if none
 */
static table_row_t *table_row_next(table_row_t *cur)
{
	link_t *link;

	link = list_next(&cur->ltable, &cur->table->rows);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, table_row_t, ltable);
}

/** Get first cell in table row.
 *
 * @param row Table row
 * @return First cell in row or @c NULL if none
 */
static table_cell_t *table_row_cell_first(table_row_t *row)
{
	link_t *link;

	link = list_first(&row->cells);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, table_cell_t, lrow);
}

/** Get next cell in table row.
 *
 * @param cur Current cell
 * @return Next cell in table row or @c NULL if none
 */
static table_cell_t *table_row_cell_next(table_cell_t *cur)
{
	link_t *link;

	link = list_next(&cur->lrow, &cur->row->cells);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, table_cell_t, lrow);
}

/** Get first table column.
 *
 * @param table Table
 * @return First table column or @c NULL if none
 */
static table_column_t *table_column_first(table_t *table)
{
	link_t *link;

	link = list_first(&table->columns);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, table_column_t, ltable);
}

/** Get next table column.
 *
 * @param cur Current column
 * @return Next table column or @c NULL if none
 */
static table_column_t *table_column_next(table_column_t *cur)
{
	link_t *link;

	link = list_next(&cur->ltable, &cur->table->columns);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, table_column_t, ltable);
}

/** Append to cell text.
 *
 * @param cell Cell
 * @param str Text to append
 * @param len Max number of bytes to read from str
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t table_cell_extend(table_cell_t *cell, const char *str, size_t len)
{
	char *cstr;
	int rc;

	if (cell->text == NULL) {
		cell->text = str_ndup(str, len);
		if (cell->text == NULL)
			return ENOMEM;
	} else {
		rc = asprintf(&cstr, "%s%.*s", cell->text, (int)len, str);
		if (rc < 0)
			return ENOMEM;

		free(cell->text);
		cell->text = cstr;
	}

	return EOK;
}

/** Create table.
 *
 * @para, rtable Place to store pointer to new table.
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t table_create(table_t **rtable)
{
	table_t *table;
	errno_t rc;

	table = calloc(1, sizeof(table_t));
	if (table == NULL)
		return ENOMEM;

	table->error = EOK;
	list_initialize(&table->rows);
	list_initialize(&table->columns);

	rc = table_add_row(table, &table->wrow);
	if (rc != EOK)
		goto error;

	rc = table_row_add_cell(table->wrow, &table->wcell);
	if (rc != EOK)
		goto error;

	rc = table_add_column(table, &table->wcolumn);
	if (rc != EOK)
		goto error;

	*rtable = table;
	return EOK;
error:
	table_destroy(table);
	return rc;
}

/** Destroy table.
 *
 * @param table Table
 */
void table_destroy(table_t *table)
{
	table_row_t *row;
	table_cell_t *cell;
	table_column_t *column;

	if (table == NULL)
		return;

	row = table_row_first(table);
	while (row != NULL) {
		cell = table_row_cell_first(row);
		while (cell != NULL) {
			list_remove(&cell->lrow);
			free(cell->text);
			free(cell);
			cell = table_row_cell_first(row);
		}

		list_remove(&row->ltable);
		free(row);

		row = table_row_first(table);
	}

	column = table_column_first(table);
	while (column != NULL) {
		list_remove(&column->ltable);
		free(column);

		column = table_column_first(table);
	}

	free(table);
}

/** Print out table contents to a file stream.
 *
 * @param table Table
 * @param f File where to write the output
 *
 * @return EOK on success, EIO on I/O error
 */
errno_t table_print_out(table_t *table, FILE *f)
{
	table_row_t *row;
	table_cell_t *cell;
	table_column_t *column;
	bool firstc;
	bool firstr;
	size_t spacing;
	size_t i;
	int rc;

	if (table->error != EOK)
		return table->error;

	row = table_row_first(table);
	firstr = true;
	while (row != NULL) {
		cell = table_row_cell_first(row);
		if (cell == NULL)
			break;

		column = table_column_first(table);
		firstc = true;

		while (cell != NULL && cell->text != NULL) {
			spacing = firstc ? table->metrics.margin_left : 1;
			for (i = 0; i < spacing; i++) {
				rc = fprintf(f, " ");
				if (rc < 0)
					return EIO;
			}

			rc = fprintf(f, "%*s", -(int)column->width, cell->text);
			if (rc < 0)
				return EIO;

			cell = table_row_cell_next(cell);
			column = table_column_next(column);
			firstc = false;
		}
		rc = fprintf(f, "\n");
		if (rc < 0)
			return EIO;

		if (firstr && table->header_row) {
			/* Display header separator */
			column = table_column_first(table);
			firstc = true;
			while (column != NULL) {
				spacing = firstc ? table->metrics.margin_left : 1;
				for (i = 0; i < spacing; i++) {
					rc = fprintf(f, " ");
					if (rc < 0)
						return EIO;
				}

				for (i = 0; i < column->width; i++) {
					rc = fprintf(f, "=");
					if (rc < 0)
						return EIO;
				}

				column = table_column_next(column);
				firstc = false;
			}

			rc = fprintf(f, "\n");
			if (rc < 0)
				return EIO;
		}

		row = table_row_next(row);
		firstr = false;
	}
	return EOK;
}

/** Start a header row.
 *
 * @param table Table
 */
void table_header_row(table_t *table)
{
	assert(list_count(&table->rows) == 1);
	assert(!table->header_row);
	table->header_row = true;
}

/** Insert text into table cell(s).
 *
 * Appends text to the current cell. A tab character starts a new cell.
 * A newline character starts a new row.
 *
 * @param table Table
 * @param fmt Format string
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t table_printf(table_t *table, const char *fmt, ...)
{
	va_list args;
	errno_t rc;
	int ret;
	char *str;
	char *sp, *ep;
	size_t width;

	if (table->error != EOK)
		return table->error;

	va_start(args, fmt);
	ret = vasprintf(&str, fmt, args);
	va_end(args);

	if (ret < 0) {
		table->error = ENOMEM;
		return table->error;
	}

	sp = str;
	while (*sp != '\0' && table->error == EOK) {
		ep = sp;
		while (*ep != '\0' && *ep != '\t' && *ep != '\n')
			++ep;

		if (table->wcell == NULL) {
			rc = table_write_next_cell(table);
			if (rc != EOK) {
				assert(rc == ENOMEM);
				goto out;
			}
		}

		rc = table_cell_extend(table->wcell, sp, ep - sp);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			table->error = ENOMEM;
			goto out;
		}

		/* Update column width */
		width = str_width(table->wcell->text);
		if (width > table->wcolumn->width)
			table->wcolumn->width = width;

		if (*ep == '\t')
			rc = table_write_next_cell(table);
		else if (*ep == '\n')
			rc = table_write_next_row(table);
		else
			break;

		if (rc != EOK) {
			assert(rc == ENOMEM);
			table->error = ENOMEM;
			goto out;
		}

		sp = ep + 1;
	}

	rc = table->error;
out:
	free(str);
	return rc;
}

/** Return table error code.
 *
 * @param table Table
 * @return EOK if no error indicated, non-zero error code otherwise
 */
errno_t table_get_error(table_t *table)
{
	return table->error;
}

/** Set left table margin.
 *
 * @param table Table
 * @param mleft Left margin
 */
void table_set_margin_left(table_t *table, size_t mleft)
{
	table->metrics.margin_left = mleft;
}

/** @}
 */
