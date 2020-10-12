/*
 * Copyright (c) 2020 Jiri Svoboda
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

/** @addtogroup libui
 * @{
 */
/**
 * @file Push button
 */

#include <errno.h>
#include <stdlib.h>
#include <ui/pbutton.h>
#include "../private/pbutton.h"

/** Create new push button.
 *
 * @param caption Caption
 * @param rpbutton Place to store pointer to new push button
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_pbutton_create(const char *caption, ui_pbutton_t **rpbutton)
{
	ui_pbutton_t *pbutton;

	pbutton = calloc(1, sizeof(ui_pbutton_t));
	if (pbutton == NULL)
		return ENOMEM;

	(void) caption;
	*rpbutton = pbutton;
	return EOK;
}

/** Destroy push button.
 *
 * @param pbutton Push button or @c NULL
 */
void ui_pbutton_destroy(ui_pbutton_t *pbutton)
{
	if (pbutton == NULL)
		return;

	free(pbutton);
}

/** @}
 */
