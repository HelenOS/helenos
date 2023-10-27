/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief OpenFirmware device tree navigation.
 *
 */

#include <genarch/ofw/ofw_tree.h>
#include <stdlib.h>
#include <sysinfo/sysinfo.h>
#include <memw.h>
#include <str.h>
#include <panic.h>
#include <stdio.h>

#define PATH_MAX_LEN  256
#define NAME_BUF_LEN  50

static ofw_tree_node_t *ofw_root;

void ofw_tree_init(ofw_tree_node_t *root)
{
	ofw_root = root;
}

/** Get OpenFirmware node property.
 *
 * @param node Node in which to lookup the property.
 * @param name Name of the property.
 *
 * @return Pointer to the property structure or NULL if no such
 *         property.
 *
 */
ofw_tree_property_t *ofw_tree_getprop(const ofw_tree_node_t *node,
    const char *name)
{
	for (size_t i = 0; i < node->properties; i++) {
		if (str_cmp(node->property[i].name, name) == 0)
			return &node->property[i];
	}

	return NULL;
}

/** Return value of the 'name' property.
 *
 * @param node Node of interest.
 *
 * @return Value of the 'name' property belonging to the node
 *         or NULL if the property is invalid.
 *
 */
const char *ofw_tree_node_name(const ofw_tree_node_t *node)
{
	ofw_tree_property_t *prop = ofw_tree_getprop(node, "name");
	if ((!prop) || (prop->size < 2))
		return NULL;

	return prop->value;
}

/** Lookup child of given name.
 *
 * @param node Node whose child is being looked up.
 * @param name Name of the child being looked up.
 *
 * @return NULL if there is no such child or pointer to the
 *         matching child node.
 *
 */
ofw_tree_node_t *ofw_tree_find_child(ofw_tree_node_t *node,
    const char *name)
{
	/*
	 * Try to find the disambigued name.
	 */
	for (ofw_tree_node_t *cur = node->child; cur; cur = cur->peer) {
		if (str_cmp(cur->da_name, name) == 0)
			return cur;
	}

	/*
	 * Disambigued name not found.
	 * Lets try our luck with possibly ambiguous "name" property.
	 *
	 * We need to do this because paths stored in "/aliases"
	 * are not always fully-qualified.
	 */
	for (ofw_tree_node_t *cur = node->child; cur; cur = cur->peer) {
		if (str_cmp(ofw_tree_node_name(cur), name) == 0)
			return cur;
	}

	return NULL;
}

/** Lookup first child of given device type.
 *
 * @param node  Node whose child is being looked up.
 * @param dtype Device type of the child being looked up.
 *
 * @return NULL if there is no such child or pointer to the
 *         matching child node.
 *
 */
ofw_tree_node_t *ofw_tree_find_child_by_device_type(ofw_tree_node_t *node,
    const char *dtype)
{
	for (ofw_tree_node_t *cur = node->child; cur; cur = cur->peer) {
		ofw_tree_property_t *prop =
		    ofw_tree_getprop(cur, "device_type");

		if ((!prop) || (!prop->value))
			continue;

		if (str_cmp(prop->value, dtype) == 0)
			return cur;
	}

	return NULL;
}

/** Lookup node with matching node_handle.
 *
 * Child nodes are looked up recursively contrary to peer nodes that
 * are looked up iteratively to avoid stack overflow.
 *
 * @param root   Root of the searched subtree.
 * @param handle OpenFirmware handle.
 *
 * @return NULL if there is no such node or pointer to the matching
 *         node.
 *
 */
ofw_tree_node_t *ofw_tree_find_node_by_handle(ofw_tree_node_t *root,
    phandle handle)
{
	for (ofw_tree_node_t *cur = root; cur; cur = cur->peer) {
		if (cur->node_handle == handle)
			return cur;

		if (cur->child) {
			ofw_tree_node_t *node =
			    ofw_tree_find_node_by_handle(cur->child, handle);
			if (node)
				return node;
		}
	}

	return NULL;
}

/** Lookup first peer of given device type.
 *
 * @param node  Node whose peer is being looked up.
 * @param dtype Device type of the child being looked up.
 *
 * @return NULL if there is no such child or pointer to the
 *         matching child node.
 *
 */
ofw_tree_node_t *ofw_tree_find_peer_by_device_type(ofw_tree_node_t *node,
    const char *dtype)
{
	for (ofw_tree_node_t *cur = node->peer; cur; cur = cur->peer) {
		ofw_tree_property_t *prop =
		    ofw_tree_getprop(cur, "device_type");

		if ((!prop) || (!prop->value))
			continue;

		if (str_cmp(prop->value, dtype) == 0)
			return cur;
	}

	return NULL;
}

/** Lookup first peer of given name.
 *
 * @param node Node whose peer is being looked up.
 * @param name Name of the child being looked up.
 *
 * @return NULL if there is no such peer or pointer to the matching
 *         peer node.
 *
 */
ofw_tree_node_t *ofw_tree_find_peer_by_name(ofw_tree_node_t *node,
    const char *name)
{
	for (ofw_tree_node_t *cur = node->peer; cur; cur = cur->peer) {
		ofw_tree_property_t *prop =
		    ofw_tree_getprop(cur, "name");

		if ((!prop) || (!prop->value))
			continue;

		if (str_cmp(prop->value, name) == 0)
			return cur;
	}

	return NULL;
}

