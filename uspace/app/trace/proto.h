/*
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup trace
 * @{
 */
/** @file
 */

#ifndef PROTO_H_
#define PROTO_H_

#include <ipc/common.h>
#include <adt/hash_table.h>
#include "trace.h"

#define OPER_MAX_ARGS (IPC_CALL_LEN - 1)

typedef struct {
	const char *name;

	int argc;
	val_type_t arg_type[OPER_MAX_ARGS];

	val_type_t rv_type;

	int respc;
	val_type_t resp_type[OPER_MAX_ARGS];
} oper_t;

typedef struct {
	/** Protocol name */
	const char *name;

	/** Maps method number to operation */
	hash_table_t method_oper;
} proto_t;


extern void proto_init(void);
extern void proto_cleanup(void);

extern void proto_register(int, proto_t *);
extern proto_t *proto_get_by_srv(int);
extern proto_t *proto_new(const char *);
extern void proto_delete(proto_t *);
extern void proto_add_oper(proto_t *, int, oper_t *);
extern oper_t *proto_get_oper(proto_t *, int);

extern oper_t *oper_new(const char *, int, val_type_t *, val_type_t, int,
    val_type_t *);

#endif

/** @}
 */
