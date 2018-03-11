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

#include <genarch/ofw_tree.h>
#include <genarch/ofw.h>
#include <arch/ofw.h>
#include <stddef.h>
#include <str.h>
#include <balloc.h>
#include <memstr.h>

static char path[OFW_TREE_PATH_MAX_LEN + 1];
static char name[OFW_TREE_PROPERTY_MAX_NAMELEN];
static char name2[OFW_TREE_PROPERTY_MAX_NAMELEN];

static ofw_tree_node_t *ofw_tree_node_alloc(void)
{
	return balloc(sizeof(ofw_tree_node_t), sizeof(ofw_tree_node_t));
}

static ofw_tree_property_t *ofw_tree_properties_alloc(size_t count)
{
	return balloc(count * sizeof(ofw_tree_property_t),
	    sizeof(ofw_tree_property_t));
}

static void *ofw_tree_space_alloc(size_t size)
{
	/*
	 * What we do here is a nasty hack :-)
	 * Problem: string property values that are allocated via this
	 * function typically do not contain the trailing '\0'. This
	 * is very uncomfortable for kernel, which is supposed to deal
	 * with the properties.
	 * Solution: when allocating space via this function, we always
	 * allocate space for the extra '\0' character that we store
	 * behind the requested memory.
	 */
	char *addr = balloc(size + 1, 4);
	if (addr)
		addr[size] = '\0';

	return addr;
}

/** Transfer information from one OpenFirmware node into its memory
 * representation.
 *
 * Transfer entire information from the OpenFirmware device tree 'current' node
 * to its memory representation in 'current_node'. This function recursively
 * processes all node's children. Node's peers are processed iteratively in
 * order to prevent stack from overflowing.
 *
 * @param current_node Pointer to uninitialized ofw_tree_node structure that
 *                     will become the memory represenation of 'current'.
 * @param parent_node  Parent ofw_tree_node structure or NULL in case of root
 *                     node.
 * @param current      OpenFirmware phandle to the current device tree node.
 *
 */
static void ofw_tree_node_process(ofw_tree_node_t *current_node,
    ofw_tree_node_t *parent_node, phandle current)
{
	while (current_node) {
		/*
		 * Initialize node.
		 */
		current_node->parent = (ofw_tree_node_t *) balloc_rebase(parent_node);
		current_node->peer = NULL;
		current_node->child = NULL;
		current_node->node_handle = current;
		current_node->properties = 0;
		current_node->property = NULL;
		current_node->device = NULL;

		/*
		 * Get the disambigued name.
		 */
		size_t len = ofw_package_to_path(current, path, OFW_TREE_PATH_MAX_LEN);
		if (len == (size_t) -1)
			return;

		path[len] = '\0';

		/* Find last slash */
		size_t i;
		i = len;
		while (i > 0 && path[i - 1] != '/')
			i--;

		/* Do not include the slash */
		len -= i;

		/* Add space for trailing '\0' */
		char *da_name = ofw_tree_space_alloc(len + 1);
		if (!da_name)
			return;

		memcpy(da_name, &path[i], len);
		da_name[len] = '\0';
		current_node->da_name = (char *) balloc_rebase(da_name);

		/*
		 * Recursively process the potential child node.
		 */
		phandle child = ofw_get_child_node(current);
		if ((child != 0) && (child != (phandle) -1)) {
			ofw_tree_node_t *child_node = ofw_tree_node_alloc();
			if (child_node) {
				ofw_tree_node_process(child_node, current_node,
				    child);
				current_node->child =
				    (ofw_tree_node_t *) balloc_rebase(child_node);
			}
		}

		/*
		 * Count properties.
		 */
		name[0] = '\0';
		while (ofw_next_property(current, name, name2) == 1) {
			current_node->properties++;
			memcpy(name, name2, OFW_TREE_PROPERTY_MAX_NAMELEN);
		}

		if (!current_node->properties)
			return;

		/*
		 * Copy properties.
		 */
		ofw_tree_property_t *property =
		    ofw_tree_properties_alloc(current_node->properties);
		if (!property)
			return;

		name[0] = '\0';
		for (i = 0; ofw_next_property(current, name, name2) == 1; i++) {
			if (i == current_node->properties)
				break;

			memcpy(name, name2, OFW_TREE_PROPERTY_MAX_NAMELEN);
			memcpy(property[i].name, name, OFW_TREE_PROPERTY_MAX_NAMELEN);
			property[i].name[OFW_TREE_PROPERTY_MAX_NAMELEN - 1] = '\0';

			size_t size = ofw_get_proplen(current, name);
			property[i].size = size;

			if (size) {
				void *buf = ofw_tree_space_alloc(size);
				if (buf) {
					/*
					 * Copy property value to memory node.
					 */
					(void) ofw_get_property(current, name, buf, size);
					property[i].value = balloc_rebase(buf);
				}
			} else
				property[i].value = NULL;
		}

		/* Just in case we ran out of memory. */
		current_node->properties = i;
		current_node->property = (ofw_tree_property_t *) balloc_rebase(property);

		/*
		 * Iteratively process the next peer node.
		 * Note that recursion is a bad idea here.
		 * Due to the topology of the OpenFirmware device tree,
		 * the nesting of peer nodes could be to wide and the
		 * risk of overflowing the stack is too real.
		 */
		phandle peer = ofw_get_peer_node(current);
		if ((peer != 0) && (peer != (phandle) -1)) {
			ofw_tree_node_t *peer_node = ofw_tree_node_alloc();
			if (peer_node) {
				current_node->peer = (ofw_tree_node_t *) balloc_rebase(peer_node);
				current_node = peer_node;
				current = peer;
				/*
				 * Process the peer in next iteration.
				 */
				continue;
			}
		}

		/*
		 * No more peers on this level.
		 */
		break;
	}
}

/** Construct memory representation of OpenFirmware device tree.
 *
 * @return NULL on failure or kernel pointer to the root node.
 *
 */
ofw_tree_node_t *ofw_tree_build(void)
{
	ofw_tree_node_t *root = ofw_tree_node_alloc();
	if (root)
		ofw_tree_node_process(root, NULL, ofw_root);

	/*
	 * The firmware client interface does not automatically include the
	 * "ssm" node in the list of children of "/". A nasty yet working
	 * solution is to explicitly stick "ssm" to the OFW tree.
	 */
	phandle ssm_node = ofw_find_device("/ssm@0,0");
	if (ssm_node != (phandle) -1) {
		ofw_tree_node_t *ssm = ofw_tree_node_alloc();
		if (ssm) {
			ofw_tree_node_process(ssm, root,
			    ofw_find_device("/ssm@0,0"));
			ssm->peer = root->child;
			root->child = (ofw_tree_node_t *) balloc_rebase(ssm);
		}
	}

	return (ofw_tree_node_t *) balloc_rebase(root);
}
