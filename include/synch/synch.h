/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#ifndef __SYNCH_H__
#define __SYNCH_H__

#define SYNCH_NO_TIMEOUT	0	/**< No timeout is request. */
#define SYNCH_BLOCKING		0	/**< Blocking operation request. */
#define SYNCH_NON_BLOCKING	1	/**< Non-blocking operation request. */

#define ESYNCH_WOULD_BLOCK	1	/**< Could not satisfy the request without going to sleep. */
#define ESYNCH_TIMEOUT		2	/**< Timeout occurred. */
#define ESYNCH_OK_ATOMIC	4	/**< Operation succeeded without sleeping. */
#define ESYNCH_OK_BLOCKED	8	/**< Operation succeeded and did sleep. */

#define SYNCH_FAILED(rc)	((rc) & (ESYNCH_WOULD_BLOCK | ESYNCH_TIMEOUT))
#define SYNCH_OK(rc)		((rc) & (ESYNCH_OK_ATOMIC | ESYNCH_OK_BLOCKED))

#endif
