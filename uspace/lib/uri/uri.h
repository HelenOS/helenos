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

/** @addtogroup uri
 * @{
 */
/**
 * @file
 */

#ifndef URI_URI_H_
#define URI_URI_H_

typedef struct {
	char *scheme;
	char *user_info;
	char *user_credential;
	char *host;
	char *port;
	char *path;
	char *query;
	char *fragment;
} uri_t;

extern uri_t *uri_parse(const char *);
extern errno_t uri_scheme_parse(const char *, const char **);
extern bool uri_scheme_validate(const char *);
extern errno_t uri_percent_parse(const char *, const char **, uint8_t *);
extern errno_t uri_user_info_parse(const char *, const char **);
extern bool uri_user_info_validate(const char *);
extern errno_t uri_port_parse(const char *, const char **);
extern bool uri_port_validate(const char *);
extern bool uri_validate(uri_t *);
extern void uri_destroy(uri_t *);

#endif

/** @}
 */
