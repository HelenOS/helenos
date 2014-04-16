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

/** @addtogroup nettest
 * @{
 */

/** @file
 * Networking test support functions.
 */

#ifndef NET_TEST_
#define NET_TEST_

#include <net/socket.h>

extern void print_mark(unsigned int);
extern int sockets_create(int, int *, unsigned int, uint16_t, sock_type_t);
extern int sockets_close(int, int *, unsigned int);
extern int sockets_connect(int, int *, unsigned int, struct sockaddr *,
    socklen_t);
extern int sockets_sendto(int, int *, unsigned int, struct sockaddr *,
    socklen_t, char *, size_t, unsigned int, sock_type_t);
extern int sockets_recvfrom(int, int *, unsigned int, struct sockaddr *,
    socklen_t *, char *, size_t, unsigned int);
extern int sockets_sendto_recvfrom(int, int *, unsigned int, struct sockaddr *,
    socklen_t *, char *, size_t, unsigned int, sock_type_t);

#endif

/** @}
 */

