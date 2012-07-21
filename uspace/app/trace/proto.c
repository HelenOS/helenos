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

#include <stdio.h>
#include <stdlib.h>
#include <adt/hash_table.h>

#include "trace.h"
#include "proto.h"


/* Maps service number to protocol */
static hash_table_t srv_proto;

typedef struct {
	unsigned srv;
	proto_t *proto;
	link_t link;
} srv_proto_t;

typedef struct {
	sysarg_t method;
	oper_t *oper;
	link_t link;
} method_oper_t;




static size_t srv_proto_key_hash(unsigned long key[])
{
	return key[0];
}

static size_t srv_proto_hash(const link_t *item)
{
	srv_proto_t *sp = hash_table_get_instance(item, srv_proto_t, link);
	unsigned long key = sp->srv;
	return srv_proto_key_hash(&key);	
}

static bool srv_proto_match(unsigned long key[], size_t keys, const link_t *item)
{
	srv_proto_t *sp = hash_table_get_instance(item, srv_proto_t, link);

	return key[0] == sp->srv;
}

static hash_table_ops_t srv_proto_ops = {
	.hash = srv_proto_hash,
	.key_hash = srv_proto_key_hash,
	.match = srv_proto_match,
	.equal = 0,
	.remove_callback = 0
};


static size_t method_oper_key_hash(unsigned long key[])
{
	return key[0];
}

static size_t method_oper_hash(const link_t *item)
{
	method_oper_t *mo = hash_table_get_instance(item, method_oper_t, link);
	unsigned long key = mo->method;
	return method_oper_key_hash(&key);
}

static bool method_oper_match(unsigned long key[], size_t keys, 
	const link_t *item)
{
	method_oper_t *mo = hash_table_get_instance(item, method_oper_t, link);

	return key[0] == mo->method;
}

static hash_table_ops_t method_oper_ops = {
	.hash = method_oper_hash,
	.key_hash = method_oper_key_hash,
	.match = method_oper_match,
	.equal = 0,
	.remove_callback = 0
};


void proto_init(void)
{
	hash_table_create(&srv_proto, 0, 1, &srv_proto_ops);
}

void proto_cleanup(void)
{
	hash_table_destroy(&srv_proto);
}

void proto_register(int srv, proto_t *proto)
{
	srv_proto_t *sp;

	sp = malloc(sizeof(srv_proto_t));
	sp->srv = srv;
	sp->proto = proto;

	hash_table_insert(&srv_proto, &sp->link);
}

proto_t *proto_get_by_srv(int srv)
{
	link_t *item;
	srv_proto_t *sp;

	unsigned long key = srv;
	item = hash_table_find(&srv_proto, &key);
	if (item == NULL) return NULL;

	sp = hash_table_get_instance(item, srv_proto_t, link);
	return sp->proto;
}

static void proto_struct_init(proto_t *proto, const char *name)
{
	proto->name = name;
	hash_table_create(&proto->method_oper, 0, 1, &method_oper_ops);
}

proto_t *proto_new(const char *name)
{
	proto_t *p;

	p = malloc(sizeof(proto_t));
	proto_struct_init(p, name);

	return p;
}

void proto_delete(proto_t *proto)
{
	free(proto);
}

void proto_add_oper(proto_t *proto, int method, oper_t *oper)
{
	method_oper_t *mo;

	mo = malloc(sizeof(method_oper_t));
	mo->method = method;
	mo->oper = oper;

	hash_table_insert(&proto->method_oper, &mo->link);	
}

oper_t *proto_get_oper(proto_t *proto, int method)
{
	unsigned long key;
	link_t *item;
	method_oper_t *mo;

	key = method;
	item = hash_table_find(&proto->method_oper, &key);
	if (item == NULL) return NULL;

	mo = hash_table_get_instance(item, method_oper_t, link);
	return mo->oper;
}

static void oper_struct_init(oper_t *oper, const char *name)
{
	oper->name = name;
}

oper_t *oper_new(const char *name, int argc, val_type_t *arg_types,
    val_type_t rv_type, int respc, val_type_t *resp_types)
{
	oper_t *o;
	int i;

	o = malloc(sizeof(oper_t));
	oper_struct_init(o, name);

	o->argc = argc;
	for (i = 0; i < argc; i++)
		o->arg_type[i] = arg_types[i];

	o->rv_type = rv_type;

	o->respc = respc;
	for (i = 0; i < respc; i++)
		o->resp_type[i] = resp_types[i];

	return o;
}

/** @}
 */
