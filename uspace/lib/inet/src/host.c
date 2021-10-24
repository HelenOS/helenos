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

/** @addtogroup libinet
 * @{
 */
/** @file Internet host specification
 *
 * Either a host name or an address.
 */

#include <assert.h>
#include <errno.h>
#include <inet/addr.h>
#include <inet/dnsr.h>
#include <inet/host.h>
#include <inet/hostname.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>

/** Parse host string.
 *
 * @param str Host string
 * @param rhost Place to store pointer to new host structure
 * @param endptr Place to store pointer to next character in string or @c NULL
 *
 * @return EOK on success. EINVAL if @a str does not start with a valid
 *         host specification or if @a endptr is @c NULL and @a str contains
 *         extra characters at the end. ENOMEM if out of memory
 */
errno_t inet_host_parse(const char *str, inet_host_t **rhost,
    char **endptr)
{
	inet_host_t *host;
	inet_addr_t addr;
	char *name;
	char *aend;
	errno_t rc;

	host = calloc(1, sizeof(inet_host_t));
	if (host == NULL)
		return ENOMEM;

	/* Try address */
	rc = inet_addr_parse(str, &addr, &aend);
	if (rc == EOK) {
		host->hform = inet_host_addr;
		host->host.addr = addr;
		goto have_host;
	}

	/* Try <hostname> */
	rc = inet_hostname_parse(str, &name, &aend);
	if (rc == EOK) {
		host->hform = inet_host_name;
		host->host.name = name;
		goto have_host;
	}

	free(host);
	return EINVAL;

have_host:
	if (*aend != '\0' && endptr == NULL) {
		/* Extra characters */
		free(host);
		return EINVAL;
	}

	*rhost = host;
	if (endptr != NULL)
		*endptr = (char *)aend;
	return EOK;
}

/** Convert host structure to string representation.
 *
 * @param host Host structure
 * @param rstr Place to store pointer to new string
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t inet_host_format(inet_host_t *host, char **rstr)
{
	errno_t rc;
	char *str = NULL;

	switch (host->hform) {
	case inet_host_addr:
		rc = inet_addr_format(&host->host.addr, &str);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			return ENOMEM;
		}

		break;
	case inet_host_name:
		str = str_dup(host->host.name);
		if (str == NULL)
			return ENOMEM;
		break;
	}

	*rstr = str;
	return EOK;
}

/** Destroy host structure.
 *
 * @param host Host structure or @c NULL
 */
void inet_host_destroy(inet_host_t *host)
{
	if (host == NULL)
		return;

	switch (host->hform) {
	case inet_host_addr:
		break;
	case inet_host_name:
		free(host->host.name);
		break;
	}

	free(host);
}

/** Look up first address corresponding to host.
 *
 * If host contains a host name, name resolution is performed.
 *
 * @param host Host structure
 * @param version Desired IP version (ip_any to get any version)
 * @param addr Place to store address
 *
 * @reutrn EOK on success, ENOENT on resolurion failure
 */
errno_t inet_host_lookup_one(inet_host_t *host, ip_ver_t version, inet_addr_t *addr)
{
	dnsr_hostinfo_t *hinfo = NULL;
	errno_t rc;

	switch (host->hform) {
	case inet_host_addr:
		*addr = host->host.addr;
		break;
	case inet_host_name:
		rc = dnsr_name2host(host->host.name, &hinfo, version);
		if (rc != EOK) {
			dnsr_hostinfo_destroy(hinfo);
			return ENOENT;
		}

		*addr = hinfo->addr;
		dnsr_hostinfo_destroy(hinfo);
		break;
	}

	return EOK;
}

/** Look up first address corresponding to host string.
 *
 * If host contains a host name, name resolution is performed.
 *
 * @param str Host string
 * @param version Desired IP version (ip_any to get any version)
 * @param addr Place to store address
 * @param endptr Place to store pointer to next character in string or @c NULL
 * @param errmsg Place to store pointer to error message (no need to free it)
 *               or @c NULL
 *
 * @return EOK on success. EINVAL if @a str has incorrect format or if
 *         @a endptr is @c NULL and @a str contains extra characters at
 *         the end. ENOENT if host was not found during resolution,
 *         ENOMEM if out of memory
 */
errno_t inet_host_plookup_one(const char *str, ip_ver_t version, inet_addr_t *addr,
    char **endptr, const char **errmsg)
{
	inet_host_t *host = NULL;
	char *eptr;
	errno_t rc;

	rc = inet_host_parse(str, &host, endptr != NULL ? &eptr : NULL);
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
		default:
			assert(false);
		}
	}

	rc = inet_host_lookup_one(host, version, addr);
	if (rc != EOK) {
		/* XXX Distinguish between 'not found' and other error */
		rc = ENOENT;
		if (errmsg != NULL)
			*errmsg = "Name resolution failed";
		goto error;
	}

	inet_host_destroy(host);
	if (endptr != NULL)
		*endptr = eptr;
	return EOK;
error:
	inet_host_destroy(host);
	return rc;
}

/** @}
 */
