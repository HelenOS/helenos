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
/** @file cfg.c
 * @brief Configuration files manipulation implementation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <cfg.h>

/**
 * @param data Configuration file data
 *
 * @return Anonymous section in the configuration file.
 * @return NULL if there is no such section (it is empty)
 *
 */
const cfg_section_t *cfg_anonymous(const cfg_file_t *data)
{
	assert(data != NULL);
	
	if (list_empty(&data->sections))
		return NULL;
	
	link_t *link = list_first(&data->sections);
	const cfg_section_t *section = cfg_section_instance(link);
	if (section->title[0] != 0)
		return NULL;
	
	return section;
}

/**
 * @param data Configuration file data
 *
 * @return True if the file contains no data
 *         (no sections, neither anonymous).
 * @return False otherwise.
 *
 */
bool cfg_empty(const cfg_file_t *data)
{
	assert(data != NULL);
	
	if (list_empty(&data->sections))
		return true;
	
	cfg_file_foreach(data, section) {
		if (!list_empty(&section->entries))
			return false;
	}
	
	return true;
}

/** Read file contents into memory.
 *
 * @param path Path to the file
 * @param buf  Pointer to pointer to buffer where
 *             the file contents will be stored
 *
 * @return EOK if the file was successfully loaded.
 * @return ENOMEM if there was not enough memory to load the file
 * @return EIO if there was a problem with reading the file
 * @return Other error code if there was a problem openening the file
 *
 */
static int cfg_read(const char *path, char **buf)
{
	assert(buf != NULL);
	
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return fd;
	
	size_t len = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	
	*buf = malloc(len + 1);
	if (*buf == NULL) {
		close(fd);
		return ENOMEM;
	}
	
	ssize_t rd = read_all(fd, *buf, len);
	if (rd < 0) {
		free(*buf);
		close(fd);
		return EIO;
	}
	
	(*buf)[len] = 0;
	close(fd);
	
	return EOK;
}

static inline void null_back(char *back)
{
	do {
		*back = 0;
		back--;
	} while (isspace(*back));
}

/** Allocate and initialize a new entry.
 *
 * @param key   Entry key
 * @param value Entry value
 *
 * @return New entry
 * @return NULL if there was not enough memory
 *
 */
static cfg_entry_t *cfg_new_entry(const char *key, const char *value)
{
	cfg_entry_t *entry = malloc(sizeof(cfg_entry_t));
	if (entry == NULL)
		return NULL;
	
	link_initialize(&entry->link);
	entry->key = key;
	entry->value = value;
	
	return entry;
}

/** Allocate and initialize a new section.
 *
 * @param title Section title
 *
 * @return New section
 * @return NULL if there was not enough memory
 *
 */
static cfg_section_t *cfg_new_section(const char *title)
{
	cfg_section_t *sec = malloc(sizeof(cfg_section_t));
	if (sec == NULL)
		return NULL;
	
	link_initialize(&sec->link);
	
	if (title != NULL)
		sec->title = title;
	else
		sec->title = "";
	
	list_initialize(&sec->entries);
	sec->entry_count = 0;
	
	return sec;
}

/** Skip whitespaces
 *
 */
static inline void skip_whitespaces(char **buffer)
{
	while (isspace(**buffer))
		(*buffer)++;
}

static inline int starts_comment(char c)
{
	return ((c == ';') || (c == '#'));
}

/** Load file content into memory
 *
 * Parse the file into sections and entries
 * and initialize data with this info.
 *
 * @param path Path to the configuration file
 * @param data Configuration file data
 *
 * @return EOK if the file was successfully loaded
 * @return EBADF if the configuration file has bad format.
 * @return ENOMEM if there was not enough memory
 * @return Error code from cfg_read()
 *
 */
