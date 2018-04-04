/*
 * Copyright (c) 2010 Lenka Trochtova
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

/** @addtogroup devman
 * @{
 */

#include <errno.h>
#include <io/log.h>
#include <str.h>
#include <str_error.h>
#include <stddef.h>
#include <vfs/vfs.h>

#include "devman.h"
#include "match.h"

#define COMMENT	'#'

/** Compute compound score of driver and device.
 *
 * @param driver Match id of the driver.
 * @param device Match id of the device.
 * @return Compound score.
 * @retval 0 No match at all.
 */
static int compute_match_score(match_id_t *driver, match_id_t *device)
{
	if (str_cmp(driver->id, device->id) == 0) {
		/*
		 * The strings match, return the product of their scores.
		 */
		return driver->score * device->score;
	} else {
		/*
		 * Different strings, return zero.
		 */
		return 0;
	}
}

int get_match_score(driver_t *drv, dev_node_t *dev)
{
	link_t *drv_head = &drv->match_ids.ids.head;
	link_t *dev_head = &dev->pfun->match_ids.ids.head;

	if (list_empty(&drv->match_ids.ids) ||
	    list_empty(&dev->pfun->match_ids.ids)) {
		return 0;
	}

	/*
	 * Go through all pairs, return the highest score obtained.
	 */
	int highest_score = 0;

	link_t *drv_link = drv->match_ids.ids.head.next;
	while (drv_link != drv_head) {
		link_t *dev_link = dev_head->next;
		while (dev_link != dev_head) {
			match_id_t *drv_id = list_get_instance(drv_link, match_id_t, link);
			match_id_t *dev_id = list_get_instance(dev_link, match_id_t, link);

			int score = compute_match_score(drv_id, dev_id);
			if (score > highest_score) {
				highest_score = score;
			}

			dev_link = dev_link->next;
		}

		drv_link = drv_link->next;
	}

	return highest_score;
}

/** Read match id at the specified position of a string and set the position in
 * the string to the first character following the id.
 *
 * @param buf		The position in the input string.
 * @return		The match id.
 */
char *read_match_id(char **buf)
{
	char *res = NULL;
	size_t len = get_nonspace_len(*buf);

	if (len > 0) {
		res = malloc(len + 1);
		if (res != NULL) {
			str_ncpy(res, len + 1, *buf, len);
			*buf += len;
		}
	}

	return res;
}

/**
 * Read match ids and associated match scores from a string.
 *
 * Each match score in the string is followed by its match id.
 * The match ids and match scores are separated by whitespaces.
 * Neither match ids nor match scores can contain whitespaces.
 *
 * @param buf		The string from which the match ids are read.
 * @param ids		The list of match ids into which the match ids and
 *			scores are added.
 * @return		True if at least one match id and associated match score
 *			was successfully read, false otherwise.
 */
bool parse_match_ids(char *buf, match_id_list_t *ids)
{
	int score = 0;
	char *id = NULL;
	int ids_read = 0;

	while (true) {
		/* skip spaces */
		if (!skip_spaces(&buf))
			break;

		if (*buf == COMMENT) {
			skip_line(&buf);
			continue;
		}

		/* read score */
		score = strtoul(buf, &buf, 10);

		/* skip spaces */
		if (!skip_spaces(&buf))
			break;

		/* read id */
		id = read_match_id(&buf);
		if (NULL == id)
			break;

		/* create new match_id structure */
		match_id_t *mid = create_match_id();
		mid->id = id;
		mid->score = score;

		/* add it to the list */
		add_match_id(ids, mid);

		ids_read++;
	}

	return ids_read > 0;
}

/**
 * Read match ids and associated match scores from a file.
 *
 * Each match score in the file is followed by its match id.
 * The match ids and match scores are separated by whitespaces.
 * Neither match ids nor match scores can contain whitespaces.
 *
 * @param buf		The path to the file from which the match ids are read.
 * @param ids		The list of match ids into which the match ids and
 *			scores are added.
 * @return		True if at least one match id and associated match score
 *			was successfully read, false otherwise.
 */
bool read_match_ids(const char *conf_path, match_id_list_t *ids)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "read_match_ids(conf_path=\"%s\")", conf_path);

	bool suc = false;
	char *buf = NULL;
	bool opened = false;
	int fd;
	size_t len = 0;
	vfs_stat_t st;

	errno_t rc = vfs_lookup_open(conf_path, WALK_REGULAR, MODE_READ, &fd);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Unable to open `%s' for reading: %s.",
		    conf_path, str_error(rc));
		goto cleanup;
	}
	opened = true;

	rc = vfs_stat(fd, &st);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Unable to fstat %d: %s.", fd,
		    str_error(rc));
		goto cleanup;
	}
	len = st.size;
	if (len == 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Configuration file '%s' is empty.",
		    conf_path);
		goto cleanup;
	}

	buf = malloc(len + 1);
	if (buf == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Memory allocation failed when parsing file "
		    "'%s'.", conf_path);
		goto cleanup;
	}

	size_t read_bytes;
	rc = vfs_read(fd, (aoff64_t []) { 0 }, buf, len, &read_bytes);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Unable to read file '%s': %s.", conf_path,
		    str_error(rc));
		goto cleanup;
	}
	buf[read_bytes] = 0;

	suc = parse_match_ids(buf, ids);

cleanup:
	free(buf);

	if (opened)
		vfs_put(fd);

	return suc;
}

/** @}
 */
