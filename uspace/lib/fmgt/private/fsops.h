/*
 * Copyright (c) 2026 Jiri Svoboda
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

/** @addtogroup fmgt
 * @{
 */
/**
 * @file
 * @brief File management library - file system operations.
 */

#ifndef PRIVATE_FSOPS_H
#define PRIVATE_FSOPS_H

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include "../include/types/fmgt.h"

errno_t fmgt_open(fmgt_t *, const char *, int *);
errno_t fmgt_create_file(fmgt_t *, const char *, int *, fmgt_exists_action_t *);
errno_t fmgt_create_dir(fmgt_t *, const char *, bool);
errno_t fmgt_remove(fmgt_t *, const char *);
errno_t fmgt_read(fmgt_t *, int, const char *, aoff64_t *, void *, size_t,
    size_t *);
errno_t fmgt_write(fmgt_t *, int, const char *, aoff64_t *, void *, size_t);

#endif

/** @}
 */
