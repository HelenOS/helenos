/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup libdltest
 * @{
 */
/** @file
 */

#ifndef LIBDLTEST_H
#define LIBDLTEST_H

#include <fibril.h>

enum {
	dl_constant = 110011,
	dl_private_var_val = 220022,
	dl_public_var_val = 330033,
	dl_private_fib_var_val = 440044,
	dl_public_fib_var_val = 550055
};

extern int dl_get_constant(void);
extern int dl_get_constant_via_call(void);
extern int dl_get_private_var(void);
extern int *dl_get_private_var_addr(void);
extern int dl_get_private_uvar(void);
extern int *dl_get_private_uvar_addr(void);
extern int dl_get_public_var(void);
extern int *dl_get_public_var_addr(void);
extern int dl_get_public_uvar(void);
extern int *dl_get_public_uvar_addr(void);
extern int dl_get_private_fib_var(void);
extern int *dl_get_private_fib_var_addr(void);
extern int dl_get_private_fib_uvar(void);
extern int *dl_get_private_fib_uvar_addr(void);
extern int dl_get_public_fib_var(void);
extern int *dl_get_public_fib_var_addr(void);
extern int dl_get_public_fib_uvar(void);
extern int *dl_get_public_fib_uvar_addr(void);

extern int dl_public_var;
extern int dl_public_uvar;
extern int *dl_public_ptr_var;
extern fibril_local int dl_public_fib_var;
extern fibril_local int dl_public_fib_uvar;

#endif

/**
 * @}
 */
