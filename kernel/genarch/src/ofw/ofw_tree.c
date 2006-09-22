/*
 * Copyright (C) 2006 Jakub Jermar
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

/** @addtogroup ofw
 * @{
 */
/**
 * @file
 * @brief	OpenFirmware device tree navigation.
 *
 */

#include <genarch/ofw/ofw_tree.h>
#include <arch/memstr.h>
#include <func.h>
#include <print.h>
#include <panic.h>

#define PATH_MAX_LEN	80
#define NAME_BUF_LEN	50

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
 * @return Pointer to the property structure or NULL if no such property.
 */
ofw_tree_property_t *ofw_tree_getprop(const ofw_tree_node_t *node, const char *name)
{
	int i;
	
	for (i = 0; i < node->properties; i++) {
		if (strcmp(node->property[i].name, name) == 0)
			return &node->property[i];
	}

	return NULL;
}

/** Return value of the 'name' property.
 *
 * @param node Node of interest.
 *
 * @return Value of the 'name' property belonging to the node.
 */
const char *ofw_tree_node_name(const ofw_tree_node_t *node)
{
	ofw_tree_property_t *prop;
	
	prop = ofw_tree_getprop(node, "name");
	if (!prop)
		panic("Node without name property.\n");
		
	if (prop->size < 2)
		panic("Invalid name property.\n");
	
	return prop->value;
}

/** Lookup child of given name.
 *
 * @param node Node whose child is being looked up.
 * @param name Name of the child being looked up.
 *
 * @return NULL if there is no such child or pointer to the matching child node.
 */
static ofw_tree_node_t *ofw_tree_find_child(ofw_tree_node_t *node, const char *name)
{
	ofw_tree_node_t *cur;
	
	/*
	 * Try to find the disambigued name.
	 */
	for (cur = node->child; cur; cur = cur->peer) {
		if (strcmp(cur->da_name, name) == 0)
			return cur;
	}
	
	/*
	 * Disambigued name not found.
	 * Lets try our luck with possibly ambiguous "name" property.
	 *
	 * We need to do this because paths stored in "/aliases"
	 * are not always fully-qualified.
	 */
	for (cur = node->child; cur; cur = cur->peer) {
		if (strcmp(ofw_tree_node_name(cur), name) == 0)
			return cur;
	}
		
	return NULL;
}

/** Lookup OpenFirmware node by its path.
 *
 * @param path Path to the node.
 *
 * @return NULL if there is no such node or pointer to the leaf node.
 */
ofw_tree_node_t *ofw_tree_lookup(const char *path)
{
	char buf[NAME_BUF_LEN+1];
	ofw_tree_node_t *node = ofw_root;
	index_t i, j;
	
	if (path[0] != '/')
		return NULL;
	
	for (i = 1; i < strlen(path) && node; i = j + 1) {
		for (j = i; j < strlen(path) && path[j] != '/'; j++)
			;
		if (i == j)	/* skip extra slashes */
			continue;
			
		memcpy(buf, &path[i], j - i);
		buf[j - i] = '\0';
		node = ofw_tree_find_child(node, buf);
	}
	
	return node;
}

/** Recursively print subtree rooted in a node.
 *
 * @param node Root of the subtree.
 * @param path Current path, NULL for the very root of the entire tree.
 */
static void ofw_tree_node_print(const ofw_tree_node_t *node, const char *path)
{
	char p[PATH_MAX_LEN];
	
	if (node->parent) {
		snprintf(p, PATH_MAX_LEN, "%s/%s", path, node->da_name);
		printf("%s\n", p);
	} else {
		snprintf(p, PATH_MAX_LEN, "%s", node->da_name);
		printf("/\n");
	}

	if (node->child)
		ofw_tree_node_print(node->child, p);
	
	if (node->peer)
		ofw_tree_node_print(node->peer, path);
}

/** Print the structure of the OpenFirmware device tree. */
void ofw_tree_print(void)
{
	ofw_tree_node_print(ofw_root, NULL);
}

/** @}
 */
