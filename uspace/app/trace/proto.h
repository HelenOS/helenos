/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
