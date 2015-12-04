/*
 * Copyright (c) 2015 Michal Koutny
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
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

#ifndef SYSMAN_REPO_H
#define SYSMAN_REPO_H

#include <adt/list.h>
#include <ipc/sysman.h>
#include <stdio.h>

#include "unit.h"

#define ANONYMOUS_SERVICE_MASK "service_%" PRIu64

/*
 * If you access units out of the main event-loop fibril call repo_rlock(),
 * repo_runlock().
 */
extern list_t units;

extern void repo_init(void);

extern int repo_add_unit(unit_t *);
extern int repo_remove_unit(unit_t *);

extern void repo_begin_update(void);

extern void repo_commit(void);

extern void repo_rollback(void);

extern int repo_resolve_references(void);

extern unit_t *repo_find_unit_by_name(const char *);
extern unit_t *repo_find_unit_by_handle(unit_handle_t);

extern void repo_rlock(void);
extern void repo_runlock(void);

#endif


