/*
 * SPDX-FileCopyrightText: 2016 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_adt
 * @{
 */
/** @file
 */

#ifndef _LIBC_ODICT_H_
#define _LIBC_ODICT_H_

#include <errno.h>
#include <member.h>
#include <stdbool.h>
#include <stddef.h>
#include <types/adt/odict.h>

#define odict_get_instance(odlink, type, member) \
	member_to_inst(odlink, type, member)

extern void odict_initialize(odict_t *, odgetkey_t, odcmp_t);
extern void odict_finalize(odict_t *);
extern void odlink_initialize(odlink_t *);
extern void odict_insert(odlink_t *, odict_t *, odlink_t *);
extern void odict_remove(odlink_t *);
extern void odict_key_update(odlink_t *, odict_t *);
extern bool odlink_used(odlink_t *);
extern bool odict_empty(odict_t *);
extern unsigned long odict_count(odict_t *);
extern odlink_t *odict_first(odict_t *);
extern odlink_t *odict_last(odict_t *);
extern odlink_t *odict_prev(odlink_t *, odict_t *);
extern odlink_t *odict_next(odlink_t *, odict_t *);
extern odlink_t *odict_find_eq(odict_t *, void *, odlink_t *);
extern odlink_t *odict_find_eq_last(odict_t *, void *, odlink_t *);
extern odlink_t *odict_find_geq(odict_t *, void *, odlink_t *);
extern odlink_t *odict_find_gt(odict_t *, void *, odlink_t *);
extern odlink_t *odict_find_leq(odict_t *, void *, odlink_t *);
extern odlink_t *odict_find_lt(odict_t *, void *, odlink_t *);
extern errno_t odict_validate(odict_t *);

#endif

/** @}
 */
