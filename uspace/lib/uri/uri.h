/*
 * SPDX-FileCopyrightText: 2013 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