/** Lookup OpenFirmware node by its path.
 *
 * @param path Path to the node.
 *
 * @return NULL if there is no such node or pointer to the leaf
 *         node.
 *
 */
ofw_tree_node_t *ofw_tree_lookup(const char *path)
{
	if (path[0] != '/')
		return NULL;

	ofw_tree_node_t *node = ofw_root;
	size_t j;

	for (size_t i = 1; (i < str_size(path)) && (node); i = j + 1) {
		j = i;
		while (j < str_size(path) && path[j] != '/')
			j++;

		/* Skip extra slashes */
		if (i == j)
			continue;

		char buf[NAME_BUF_LEN + 1];
		memcpy(buf, &path[i], j - i);
		buf[j - i] = 0;
		node = ofw_tree_find_child(node, buf);
	}

	return node;
}

/** Walk the OpenFirmware device subtree rooted in a node.
 *
 * Child nodes are processed recursively and peer nodes are processed
 * iteratively in order to avoid stack overflow.
 *
 * @param node   Root of the subtree.
 * @param dtype  Device type to look for.
 * @param walker Routine to be invoked on found device.
 * @param arg    User argument to the walker.
 *
 * @return True if the walk should continue.
 *
 */
static bool ofw_tree_walk_by_device_type_internal(ofw_tree_node_t *node,
    const char *dtype, ofw_tree_walker_t walker, void *arg)
{
	for (ofw_tree_node_t *cur = node; cur; cur = cur->peer) {
		ofw_tree_property_t *prop =
		    ofw_tree_getprop(cur, "device_type");

		if ((prop) && (prop->value) && (str_cmp(prop->value, dtype) == 0)) {
			bool ret = walker(cur, arg);
			if (!ret)
				return false;
		}

		if (cur->child) {
			bool ret =
			    ofw_tree_walk_by_device_type_internal(cur->child, dtype, walker, arg);
			if (!ret)
				return false;
		}
	}

	return true;
}

/** Walk the OpenFirmware device tree and find devices by type.
 *
 * Walk the whole OpenFirmware device tree and if any node has
 * the property "device_type" equal to dtype, run a walker on it.
 * If the walker returns false, the walk does not continue.
 *
 * @param dtype  Device type to look for.
 * @param walker Routine to be invoked on found device.
 * @param arg    User argument to the walker.
 *
 */
void ofw_tree_walk_by_device_type(const char *dtype, ofw_tree_walker_t walker,
    void *arg)
{
	(void) ofw_tree_walk_by_device_type_internal(ofw_root, dtype, walker, arg);
}

/** Get OpenFirmware node properties.
 *
 * @param item    Sysinfo item (unused).
 * @param size    Size of the returned data.
 * @param dry_run Do not get the data, just calculate the size.
 * @param data    OpenFirmware node.
 *
 * @return Data containing a serialized dump of all node
 *         properties. If the return value is not NULL, it
 *         should be freed in the context of the sysinfo request.
 *
 */
static void *ofw_sysinfo_properties(struct sysinfo_item *item, size_t *size,
    bool dry_run, void *data)
{
	ofw_tree_node_t *node = (ofw_tree_node_t *) data;

	/* Compute serialized data size */
	*size = 0;
	for (size_t i = 0; i < node->properties; i++)
		*size += str_size(node->property[i].name) + 1 +
		    sizeof(node->property[i].size) + node->property[i].size;

	if (dry_run)
		return NULL;

	void *dump = malloc(*size);
	if (dump == NULL) {
		*size = 0;
		return NULL;
	}

	/* Serialize the data */
	size_t pos = 0;
	for (size_t i = 0; i < node->properties; i++) {
		/* Property name */
		str_cpy(dump + pos, *size - pos, node->property[i].name);
		pos += str_size(node->property[i].name) + 1;

		/* Value size */
		memcpy(dump + pos, &node->property[i].size,
		    sizeof(node->property[i].size));
		pos += sizeof(node->property[i].size);

		/* Value */
		memcpy(dump + pos, node->property[i].value,
		    node->property[i].size);
		pos += node->property[i].size;
	}

	return ((void *) dump);
}

/** Map OpenFirmware device subtree rooted in a node into sysinfo.
 *
 * Child nodes are processed recursively and peer nodes are processed
 * iteratively in order to avoid stack overflow.
 *
 * @param node Root of the subtree.
 * @param path Current path, NULL for the very root of the entire tree.
 *
 */
static void ofw_tree_node_sysinfo(ofw_tree_node_t *node, const char *path)
{
	char *cur_path = malloc(PATH_MAX_LEN);
	if (!cur_path)
		panic("Not enough memory to process OFW tree.");

	for (ofw_tree_node_t *cur = node; cur; cur = cur->peer) {
		if ((cur->parent) && (path))
			snprintf(cur_path, PATH_MAX_LEN, "%s.%s", path, cur->da_name);
		else if (!str_size(cur->da_name))
			snprintf(cur_path, PATH_MAX_LEN, "firmware.ofw");
		else
			snprintf(cur_path, PATH_MAX_LEN, "firmware.%s", cur->da_name);

		sysinfo_set_item_gen_data(cur_path, NULL, ofw_sysinfo_properties,
		    (void *) cur);

		if (cur->child)
			ofw_tree_node_sysinfo(cur->child, cur_path);
	}

	free(cur_path);
}

/** Map the OpenFirmware device tree into sysinfo. */
void ofw_sysinfo_map(void)
{
	ofw_tree_node_sysinfo(ofw_root, NULL);
}

/** @}
 */
