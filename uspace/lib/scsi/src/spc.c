/*
 * Copyright (c) 2011 Jiri Svoboda
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

#include <stdint.h>
#include <stdlib.h>
#include "scsi/spc.h"

/** String descriptions for SCSI peripheral device type.
 *
 * Only device types that we are ever likely to encounter are listed here.
 */
const char *scsi_dev_type_str[SCSI_DEV_LIMIT] = {
	[SCSI_DEV_BLOCK]	= "Direct-Access Block Device (Disk)",
	[SCSI_DEV_STREAM]	= "Sequential-Access Device (Tape)",
	[SCSI_DEV_CD_DVD]	= "CD/DVD",
	[SCSI_DEV_CHANGER]	= "Media Changer",
	[SCSI_DEV_ENCLOSURE]	= "SCSI Enclosure",
	[SCSI_DEV_OSD]		= "Object Storage Device"
};

/** Get peripheral device type string.
 *
 * Return string description of SCSI peripheral device type.
 * The returned string is valid indefinitely, the caller should
 * not attempt to free it.
 */
const char *scsi_get_dev_type_str(unsigned dev_type)
{
	if (dev_type >= SCSI_DEV_LIMIT || scsi_dev_type_str[dev_type] == NULL)
		return "<unknown>";

	return scsi_dev_type_str[dev_type];
}
