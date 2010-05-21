/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup net
 *  @{
 */

/** @file
 *  Socket error codes.
 *  Based on BSD.
 */

#ifndef __NET_SOCKET_ERR_H__
#define __NET_SOCKET_ERR_H__

#include <errno.h>

/** @name Socket error codes definitions
 */
/*@{*/

////#define EINTR			(-10004)
////#define EBADF			(-10009)
//#define EACCES			(-10013)
//#define EFAULT			(-10014)
////#define EINVAL			(-10022)
////#define EMFILE			(-10024)
//#define EWOULDBLOCK		(-10035)

/** An API function is called while another blocking function is in progress.
 */
#define EINPROGRESS		(-10036)

//#define EALREADY		(-10037)

/** The socket identifier is not valid.
 */
#define ENOTSOCK		(-10038)

/** The destination address required.
 */
#define EDESTADDRREQ	(-10039)

//#define EMSGSIZE		(-10040)
//#define EPROTOTYPE		(-10041)
//#define ENOPROTOOPT		(-10042)

/** Protocol is not supported.
 */
#define EPROTONOSUPPORT	(-10043)

/** Socket type is not supported.
 */
#define ESOCKTNOSUPPORT	(-10044)

//#define EOPNOTSUPP		(-10045)

/** Protocol family is not supported.
 */
#define EPFNOSUPPORT	(-10046)

/** Address family is not supported.
 */
#define EAFNOSUPPORT	(-10047)

/** Address is already in use.
 */
#define EADDRINUSE		(-10048)

//#define EADDRNOTAVAIL	(-10049)
/* May be reported at any time if the implementation detects an underlying failure.
 */
//#define ENETDOWN		(-10050)
//#define ENETUNREACH		(-10051)
//#define ENETRESET		(-10052)
//#define ECONNABORTED	(-10053)
//#define ECONNRESET		(-10054)
//#define ENOBUFS			(-10055)
//#define EISCONN			(-10056)

/** The socket is not connected or bound.
 */
#define ENOTCONN		(-10057)

//#define ESHUTDOWN		(-10058)
//#define ETOOMANYREFS	(-10059)
//#define ETIMEDOUT		(-10060)
//#define ECONNREFUSED	(-10061)
//#define ELOOP			(-10062)
////#define ENAMETOOLONG	(-10063)
//#define EHOSTDOWN		(-10064)
//#define EHOSTUNREACH	(-10065)
//#define HOST_NOT_FOUND	(-11001)

/** The requested operation was not performed.
 *  Try again later.
 */
#define TRY_AGAIN		(-11002)

//#define NO_RECOVERY		(-11003)

/** No data.
 */
#define NO_DATA			(-11004)

/*@}*/

#endif

/** @}
 */
