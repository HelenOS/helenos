/*
 * Copyright (C) 2005 Martin Decky
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

#ifndef __ppc32_OFW_H__
#define __ppc32_OFW_H__

#include <arch/types.h>

#define MAX_OFW_ARGS	10

typedef __u32 ofw_arg_t;
typedef __u32 ihandle;
typedef __u32 phandle;

/** OpenFirmware command structure
 *
 */
typedef struct {
	const char *service;          /**< Command name */
	__u32 nargs;                  /**< Number of in arguments */
	__u32 nret;                   /**< Number of out arguments */
	ofw_arg_t args[MAX_OFW_ARGS]; /**< List of arguments */
} ofw_args_t;

typedef void (*ofw_entry)(ofw_args_t *);

extern ofw_entry ofw;

extern void ofw_init(void);
extern void ofw_done(void);
extern int ofw_call(const char *service, const int nargs, const int nret, ...);
extern void ofw_putchar(const char ch);
extern phandle ofw_find_device(const char *name);
extern int ofw_get_property(const phandle device, const char *name, void *buf, const int buflen);
extern void *ofw_claim(const void *addr, const int size, const int align);
extern void putchar(const char ch);

#endif
