/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INPUT_H_
#define INPUT_H_

#include "mytypes.h"

errno_t input_new_file(input_t **input, const char *fname);
errno_t input_new_interactive(input_t **input);
errno_t input_new_string(input_t **input, const char *str);

errno_t input_get_line(input_t *input, char **line);
int input_get_line_no(input_t *input);

#endif
