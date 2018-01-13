/*
 * Copyright (c) 2016 Jiri Svoboda
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
/** @file Internet host:port specification
 *
 * As in RFC 1738 Uniform Resouce Locators (URL) and RFC 2732 Format for
 * literal IPv6 Addresses in URLs
 */

#include <errno.h>
#include <inet/addr.h>
#include <inet/dnsr.h>
#include <inet/endpoint.h>
#include <inet/hostname.h>
#include <inet/hostport.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>

/** Parse host:port string.
 *
 * @param str Host:port string
 * @param rhp Place to store pointer to new hostport structure
 * @param endptr Place to store pointer to next character in string or @c NULL
 *
 * @return EOK on success. EINVAL if @a str does not start with a valid
 *         host:port or if @a endptr is @c NULL and @a str contains
 *         extra characters at the end. ENOMEM if out of memory
 */
errno_t inet_hostport_parse(const char *str, inet_hostport_t **rhp,
    char **endptr)
{
	inet_hostport_t *hp;
	inet_addr_t addr;
	uint16_t port;
	char *name;
	char *aend;
	const char *pend;
	errno_t rc;

	hp = calloc(1, sizeof(inet_hostport_t));
	if (hp == NULL)
		return ENOMEM;

	if (str[0] == '[') {
		/* Try [<ip-addr-literal>] */
		rc = inet_addr_parse(str + 1, &addr, &aend);
		if (rc == EOK) {
			if (*aend == ']') {
				hp->hform = inet_host_addr;
				hp->host.addr = addr;
				++aend;
			}
			goto have_host;
		}
	}

	/* Try <ipv4-addr> */
	rc = inet_addr_parse(str, &addr, &aend);
	if (rc == EOK) {
		hp->hform = inet_host_addr;
		hp->host.addr = addr;
		goto have_host;
	}

	/* Try <hostname> */
	rc = inet_hostname_parse(str, &name, &aend);
	if (rc == EOK) {
		hp->hform = inet_host_name;
		hp->host.name = name;
		goto have_host;
	}

	free(hp);
	return EINVAL;

have_host:
	if (*aend == ':') {
		/* Port number */
		rc = str_uint16_t(aend + 1, &pend, 10, false, &port);
		if (rc != EOK) {
			free(hp);
			return EINVAL;
		}
	} else {
		/* Port number omitted */
		port = 0;
		pend = aend;
	}

	hp->port = port;

	if (*pend != '\0' && endptr == NULL) {
		/* Extra characters */
		free(hp);
		return EINVAL;
	}

	*rhp = hp;
	if (endptr != NULL)
		*endptr = (char *)pend;
	return EOK;
}

/** Convert host:port to string representation.
 *
 * @param hp Host:port structure
 * @param rstr Place to store pointer to new string
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t inet_hostport_format(inet_hostport_t *hp, char **rstr)
{
	errno_t rc;
	int ret;
	char *astr, *str;
	char *hstr = NULL;

	switch (hp->hform) {
	case inet_host_addr:
		rc = inet_addr_format(&hp->host.addr, &astr);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			return ENOMEM;
		}

		if (hp->host.addr.version != ip_v4) {
			hstr = astr;
		} else {
			ret = asprintf(&hstr, "[%s]", astr);
			if (ret < 0) {
				free(astr);
				return ENOMEM;
			}

			free(astr);
		}

		break;
	case inet_host_name:
		hstr = str_dup(hp->host.name);
		if (hstr == NULL)
			return ENOMEM;
		break;
	}

	ret = asprintf(&str, "%s:%u", hstr, hp->port);
	if (ret < 0)
		return ENOMEM;

	free(hstr);
	*rstr = str;
	return EOK;
}

/** Destroy host:port structure.
 *
 * @param hp Host:port or @c NULL
 */
void inet_hostport_destroy(inet_hostport_t *hp)
{
	if (hp == NULL)
		return;

	switch (hp->hform) {
	case inet_host_addr:
		break;
	case inet_host_name:
		free(hp->host.name);
		break;
	}

	free(hp);
}

/** Look up first endpoint corresponding to host:port.
 *
 * If host:port contains a host name, name resolution is performed.
 *
 * @param hp Host:port structure
 * @param version Desired IP version (ip_any to get any version)
 * @param ep Place to store endpoint
 *
 * @return EOK on success, ENOENT on resolution failure
 */
errno_t inet_hostport_lookup_one(inet_hostport_t *hp, ip_ver_t version,
    inet_ep_t *ep)
{
	dnsr_hostinfo_t *hinfo = NULL;
	errno_t rc;

	inet_ep_init(ep);

	switch (hp->hform) {
	case inet_host_addr:
		ep->addr = hp->host.addr;
		break;
	case inet_host_name:
		rc = dnsr_name2host(hp->host.name, &hinfo, ip_any);
		if (rc != EOK) {
			dnsr_hostinfo_destroy(hinfo);
			return ENOENT;
		}

		ep->addr = hinfo->addr;
		dnsr_hostinfo_destroy(hinfo);
		break;
	}

	ep->port = hp->port;
	return EOK;
}

/** Look up first endpoint corresponding to host:port string.
 *
 * If host:port contains a host name, name resolution is performed.
 *
 * @param str Host:port string
 * @param version Desired IP version (ip_any to get any version)
 * @param ep Place to store endpoint
 * @param endptr Place to store pointer to next character in string or @c NULL
 * @param errmsg Place to store pointer to error message (no need to free it)
 *               or @c NULL
 *
 * @return EOK on success. EINVAL if @a str has incorrect format or if
 *         @a endptr is @c NULL and @a str contains extra characters at
 *         the end. ENOENT if host was not found during resolution,
 *         ENOMEM if out of memory
 */
errno_t inet_hostport_plookup_one(const char *str, ip_ver_t version, inet_ep_t *ep,
    char **endptr, const char **errmsg)
{
	inet_hostport_t *hp = NULL;
	char *eptr;
	errno_t rc;

	rc = inet_hostport_parse(str, &hp, endptr != NULL ? &eptr : NULL);
	if (rc != EOK) {
		switch (rc) {
		case EINVAL:
			if (errmsg != NULL)
				*errmsg = "Invalid format";
			goto error;
		case ENOMEM:
			if (errmsg != NULL)
				*errmsg = "Out of memory";
			goto error;
		case EOK:
			break;
		default:
			assert(false);
		}
	}

	rc = inet_hostport_lookup_one(hp, version, ep);
	if (rc != EOK) {
		/* XXX Distinguish between 'not found' and other error */
		rc = ENOENT;
		if (errmsg != NULL)
			*errmsg = "Name resolution failed";
		goto error;
	}

	inet_hostport_destroy(hp);
	if (endptr != NULL)
		*endptr = eptr;
	return EOK;
error:
	inet_hostport_destroy(hp);
	return rc;
}

/** @}
 */
