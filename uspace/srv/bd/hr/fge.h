/*
 * Copyright (c) 2025 Miroslav Cimerman
 * Copyright (c) 2024 Vojtech Horky
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

/** @addtogroup hr
 * @{
 */
/**
 * @file
 */

#ifndef _HR_FGE_H
#define _HR_FGE_H

#include <errno.h>
#include <stddef.h>

struct hr_fpool;
typedef struct hr_fpool hr_fpool_t;
struct hr_fgroup;
typedef struct hr_fgroup hr_fgroup_t;

typedef errno_t (*hr_wu_t)(void *);

extern hr_fpool_t *hr_fpool_create(size_t, size_t, size_t);
extern void hr_fpool_destroy(hr_fpool_t *);
extern hr_fgroup_t *hr_fgroup_create(hr_fpool_t *, size_t);
extern void *hr_fgroup_alloc(hr_fgroup_t *);
extern void hr_fgroup_submit(hr_fgroup_t *, hr_wu_t, void *);
extern errno_t hr_fgroup_wait(hr_fgroup_t *, size_t *, size_t *);

#endif

/** @}
 */
