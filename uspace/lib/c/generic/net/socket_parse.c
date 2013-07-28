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

/** @addtogroup socket
 * @{
 */

/** @file
 * Command-line argument parsing functions related to networking.
 */

#include <net/socket_parse.h>
#include <net/socket.h>
#include <arg_parse.h>
#include <errno.h>
#include <str.h>

/** Translate the character string to the address family number.
 *
 * @param[in]  name The address family name.
 * @param[out] af   The corresponding address family number
 *                  or EAFNOSUPPORTED.
 *
 * @return EOK on success.
 * @return ENOTSUP on unsupported address family.
 *
 */
int socket_parse_address_family(const char *name, int *af)
{
	if (str_lcmp(name, "AF_INET6", 8) == 0) {
		*af = AF_INET6;
		return EOK;
	}
	
	if (str_lcmp(name, "AF_INET", 7) == 0) {
		*af = AF_INET;
		return EOK;
	}
	
	*af = EAFNOSUPPORT;
	return ENOTSUP;
}

/** Translate the character string to the protocol family number.
 *
 * @param[in]  name The protocol family name.
 * @param[out] pf   The corresponding protocol family number
 *                  or EPFNOSUPPORTED.
 *
 * @return EOK on success.
 * @return ENOTSUP on unsupported protocol family.
 *
 */
int socket_parse_protocol_family(const char *name, int *pf)
{
	if (str_lcmp(name, "PF_INET6", 8) == 0) {
		*pf = PF_INET6;
		return EOK;
	}

	if (str_lcmp(name, "PF_INET", 7) == 0) {
		*pf = PF_INET;
		return EOK;
	}
	
	*pf = EPFNOSUPPORT;
	return ENOTSUP;
}

/** Translate the character string to the socket type number.
 *
 * @param[in]  name  The socket type name.
 * @param[out] sockt The corresponding socket type number
 *                   or ESOCKTNOSUPPORT.
 *
 * @return EOK on success.
 * @return ENOTSUP on unsupported socket type.
 *
 */
int socket_parse_socket_type(const char *name, int *sockt)
{
	if (str_lcmp(name, "SOCK_DGRAM", 11) == 0) {
		*sockt = SOCK_DGRAM;
		return EOK;
	}
	
	if (str_lcmp(name, "SOCK_STREAM", 12) == 0) {
		*sockt = SOCK_STREAM;
		return EOK;
	}
	
	*sockt = ESOCKTNOSUPPORT;
	return ENOTSUP;
}

/** @}
 */