int cfg_load(const char *path, cfg_file_t *data)
{
	char *buffer;
	int res = cfg_read(path, &buffer);
	if (res != EOK)
		return res;
	
	list_initialize(&data->sections);
	data->section_count = 0;
	data->data = buffer;
	
	cfg_section_t *curr_section = NULL;
	skip_whitespaces(&buffer);
	
	while (*buffer) {
		while (starts_comment(*buffer)) {
			while ((*buffer) && (*buffer != '\n'))
				buffer++;
			
			skip_whitespaces(&buffer);
		}
		
		if (*buffer == '[') {
			buffer++;
			skip_whitespaces(&buffer);
			
			const char *title = buffer;
			while ((*buffer) && (*buffer != ']') && (*buffer != '\n'))
				buffer++;
			
			if (*buffer != ']') {
				cfg_unload(data);
				return EBADF;
			}
			
			null_back(buffer);
			buffer++;
			
			cfg_section_t *sec = cfg_new_section(title);
			if (sec == NULL) {
				cfg_unload(data);
				return ENOMEM;
			}
			
			list_append(&sec->link, &data->sections);
			data->section_count++;
			curr_section = sec;
		} else if (*buffer) {
			const char *key = buffer;
			while ((*buffer) && (*buffer != '=') && (*buffer != '\n'))
				buffer++;
			
			if (*buffer != '=') {
				cfg_unload(data);
				return EBADF;
			}
			
			/* null = and whitespaces before */
			null_back(buffer);
			buffer++;
			skip_whitespaces(&buffer);
			
			while (starts_comment(*buffer)) {
				while ((*buffer) && (*buffer != '\n'))
					buffer++;
				
				skip_whitespaces(&buffer);
			}
			
			const char *value = buffer;
			/* Empty value is correct value */
			if (*buffer) {
				while ((*buffer) && (*buffer != '\n'))
					buffer++;
				
				if (*buffer) {
					null_back(buffer);
					buffer++;
				} else
					null_back(buffer);
			}
			
			/* Create anonymous section if not present */
			if (curr_section == NULL) {
				curr_section = cfg_new_section(NULL);
				if (curr_section == NULL) {
					cfg_unload(data);
					return ENOMEM;
				}
				
				list_append(&curr_section->link, &data->sections);
			}
			
			cfg_entry_t *entry = cfg_new_entry(key, value);
			if (entry == NULL) {
				cfg_unload(data);
				return ENOMEM;
			}
			
			list_append(&entry->link, &curr_section->entries);
			curr_section->entry_count++;
		}
		
		skip_whitespaces(&buffer);
	}
	
	return EOK;
}

/** Load file content into memory (with path)
 *
 * Parse the file (with path) into sections and entries
 * and initialize data with this info.
 *
 */
int cfg_load_path(const char *path, const char *fname, cfg_file_t *data)
{
	size_t sz = str_size(path) + str_size(fname) + 2;
	char *name = malloc(sz);
	if (name == NULL)
		return ENOMEM;
	
	snprintf(name, sz, "%s/%s", path, fname);
	int rc = cfg_load(name, data);
	
	free(name);
	
	return rc;
}

/** Deallocate memory used by entry
 *
 */
static void cfg_free_entry(const cfg_entry_t *entry)
{
	assert(entry != NULL);
	free(entry);
}

/** Deallocate memory used by all entries
 *
 * Deallocate memory used by all entries within a section
 * and the memory used by the section itself.
 *
 */
static void cfg_free_section(const cfg_section_t *section)
{
	assert(section != NULL);
	
	link_t *cur;
	link_t *next;
	
	for (cur = section->entries.head.next;
	    cur != &section->entries.head;
	    cur = next) {
		next = cur->next;
		cfg_free_entry(cfg_entry_instance(cur));
	}
	
	free(section);
}

/** Deallocate memory used by configuration data
 *
 * Deallocate all inner sections and entries.
 *
 */
void cfg_unload(cfg_file_t *data)
{
	assert(data != NULL);
	
	link_t *cur, *next;
	for (cur = data->sections.head.next;
	    cur != &data->sections.head;
	    cur = next) {
		next = cur->next;
		cfg_free_section(cfg_section_instance(cur));
	}
	
	free(data->data);
}

/** Find a section in the configuration data
 *
 * @param data  Configuration data
 * @param title Title of the section to search for
 *
 * @return Found section
 * @return NULL if there is no section with such title
 *
 */
const cfg_section_t *cfg_find_section(const cfg_file_t *data, const char *title)
{
	cfg_file_foreach(data, section) {
		if (str_cmp(section->title, title) == 0)
			return section;
	}
	
	return NULL;
}

/** Find entry value in the configuration data
 *
 * @param section Section in which to search
 * @param key     Key of the entry we to search for
 *
 * @return Value of the entry
 * @return NULL if there is no entry with such key
 *
 */
const char *cfg_find_value(const cfg_section_t *section, const char *key)
{
	cfg_section_foreach(section, entry) {
		if (str_cmp(entry->key, key) == 0)
			return entry->value;
	}
	
	return NULL;
}

/** @}
 */
