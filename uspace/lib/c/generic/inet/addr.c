/*
 * Copyright (c) 2013 Jiri Svoboda
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
/** @file Internet address parsing and formatting.
 */

#include <errno.h>
#include <inet/addr.h>
#include <stdio.h>

/** Parse network address.
 *
 * @param text	Network address in CIDR notation (a.b.c.d/w)
 * @param naddr	Place to store network address
 *
 * @return 	EOK on success, EINVAL if input is not in valid format
 */
int inet_naddr_parse(const char *text, inet_naddr_t *naddr)
{
	unsigned long a[4], bits;
	char *cp = (char *)text;
	int i;

	for (i = 0; i < 3; i++) {
		a[i] = strtoul(cp, &cp, 10);
		if (*cp != '.')
			return EINVAL;
		++cp;
	}

	a[3] = strtoul(cp, &cp, 10);
	if (*cp != '/')
		return EINVAL;
	++cp;

	bits = strtoul(cp, &cp, 10);
	if (*cp != '\0')
		return EINVAL;

	naddr->ipv4 = 0;
	for (i = 0; i < 4; i++) {
		if (a[i] > 255)
			return EINVAL;
		naddr->ipv4 = (naddr->ipv4 << 8) | a[i];
	}

	if (bits > 31)
		return EINVAL;

	naddr->bits = bits;
	return EOK;
}

/** Parse node address.
 *
 * @param text	Network address in dot notation (a.b.c.d)
 * @param addr	Place to store node address
 *
 * @return 	EOK on success, EINVAL if input is not in valid format
 */
int inet_addr_parse(const char *text, inet_addr_t *addr)
{
	unsigned long a[4];
	char *cp = (char *)text;
	int i;

	for (i = 0; i < 3; i++) {
		a[i] = strtoul(cp, &cp, 10);
		if (*cp != '.')
			return EINVAL;
		++cp;
	}

	a[3] = strtoul(cp, &cp, 10);
	if (*cp != '\0')
		return EINVAL;

	addr->ipv4 = 0;
	for (i = 0; i < 4; i++) {
		if (a[i] > 255)
			return EINVAL;
		addr->ipv4 = (addr->ipv4 << 8) | a[i];
	}

	return EOK;
}

/** Format network address.
 *
 * @param naddr	Network address
 * @param bufp	Place to store pointer to formatted string (CIDR notation)
 *
 * @return 	EOK on success, ENOMEM if out of memory.
 */
int inet_naddr_format(inet_naddr_t *naddr, char **bufp)
{
	int rc;

	rc = asprintf(bufp, "%d.%d.%d.%d/%d", naddr->ipv4 >> 24,
	    (naddr->ipv4 >> 16) & 0xff, (naddr->ipv4 >> 8) & 0xff,
	    naddr->ipv4 & 0xff, naddr->bits);

	if (rc < 0)
		return ENOMEM;

	return EOK;
}

/** Format node address.
 *
 * @param addr	Node address
 * @param bufp	Place to store pointer to formatted string (dot notation)
 *
 * @return 	EOK on success, ENOMEM if out of memory.
 */
int inet_addr_format(inet_addr_t *addr, char **bufp)
{
	int rc;

	rc = asprintf(bufp, "%d.%d.%d.%d", addr->ipv4 >> 24,
	    (addr->ipv4 >> 16) & 0xff, (addr->ipv4 >> 8) & 0xff,
	    addr->ipv4 & 0xff);

	if (rc < 0)
		return ENOMEM;

	return EOK;
}

/** @}
 */
