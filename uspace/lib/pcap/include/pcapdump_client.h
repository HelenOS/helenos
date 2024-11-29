/*
 * Copyright (c) 2023 Nataliia Korop
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

/**
 * @addtogroup libpcap
 * @{
 */
/**
 * @file
 *
 */

#ifndef _PCAPDUMP_CLIENT_H_
#define _PCAPDUMP_CLIENT_H_

#include <stdbool.h>
#include <stdio.h>
#include <async.h>
#include <loc.h>
#include <fibril_synch.h>

typedef struct {
	async_sess_t *sess;
} pcapctl_sess_t;

extern errno_t pcapctl_dump_open(int *, pcapctl_sess_t **);
extern errno_t pcapctl_dump_close(pcapctl_sess_t *);

extern errno_t pcapctl_dump_start(const char *, int *, pcapctl_sess_t *);
extern errno_t pcapctl_dump_stop(pcapctl_sess_t *);
extern errno_t pcapctl_list(void);
extern errno_t pcapctl_is_valid_device(int *);
extern errno_t pcapctl_is_valid_ops_number(int *, pcapctl_sess_t *);

#endif

/** @}
 */
