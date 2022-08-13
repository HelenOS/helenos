/*
 * SPDX-FileCopyrightText: 2016 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
extern fibril_local int dl_public_fib_var;
extern fibril_local int dl_public_fib_uvar;

#endif

/**
 * @}
 */
