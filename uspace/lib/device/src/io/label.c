/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file Volume service API
 */

#include <errno.h>
#include <io/label.h>
#include <stdlib.h>
#include <str.h>

/** Format label type as string.
 *
 * @param ltype Label type
 * @param rstr Place to return pointer to newly allocated string
 * @return EOK on success, ENOMEM if out of memory
 */
int label_type_format(label_type_t ltype, char **rstr)
{
	const char *sltype;
	char *s;

	sltype = NULL;
	switch (ltype) {
	case lt_none:
		sltype = "None";
		break;
	case lt_mbr:
		sltype = "MBR";
		break;
	case lt_gpt:
		sltype = "GPT";
		break;
	}

	s = str_dup(sltype);
	if (s == NULL)
		return ENOMEM;

	*rstr = s;
	return EOK;
}

/** Format partition kind as string.
 *
 * @param pkind Partition kind
 * @param rstr Place to return pointer to newly allocated string
 * @return EOK on success, ENOMEM if out of memory
 */
int label_pkind_format(label_pkind_t pkind, char **rstr)
{
	const char *spkind;
	char *s;

	spkind = NULL;
	switch (pkind) {
	case lpk_primary:
		spkind = "Primary";
		break;
	case lpk_extended:
		spkind = "Extended";
		break;
	case lpk_logical:
		spkind = "Logical";
		break;
	}

	s = str_dup(spkind);
	if (s == NULL)
		return ENOMEM;

	*rstr = s;
	return EOK;
}

/** @}
 */
