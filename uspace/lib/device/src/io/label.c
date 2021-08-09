/*
 * Copyright (c) 2015 Jiri Svoboda
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
