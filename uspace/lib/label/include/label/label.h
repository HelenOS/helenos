/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup liblabel
 * @{
 */
/**
 * @file Disk label library.
 */

#ifndef LIBLABEL_LABEL_H_
#define LIBLABEL_LABEL_H_

#include <types/label.h>
#include <types/liblabel.h>

extern errno_t label_open(label_bd_t *, label_t **);
extern errno_t label_create(label_bd_t *, label_type_t, label_t **);
extern void label_close(label_t *);
extern errno_t label_destroy(label_t *);
extern errno_t label_get_info(label_t *, label_info_t *);

extern label_part_t *label_part_first(label_t *);
extern label_part_t *label_part_next(label_part_t *);
extern void label_part_get_info(label_part_t *, label_part_info_t *);

extern errno_t label_part_create(label_t *, label_part_spec_t *,
    label_part_t **);
extern errno_t label_part_destroy(label_part_t *);
extern void label_pspec_init(label_part_spec_t *);
extern errno_t label_suggest_ptype(label_t *, label_pcnt_t, label_ptype_t *);

#endif

/** @}
 */
