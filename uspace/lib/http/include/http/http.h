/*
 * Copyright (c) 2013 Martin Sucha
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

/** @addtogroup http
 * @{
 */
/**
 * @file
 */

#ifndef HTTP_HTTP_H_
#define HTTP_HTTP_H_

#include <adt/list.h>
#include <inet/addr.h>
#include <inet/tcp.h>

#include "receive-buffer.h"

typedef struct {
	char *host;
	uint16_t port;
	inet_addr_t addr;

	tcp_t *tcp;
	tcp_conn_t *conn;

	size_t buffer_size;
	receive_buffer_t recv_buffer;
} http_t;

typedef struct {
	uint8_t minor;
	uint8_t major;
} http_version_t;

typedef struct {
	link_t link;
	char *name;
	char *value;
} http_header_t;

typedef struct {
	list_t list;
} http_headers_t;

typedef struct {
	char *method;
	char *path;
	http_headers_t headers;
} http_request_t;

typedef struct {
	http_version_t version;
	uint16_t status;
	char *message;
	http_headers_t headers;
} http_response_t;

extern http_t *http_create(const char *, uint16_t);
extern errno_t http_connect(http_t *);

extern void http_header_init(http_header_t *);
extern http_header_t *http_header_create(const char *, const char *);
extern errno_t http_header_receive_name(receive_buffer_t *, receive_buffer_mark_t *);
extern errno_t http_header_receive_value(receive_buffer_t *, receive_buffer_mark_t *,
    receive_buffer_mark_t *);
extern errno_t http_header_receive(receive_buffer_t *, http_header_t *, size_t,
    size_t *);
extern void http_header_normalize_value(char *);
extern bool http_header_name_match(const char *, const char *);
ssize_t http_header_encode(http_header_t *, char *, size_t);
extern void http_header_destroy(http_header_t *);

extern void http_headers_init(http_headers_t *);
extern errno_t http_headers_find_single(http_headers_t *, const char *,
    http_header_t **);
extern errno_t http_headers_append(http_headers_t *, const char *, const char *);
extern errno_t http_headers_set(http_headers_t *, const char *, const char *);
extern errno_t http_headers_get(http_headers_t *, const char *, char **);
extern errno_t http_headers_receive(receive_buffer_t *, http_headers_t *, size_t,
    unsigned);
extern void http_headers_clear(http_headers_t *);

#define http_headers_foreach(headers, iter) \
    list_foreach((headers).list, link, http_header_t, (iter))

static inline void http_headers_remove(http_headers_t *headers,
    http_header_t *header)
{
	list_remove(&header->link);
}

static inline void http_headers_append_header(http_headers_t *headers,
    http_header_t *header)
{
	list_append(&header->link, &headers->list);
}

extern http_request_t *http_request_create(const char *, const char *);
extern void http_request_destroy(http_request_t *);
extern errno_t http_request_format(http_request_t *, char **, size_t *);
extern errno_t http_send_request(http_t *, http_request_t *);
extern errno_t http_receive_status(receive_buffer_t *, http_version_t *, uint16_t *,
    char **);
extern errno_t http_receive_response(receive_buffer_t *, http_response_t **,
    size_t, unsigned);
extern void http_response_destroy(http_response_t *);
extern errno_t http_close(http_t *);
extern void http_destroy(http_t *);

#endif

/** @}
 */
