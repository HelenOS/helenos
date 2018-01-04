/*
 * Copyright (c) 2010 Lenka Trochtova
 * Copyright (c) 2013 Jiri Svoboda
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

/** @addtogroup devman
 * @{
 */

#ifndef FUN_H_
#define FUN_H_

#include "devman.h"

extern fun_node_t *create_fun_node(void);
extern void delete_fun_node(fun_node_t *);
extern void fun_add_ref(fun_node_t *);
extern void fun_del_ref(fun_node_t *);
extern void fun_busy_lock(fun_node_t *);
extern void fun_busy_unlock(fun_node_t *);
extern fun_node_t *find_fun_node_no_lock(dev_tree_t *, devman_handle_t);
extern fun_node_t *find_fun_node(dev_tree_t *, devman_handle_t);
extern fun_node_t *find_fun_node_by_path(dev_tree_t *, char *);
extern fun_node_t *find_fun_node_in_device(dev_tree_t *tree, dev_node_t *,
    const char *);
extern bool set_fun_path(dev_tree_t *, fun_node_t *, fun_node_t *);
extern errno_t fun_online(fun_node_t *);
extern errno_t fun_offline(fun_node_t *);

#endif

/** @}
 */
