/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef OS_H_
#define OS_H_

#include <errno.h>

char *os_str_acat(const char *a, const char *b);
char *os_str_aslice(const char *str, size_t start, size_t length);
int os_str_cmp(const char *a, const char *b);
char *os_str_dup(const char *str);
size_t os_str_length(const char *str);
errno_t os_str_get_char(const char *str, int index, int *out_char);
char *os_chr_to_astr(char32_t chr);
void os_input_disp_help(void);
errno_t os_input_line(const char *prompt, char **ptr);
errno_t os_exec(char *const cmd[]);

void os_store_ef_path(char *path);
char *os_get_lib_path(void);

#endif
