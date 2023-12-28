/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup libofw
 * @{
 */
/**
 * @file OpenFirmware device tree access
 */

#include <assert.h>
#include <errno.h>
#include <ofw.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <sysinfo.h>

/** Convert OpenFirmware path to sysinfo path.
 *
 * @param ofwpath OpenFirmware path
 * @param sipath place to store pointer to newly allocated sysinfo path
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t ofw_path_to_sipath(const char *ofwpath, char **sipath)
{
	int rv;
	size_t size;
	size_t i;

	if (str_cmp(ofwpath, "/") == 0)
		ofwpath = "";

	/* Append path to firmware.ofw */
	rv = asprintf(sipath, "firmware.ofw%s", ofwpath);
	if (rv < 0)
		return ENOMEM;

	/* Replace '/' with '.' */
	size = str_size(*sipath);
	for (i = 0; i < size; i++)
		if ((*sipath)[i] == '/')
			(*sipath)[i] = '.';

	return EOK;
}

/** Initialize child iterator with first child entry.
 *
 * Initialize the child iterator and make it point to the first child
 * of the specified OpenFirmware device node.
 *
 * @param it Child iterator to initialize
 * @param ofwpath OpenFirmware path of the parent node
 */
errno_t ofw_child_it_first(ofw_child_it_t *it, const char *ofwpath)
{
	errno_t rc;
	char *sipath = NULL;

	it->ofwpath = NULL;
	it->keys = NULL;
	it->keys_sz = 0;
	it->pos = 0;

	rc = ofw_path_to_sipath(ofwpath, &sipath);
	if (rc != EOK)
		goto error;

	if (str_cmp(ofwpath, "/") == 0) {
		it->ofwpath = str_dup("");
	} else {
		it->ofwpath = str_dup(ofwpath);
	}

	if (it->ofwpath == NULL) {
		rc = ENOMEM;
		goto error;
	}

	it->keys = sysinfo_get_keys(sipath, &it->keys_sz);
	if (it->keys == NULL) {
		rc = ENOENT;
		goto error;
	}

	return EOK;
error:
	if (sipath != NULL)
		free(sipath);

	if (it->ofwpath != NULL) {
		free(it->ofwpath);
		it->ofwpath = NULL;
	}

	return rc;
}

/** Move child iterator to the next child.
 *
 * @param it Child iterator
 */
void ofw_child_it_next(ofw_child_it_t *it)
{
	size_t adj;

	assert(!ofw_child_it_end(it));

	adj = str_nsize(it->keys + it->pos, it->keys_sz - it->pos) + 1;
	assert(it->pos + adj <= it->keys_sz);
	it->pos += adj;
}

/** Determine if there are no more child nodes.
 *
 * @param it Child iterator
 * @return @c true iff iterator is beyond the last entry
 */
bool ofw_child_it_end(ofw_child_it_t *it)
{
	return it->pos >= it->keys_sz;
}

/** Get child name from iterator.
 *
 * @param it Child iterator
 * @return Child name (valid until next operation on @a it
 */
const char *ofw_child_it_get_name(ofw_child_it_t *it)
{
	assert(!ofw_child_it_end(it));
	return it->keys + it->pos;
}

/** Get child path from iterator.
 *
 * @param it Child iterator
 * @param rpath Place to store pointer to allocated OpenFirmware path
 * @return @c EOK on success or an error code
 */
errno_t ofw_child_it_get_path(ofw_child_it_t *it, char **rpath)
{
	const char *name;
	int rv;

	assert(!ofw_child_it_end(it));
	name = ofw_child_it_get_name(it);

	rv = asprintf(rpath, "%s/%s", it->ofwpath, name);
	if (rv < 0)
		return ENOMEM;

	return EOK;
}

/** Finalize child iterator.
 *
 * This must be called after using a child iterator.
 *
 * @param it Child iterator
 */
void ofw_child_it_fini(ofw_child_it_t *it)
{
	free(it->ofwpath);
	free(it->keys);
	it->keys = NULL;
	it->keys_sz = 0;
}

/** Initialize property iterator with property.
 *
 * Initialize the property iterator and make it point to the first property
 * of the specified OpenFirmware device node.
 *
 * @param it Property iterator to initialize
 * @param ofwpath OpenFirmware path of the node
 */
errno_t ofw_prop_it_first(ofw_prop_it_t *it, const char *ofwpath)
{
	errno_t rc;
	char *sipath;

	it->data = NULL;
	it->data_sz = 0;
	it->pos = 0;

	rc = ofw_path_to_sipath(ofwpath, &sipath);
	if (rc != EOK)
		goto error;

	it->data = sysinfo_get_data(sipath, &it->data_sz);
	if (it->data == NULL) {
		rc = ENOENT;
		goto error;
	}

	return EOK;
error:
	if (sipath != NULL)
		free(sipath);
	return rc;
}

/** Move property iterator to the next property.
 *
 * @param it Child iterator
 */
void ofw_prop_it_next(ofw_prop_it_t *it)
{
	size_t cur_size;
	size_t value_size;

	cur_size = str_nsize(it->data + it->pos, it->data_sz - it->pos);
	if (((char *) it->data)[it->pos + cur_size] != 0)
		return;

	it->pos += cur_size + 1;

	/* Process value size */
	memcpy(&value_size, it->data + it->pos, sizeof(value_size));

	it->pos += sizeof(value_size);
	if ((it->pos >= it->data_sz) || (it->pos + value_size > it->data_sz))
		return;

	it->pos += value_size;
}

/** Determine if there are no more properties.
 *
 * @param it Property iterator
 * @return @c true iff iterator is beyond the last entry
 */
bool ofw_prop_it_end(ofw_prop_it_t *it)
{
	return it->pos >= it->data_sz;
}

/** Get property name from iterator.
 *
 * @param it Child iterator
 * @return Property name (valid until next operation on @a it
 */
const char *ofw_prop_it_get_name(ofw_prop_it_t *it)
{
	assert(!ofw_prop_it_end(it));
	return it->data + it->pos;
}

/** Get property data from iterator.
 *
 * @param it Property iterator
 * @param rsize Place to store size of data
 * @return Pointer to data
 */
const void *ofw_prop_it_get_data(ofw_prop_it_t *it, size_t *rsize)
{
	size_t cur_size;
	size_t value_size;
	size_t pos;

	assert(!ofw_prop_it_end(it));
	pos = it->pos;

	cur_size = str_nsize(it->data + pos, it->data_sz - pos);
	if (((char *) it->data)[pos + cur_size] != 0)
		return NULL;

	pos += cur_size + 1;

	/* Process value size */
	memcpy(&value_size, it->data + pos, sizeof(value_size));
	pos += sizeof(value_size);

	*rsize = value_size;
	return it->data + pos;
}

/** Finalize property iterator.
 *
 * This must be called after using a child iterator.
 *
 * @param it Child iterator
 */
void ofw_prop_it_fini(ofw_prop_it_t *it)
{
	free(it->data);
	it->data = NULL;
	it->data_sz = 0;
	it->pos = 0;
}

/** @}
 */
