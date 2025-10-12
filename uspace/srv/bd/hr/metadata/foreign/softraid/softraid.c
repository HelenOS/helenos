/* $OpenBSD: softraid.c,v 1.429 2022/12/21 09:54:23 kn Exp $ */
/*
 * Copyright (c) 2007, 2008, 2009 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2008 Chris Kuethe <ckuethe@openbsd.org>
 * Copyright (c) 2009 Joel Sing <jsing@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* stripped down softraid.c */

#include <stdio.h>
#include <stdlib.h>

#include "softraidvar.h"

void
sr_checksum_print(const u_int8_t *md5)
{
	int			i;

	for (i = 0; i < MD5_DIGEST_LENGTH; i++)
		printf("%02x", md5[i]);
}

char *
sr_uuid_format(const struct sr_uuid *uuid)
{
	char *uuidstr;

	/* changed to match HelenOS malloc() */
	uuidstr = malloc(37);
	if (uuidstr == NULL)
		return NULL;

	snprintf(uuidstr, 37,
	    "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-"
	    "%02x%02x%02x%02x%02x%02x",
	    uuid->sui_id[0], uuid->sui_id[1],
	    uuid->sui_id[2], uuid->sui_id[3],
	    uuid->sui_id[4], uuid->sui_id[5],
	    uuid->sui_id[6], uuid->sui_id[7],
	    uuid->sui_id[8], uuid->sui_id[9],
	    uuid->sui_id[10], uuid->sui_id[11],
	    uuid->sui_id[12], uuid->sui_id[13],
	    uuid->sui_id[14], uuid->sui_id[15]);

	return uuidstr;
}

void
sr_uuid_print(const struct sr_uuid *uuid, int cr)
{
	char *uuidstr;

	uuidstr = sr_uuid_format(uuid);
	printf("%s%s", uuidstr, (cr ? "\n" : ""));
	free(uuidstr);
}

void
sr_meta_print(const struct sr_metadata *m)
{
	unsigned		 i;
	struct sr_meta_chunk	*mc;
	struct sr_meta_opt_hdr	*omh;

	printf("\tssd_magic 0x%" PRIx64 "\n", m->ssdi.ssd_magic);
	printf("\tssd_version %" PRId32 "\n", m->ssdi.ssd_version);
	printf("\tssd_vol_flags 0x%" PRIx32 "\n", m->ssdi.ssd_vol_flags);
	printf("\tssd_uuid ");
	sr_uuid_print(&m->ssdi.ssd_uuid, 1);
	printf("\tssd_chunk_no %" PRId32 "\n", m->ssdi.ssd_chunk_no);
	printf("\tssd_chunk_id %" PRId32 "\n", m->ssdi.ssd_chunk_id);
	printf("\tssd_opt_no %" PRId32 "\n", m->ssdi.ssd_opt_no);
	printf("\tssd_volid %" PRId32 "\n", m->ssdi.ssd_volid);
	printf("\tssd_level %" PRId32 "\n", m->ssdi.ssd_level);
	printf("\tssd_size %" PRId64 "\n", m->ssdi.ssd_size);
	printf("\tssd_devname %s\n", m->ssd_devname);
	printf("\tssd_vendor %s\n", m->ssdi.ssd_vendor);
	printf("\tssd_product %s\n", m->ssdi.ssd_product);
	printf("\tssd_revision %s\n", m->ssdi.ssd_revision);
	printf("\tssd_strip_size %" PRId32 "\n", m->ssdi.ssd_strip_size);
	printf("\tssd_checksum ");
	sr_checksum_print(m->ssd_checksum);
	printf("\n");
	printf("\tssd_meta_flags 0x%" PRIx32 "\n", m->ssd_meta_flags);
	printf("\tssd_ondisk %" PRId64 "\n", m->ssd_ondisk);
	printf("\tssd_rebuild %" PRId64 "\n", m->ssd_rebuild);

	mc = (struct sr_meta_chunk *)(m + 1);
	for (i = 0; i < m->ssdi.ssd_chunk_no; i++, mc++) {
		printf("\t\tscm_volid %" PRId32 "\n", mc->scmi.scm_volid);
		printf("\t\tscm_chunk_id %" PRId32 "\n", mc->scmi.scm_chunk_id);
		printf("\t\tscm_devname %s\n", mc->scmi.scm_devname);
		printf("\t\tscm_size %" PRId64 "\n", mc->scmi.scm_size);
		printf("\t\tscm_coerced_size %" PRId64 "\n", mc->scmi.scm_coerced_size);
		printf("\t\tscm_uuid ");
		sr_uuid_print(&mc->scmi.scm_uuid, 1);
		printf("\t\tscm_checksum ");
		sr_checksum_print(mc->scm_checksum);
		printf("\n");
		printf("\t\tscm_status %" PRId32 "\n", mc->scm_status);
	}

	omh = (struct sr_meta_opt_hdr *)((u_int8_t *)(m + 1) +
	    sizeof(struct sr_meta_chunk) * m->ssdi.ssd_chunk_no);
	for (i = 0; i < m->ssdi.ssd_opt_no; i++) {
		printf("\t\t\tsom_type %" PRId32 "\n", omh->som_type);
		printf("\t\t\tsom_checksum ");
		sr_checksum_print(omh->som_checksum);
		printf("\n");
		omh = (struct sr_meta_opt_hdr *)((void *)omh +
		    omh->som_length);
	}
}
