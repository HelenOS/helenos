/*
 * Copyright (c) 2010 Jiri Svoboda
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

#ifndef BUILTIN_H_
#define BUILTIN_H_

#include "mytypes.h"

void builtin_declare(stree_program_t *program);
void builtin_bind(builtin_t *bi);
void builtin_code_snippet(builtin_t *bi, const char *snippet);

stree_csi_t *builtin_get_gf_class(builtin_t *builtin);
void builtin_run_proc(run_t *run, stree_proc_t *proc);

rdata_var_t *builtin_get_self_mbr_var(run_t *run, const char *mbr_name);
void builtin_return_string(run_t *run, const char *str);

stree_symbol_t *builtin_declare_fun(stree_csi_t *csi, const char *name);
void builtin_fun_add_arg(stree_symbol_t *fun_sym, const char *name);

stree_symbol_t *builtin_find_lvl0(builtin_t *bi, const char *sym_name);
stree_symbol_t *builtin_find_lvl1(builtin_t *bi, const char *csi_name,
    const char *sym_name);

void builtin_fun_bind(builtin_t *bi, const char *csi_name,
    const char *sym_name, builtin_proc_t bproc);

#endif
