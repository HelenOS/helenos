/*
 * Copyright (c) 2011 Radim Vansa
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

/**
 * @addtogroup libc
 * @{
 */
/**
 * @file cfg.h
 * @brief Simple interface for configuration files manipulation.
 */

#ifndef LIBC_CFG_H_
#define LIBC_CFG_H_

#include <adt/list.h>
#include <sys/types.h>
#include <stdbool.h>

/**
 * One key-value pair
 */
typedef struct {
	link_t link;
	
	const char *key;
	const char *value;
} cfg_entry_t;

/**
 * One section in the configuration file.
 * Has own title (or is anonymous) and contains entries.
 */
typedef struct {
	link_t link;
	
	const char *title;
	list_t entries;
	size_t entry_count;
} cfg_section_t;

/**
 * The configuration file data. The whole file is loaded
 * into memory and strings in sections (titles) or entries
 * (keys and values) are only pointers to this memory.
 * Therefore all the text data are allocated and deallocated
 * only once, if you want to use the strings after unloading
 * the configuration file, you have to copy them.
 */
typedef struct {
	list_t sections;
	size_t section_count;
	const char *data;
} cfg_file_t;

#define cfg_file_foreach(file, cur) \
	list_foreach((file)->sections, link, const cfg_section_t, (cur))

#define cfg_section_instance(cur) \
	list_get_instance((cur), const cfg_section_t, link)

#define cfg_section_foreach(section, cur) \
	list_foreach((section)->entries, link, const cfg_entry_t, (cur))

#define cfg_entry_instance(cur) \
	list_get_instance((cur), const cfg_entry_t, link)

extern int cfg_load(const char *, cfg_file_t *);
extern int cfg_load_path(const char *, const char *, cfg_file_t *);
extern void cfg_unload(cfg_file_t *);
extern bool cfg_empty(const cfg_file_t *);
extern const cfg_section_t *cfg_find_section(const cfg_file_t *, const char *);
extern const char *cfg_find_value(const cfg_section_t *, const char *);
extern const cfg_section_t *cfg_anonymous(const cfg_file_t *);

#endif

/** @}
 */
