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
 * @file User interface
 */

#include <display.h>
#include <errno.h>
#include <fibril.h>
#include <stdlib.h>
#include <task.h>
#include <ui/ui.h>
#include "../private/ui.h"

/** Create new user interface.
 *
 * @param ospec Output specification or @c UI_DISPLAY_DEFAULT to use
 *              the default output
 * @param rui Place to store pointer to new UI
 * @return EOK on success or an error code
 */
errno_t ui_create(const char *ospec, ui_t **rui)
{
	errno_t rc;
	display_t *display;
	ui_t *ui;

	rc = display_open(ospec, &display);
	if (rc != EOK)
		return rc;

	rc = ui_create_disp(display, &ui);
	if (rc != EOK) {
		display_close(display);
		return rc;
	}

	ui->display = display;
	ui->myoutput = true;
	*rui = ui;
	return EOK;
}

/** Create new user interface using display service.
 *
 * @param disp Display
 * @param rui Place to store pointer to new UI
 * @return EOK on success or an error code
 */
errno_t ui_create_disp(display_t *disp, ui_t **rui)
{
	ui_t *ui;

	ui = calloc(1, sizeof(ui_t));
	if (ui == NULL)
		return ENOMEM;

	ui->display = disp;
	*rui = ui;
	return EOK;
}

/** Destroy user interface.
 *
 * @param ui User interface or @c NULL
 */
void ui_destroy(ui_t *ui)
{
	if (ui == NULL)
		return;

	if (ui->myoutput)
		display_close(ui->display);
	free(ui);
}

/** Execute user interface.
 *
 * Return task exit code of zero and block unitl the application starts
 * the termination process by calling ui_quit(@a ui).
 *
 * @param ui User interface
 */
void ui_run(ui_t *ui)
{
	task_retval(0);

	while (!ui->quit)
		fibril_usleep(100000);
}

/** Terminate user interface.
 *
 * Calling this function causes the user interface to terminate
 * (i.e. exit from ui_run()). This would be typically called from
 * an event handler.
 *
 * @param ui User interface
 */
void ui_quit(ui_t *ui)
{
	ui->quit = true;
}

/** @}
 */
