/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup libofw
 * @{
 */
/**
 * @file OpenFirmware device tree access
 */

#ifndef _OFW_H
#define _OFW_H

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <types/ofw.h>

extern errno_t ofw_child_it_first(ofw_child_it_t *, const char *);
extern void ofw_child_it_next(ofw_child_it_t *);
extern bool ofw_child_it_end(ofw_child_it_t *);
extern const char *ofw_child_it_get_name(ofw_child_it_t *);
extern errno_t ofw_child_it_get_path(ofw_child_it_t *, char **);
extern void ofw_child_it_fini(ofw_child_it_t *);

extern errno_t ofw_prop_it_first(ofw_prop_it_t *, const char *);
extern void ofw_prop_it_next(ofw_prop_it_t *);
extern bool ofw_prop_it_end(ofw_prop_it_t *);
extern const char *ofw_prop_it_get_name(ofw_prop_it_t *);
extern const void *ofw_prop_it_get_data(ofw_prop_it_t *, size_t *);
extern void ofw_prop_it_fini(ofw_prop_it_t *);

#endif

/** @}
 */
