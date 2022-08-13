/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
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

const char *scsi_sense_key_str[SCSI_SK_LIMIT] = {
	[SCSI_SK_NO_SENSE]		= "No Sense",
	[SCSI_SK_RECOVERED_ERROR]	= "Recovered Error",
	[SCSI_SK_NOT_READY]		= "Not Ready",
	[SCSI_SK_MEDIUM_ERROR]		= "Medium Error",
	[SCSI_SK_HARDWARE_ERROR]	= "Hardware Error",
	[SCSI_SK_ILLEGAL_REQUEST]	= "Illegal Request",
	[SCSI_SK_UNIT_ATTENTION]	= "Unit Attention",
	[SCSI_SK_DATA_PROTECT]		= "Data Protect",
	[SCSI_SK_BLANK_CHECK]		= "Blank Check",
	[SCSI_SK_VENDOR_SPECIFIC]	= "Vendor-specific",
	[SCSI_SK_COPY_ABORTED]		= "Copy Aborted",
	[SCSI_SK_ABORTED_COMMAND]	= "Aborted Command",
	[SCSI_SK_VOLUME_OVERFLOW]	= "Volume Overflow",
	[SCSI_SK_MISCOMPARE]		= "Miscompare"
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
		return "Unknown";

	return scsi_dev_type_str[dev_type];
}

/** Get sense key string.
 *
 * Return string description of SCSI sense key.
 * The returned string is valid indefinitely, the caller should
 * not attempt to free it.
 */
const char *scsi_get_sense_key_str(unsigned sense_key)
{
	if (sense_key >= SCSI_SK_LIMIT || scsi_sense_key_str[sense_key] == NULL)
		return "Unknown";

	return scsi_sense_key_str[sense_key];
}
