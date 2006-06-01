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

#ifndef __ERRNO_H__
#define __ERRNO_H__

/* 1-255 are kernel error codes, 256-512 are user error codes */

#define ENOENT		-1	/* No such entry */
#define ENOMEM		-2	/* Not enough memory */
#define ELIMIT		-3	/* Limit exceeded */
#define EREFUSED	-4	/* Connection refused */
#define EFORWARD	-5	/* Forward error */
#define EPERM		-6	/* Permission denied */
#define EHANGUP		-7	/* Answerbox closed connection, call sys_ipc_hangup
				 * to close the connection. Used by answerbox
				 * to close the connection.  */
#define EEXISTS		-8	/* Entry already exists */
#define EBADMEM		-9	/* Bad memory pointer */
#define ENOTSUP		-10	/* Not supported */
#define EADDRNOTAVAIL	-11	/* Address not available. */
#define ETIMEOUT        -12     /* Timeout expired */
#define EINVAL          -13     /* Invalid value */

#endif
