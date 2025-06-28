/*
 * Copyright (c) 2025 Miroslav Cimerman
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

/** @addtogroup hr
 * @{
 */
/**
 * @file
 */

#include <block.h>
#include <byteorder.h>
#include <errno.h>
#include <inttypes.h>
#include <io/log.h>
#include <loc.h>
#include <mem.h>
#include <uuid.h>
#include <stdlib.h>
#include <stdio.h>
#include <str.h>

#include "superblock.h"
#include "util.h"
#include "var.h"

#include "metadata/native.h"

#include "metadata/foreign/geom/g_mirror.h"
#include "metadata/foreign/geom/g_stripe.h"
#include "metadata/foreign/softraid/softraidvar.h"

extern hr_superblock_ops_t metadata_native_ops;
extern hr_superblock_ops_t metadata_gmirror_ops;
extern hr_superblock_ops_t metadata_gstripe_ops;
extern hr_superblock_ops_t metadata_softraid_ops;
extern hr_superblock_ops_t metadata_md_ops;

static hr_superblock_ops_t *hr_superblock_ops_all[] = {
	[HR_METADATA_NATIVE] = &metadata_native_ops,
	[HR_METADATA_GEOM_MIRROR] = &metadata_gmirror_ops,
	[HR_METADATA_GEOM_STRIPE] = &metadata_gstripe_ops,
	[HR_METADATA_SOFTRAID] = &metadata_softraid_ops,
	[HR_METADATA_MD] = &metadata_md_ops
};

hr_superblock_ops_t *hr_get_meta_type_ops(hr_metadata_type_t type)
{
	assert(type >= HR_METADATA_NATIVE && type < HR_METADATA_LAST_DUMMY);

	return hr_superblock_ops_all[type];
}

errno_t hr_find_metadata(service_id_t svc_id, void **rmetadata,
    hr_metadata_type_t *rtype)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	hr_superblock_ops_t *meta_ops;
	void *metadata_struct;

	if (rmetadata == NULL)
		return EINVAL;
	if (rtype == NULL)
		return EINVAL;

	volatile hr_metadata_type_t type = HR_METADATA_NATIVE;
	for (; type < HR_METADATA_LAST_DUMMY; type++) {
		meta_ops = hr_superblock_ops_all[type];

		rc = meta_ops->probe(svc_id, &metadata_struct);
		if (rc != EOK) {
			if (rc != ENOFS)
				return rc;
			continue;
		}

		*rmetadata = metadata_struct;
		*rtype = type;
		return EOK;
	}

	return ENOFS;
}

/** @}
 */
