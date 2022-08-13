/*
 * SPDX-FileCopyrightText: 2009 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup edit
 * @{
 */
/**
 * @file
 */

#ifndef SHEET_H__
#define SHEET_H__

#include <adt/list.h>
#include <stddef.h>
#include <stdbool.h>

/** Direction (in linear space) */
enum dir_spec {
	/** Before specified point */
	dir_before,
	/** After specified point */
	dir_after
};

/** Sheet */
struct sheet;
typedef struct sheet sheet_t;

/** Character cell coordinates
 *
 * These specify a character cell. The first cell is (1,1).
 */
typedef struct {
	int row;
	int column;
} coord_t;

/** S-point
 *
 * An s-point specifies the boundary between two successive characters
 * in the linear file space (including the beginning of file or the end
 * of file. An s-point only remains valid as long as no modifications
 * (insertions/deletions) are performed on the sheet.
 */
typedef struct {
	/* Note: This structure is opaque for the user. */
	sheet_t *sh;
	size_t b_off;
} spt_t;

/** Tag
 *
 * A tag is similar to an s-point, but remains valid over modifications
 * to the sheet. A tag tends to 'stay put'. Any tag must be properly
 * removed from the sheet before it is deallocated by the user.
 */
typedef struct {
	/* Note: This structure is opaque for the user. */

	/** Link to list of all tags in the sheet (see sheet_t.tags) */
	link_t link;
	sheet_t *sh;
	size_t b_off;
} tag_t;

extern errno_t sheet_create(sheet_t **);
extern errno_t sheet_insert(sheet_t *, spt_t *, enum dir_spec, char *);
extern errno_t sheet_delete(sheet_t *, spt_t *, spt_t *);
extern void sheet_copy_out(sheet_t *, spt_t const *, spt_t const *, char *,
    size_t, spt_t *);
extern void sheet_get_cell_pt(sheet_t *, coord_t const *, enum dir_spec,
    spt_t *);
extern void sheet_get_row_width(sheet_t *, int, int *);
extern void sheet_get_num_rows(sheet_t *, int *);
extern void spt_get_coord(spt_t const *, coord_t *);
extern bool spt_equal(spt_t const *, spt_t const *);
extern char32_t spt_next_char(spt_t, spt_t *);
extern char32_t spt_prev_char(spt_t, spt_t *);

extern void sheet_place_tag(sheet_t *, spt_t const *, tag_t *);
extern void sheet_remove_tag(sheet_t *, tag_t *);
extern void tag_get_pt(tag_t const *, spt_t *);

#endif

/** @}
 */
