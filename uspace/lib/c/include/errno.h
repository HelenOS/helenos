/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_ERRNO_H_
#define LIBC_ERRNO_H_

#include <abi/errno.h>

#define errno  (*(__errno()))

extern int *__errno(void) __attribute__((const));

#define EMFILE        (-18)
#define ENAMETOOLONG  (-256)
#define EISDIR        (-257)
#define ENOTDIR       (-258)
#define ENOSPC        (-259)
#define EEXIST        (-260)
#define ENOTEMPTY     (-261)
#define EBADF         (-262)
#define ERANGE        (-263)
#define EXDEV         (-264)
#define EIO           (-265)
#define EMLINK        (-266)
#define ENXIO         (-267)

/** Bad checksum. */
#define EBADCHECKSUM  (-300)

/** USB: stalled operation. */
#define ESTALL (-301)

/** Empty resource (no data). */
#define EEMPTY (-302)

/** Negative acknowledgment. */
#define ENAK (-303)

/** An API function is called while another blocking function is in progress. */
#define EINPROGRESS  (-10036)

/** The socket identifier is not valid. */
#define ENOTSOCK  (-10038)

/** The destination address required. */
#define EDESTADDRREQ  (-10039)

/** Protocol is not supported.  */
#define EPROTONOSUPPORT  (-10043)

/** Socket type is not supported. */
#define ESOCKTNOSUPPORT  (-10044)

/** Protocol family is not supported. */
#define EPFNOSUPPORT  (-10046)

/** Address family is not supported. */
#define EAFNOSUPPORT  (-10047)

/** Address is already in use. */
#define EADDRINUSE  (-10048)

/** The socket is not connected or bound. */
#define ENOTCONN  (-10057)

#define ECONNREFUSED  (-10058)

#define ECONNABORTED  (-10059)

/** The requested operation was not performed. Try again later. */
#define EAGAIN  (-11002)

/** No data. */
#define NO_DATA (-11004)

#endif

/** @}
 */
