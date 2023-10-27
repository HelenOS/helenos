/*
 * Copyright (c) 2016 Jiri Svoboda
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
