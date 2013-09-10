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
/** @file Device Manager
 *
 * Locking order:
 *   (1) driver_t.driver_mutex
 *   (2) dev_tree_t.rwlock
 *
 * Synchronization:
 *    - device_tree.rwlock protects:
 *        - tree root, complete tree topology
 *        - complete contents of device and function nodes
 *    - dev_node_t.refcnt, fun_node_t.refcnt prevent nodes from
 *      being deallocated
 *    - find_xxx() functions increase reference count of returned object
 *    - find_xxx_no_lock() do not increase reference count
 *
 * TODO
 *    - Track all steady and transient device/function states
 *    - Check states, wait for steady state on certain operations
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io/log.h>
#include <ipc/driver.h>
#include <ipc/devman.h>
#include <loc.h>
#include <str_error.h>
#include <stdio.h>

#include "dev.h"
#include "devman.h"
#include "driver.h"
#include "fun.h"

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
	
	fd = open(conf_path, O_RDONLY);
	if (fd < 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Unable to open `%s' for reading: %s.",
		    conf_path, str_error(fd));
		goto cleanup;
	}
	opened = true;
	
	len = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
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
	
	ssize_t read_bytes = read_all(fd, buf, len);
	if (read_bytes <= 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Unable to read file '%s' (%zd).", conf_path,
		    read_bytes);
		goto cleanup;
	}
	buf[read_bytes] = 0;
	
	suc = parse_match_ids(buf, ids);
	
cleanup:
	free(buf);
	
	if (opened)
		close(fd);
	
	return suc;
}

/** Create loc path and name for the function. */
void loc_register_tree_function(fun_node_t *fun, dev_tree_t *tree)
{
	char *loc_pathname = NULL;
	char *loc_name = NULL;
	
	assert(fibril_rwlock_is_locked(&tree->rwlock));
	
	asprintf(&loc_name, "%s", fun->pathname);
	if (loc_name == NULL)
		return;
	
	replace_char(loc_name, '/', LOC_SEPARATOR);
	
	asprintf(&loc_pathname, "%s/%s", LOC_DEVICE_NAMESPACE,
	    loc_name);
	if (loc_pathname == NULL) {
		free(loc_name);
		return;
	}
	
	loc_service_register_with_iface(loc_pathname,
	    &fun->service_id, DEVMAN_CONNECT_FROM_LOC);
	
	tree_add_loc_function(tree, fun);
	
	free(loc_name);
	free(loc_pathname);
}

/** Pass a device to running driver.
 *
 * @param drv		The driver's structure.
 * @param node		The device's node in the device tree.
 */
void add_device(driver_t *drv, dev_node_t *dev, dev_tree_t *tree)
{
	/*
	 * We do not expect to have driver's mutex locked as we do not
	 * access any structures that would affect driver_t.
	 */
	log_msg(LOG_DEFAULT, LVL_DEBUG, "add_device(drv=\"%s\", dev=\"%s\")",
	    drv->name, dev->pfun->name);
	
	/* Send the device to the driver. */
	devman_handle_t parent_handle;
	if (dev->pfun) {
		parent_handle = dev->pfun->handle;
	} else {
		parent_handle = 0;
	}
	
	async_exch_t *exch = async_exchange_begin(drv->sess);
	
	ipc_call_t answer;
	aid_t req = async_send_2(exch, DRIVER_DEV_ADD, dev->handle,
	    parent_handle, &answer);
	
	/* Send the device name to the driver. */
	sysarg_t rc = async_data_write_start(exch, dev->pfun->name,
	    str_size(dev->pfun->name) + 1);
	
	async_exchange_end(exch);
	
	if (rc != EOK) {
		/* TODO handle error */
	}

	/* Wait for answer from the driver. */
	async_wait_for(req, &rc);

	switch(rc) {
	case EOK:
		dev->state = DEVICE_USABLE;
		break;
	case ENOENT:
		dev->state = DEVICE_NOT_PRESENT;
		break;
	default:
		dev->state = DEVICE_INVALID;
		break;
	}
	
	dev->passed_to_driver = true;

	return;
}

/* loc devices */

fun_node_t *find_loc_tree_function(dev_tree_t *tree, service_id_t service_id)
{
	fun_node_t *fun = NULL;
	
	fibril_rwlock_read_lock(&tree->rwlock);
	ht_link_t *link = hash_table_find(&tree->loc_functions, &service_id);
	if (link != NULL) {
		fun = hash_table_get_inst(link, fun_node_t, loc_fun);
		fun_add_ref(fun);
	}
	fibril_rwlock_read_unlock(&tree->rwlock);
	
	return fun;
}

void tree_add_loc_function(dev_tree_t *tree, fun_node_t *fun)
{
	assert(fibril_rwlock_is_write_locked(&tree->rwlock));
	
	hash_table_insert(&tree->loc_functions, &fun->loc_fun);
}

/** @}
 */
