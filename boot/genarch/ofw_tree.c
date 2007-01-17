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

#include <ofw_tree.h>
#include <ofw.h>
#include <types.h>
#include <string.h>
#include <balloc.h>
#include <asm.h>

#define MAX_PATH_LEN	256

static ofw_tree_node_t *ofw_tree_node_alloc(void)
{
	return balloc(sizeof(ofw_tree_node_t), sizeof(ofw_tree_node_t));
}

static ofw_tree_property_t *ofw_tree_properties_alloc(unsigned count)
{
	return balloc(count * sizeof(ofw_tree_property_t), sizeof(ofw_tree_property_t));
}

static void * ofw_tree_space_alloc(size_t size)
{
	char *addr;

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
	addr = balloc(size + 1, size);
	if (addr)
		addr[size] = '\0';
	return addr;
}

/** Transfer information from one OpenFirmware node into its memory representation.
 *
 * Transfer entire information from the OpenFirmware device tree 'current' node to
 * its memory representation in 'current_node'. This function recursively processes
 * all node's children. Node's peers are processed iteratively in order to prevent
 * stack from overflowing.
 *
 * @param current_node	Pointer to uninitialized ofw_tree_node structure that will
 * 			become the memory represenation of 'current'.
 * @param parent_node	Parent ofw_tree_node structure or NULL in case of root node.
 * @param current	OpenFirmware phandle to the current device tree node.
 */
static void ofw_tree_node_process(ofw_tree_node_t *current_node,
	ofw_tree_node_t *parent_node, phandle current)
{
	static char path[MAX_PATH_LEN+1];
	static char name[OFW_TREE_PROPERTY_MAX_NAMELEN];
	phandle peer;
	phandle child;
	size_t len;
	int i;

	while (current_node) {
		/*
		 * Initialize node.
		 */
		current_node->parent = parent_node;
		current_node->peer = NULL;
		current_node->child = NULL;
		current_node->node_handle = current;
		current_node->properties = 0;
		current_node->property = NULL;
		current_node->device = NULL;
	
		/*
		 * Get the disambigued name.
		 */
		len = ofw_package_to_path(current, path, MAX_PATH_LEN);
		if (len == -1)
			return;
	
		path[len] = '\0';
		for (i = len - 1; i >= 0 && path[i] != '/'; i--)
			;
		i++;	/* do not include '/' */
	
		len -= i;

		/* add space for trailing '\0' */
		current_node->da_name = ofw_tree_space_alloc(len + 1);
		if (!current_node->da_name)
			return;
	
		memcpy(current_node->da_name, &path[i], len);
		current_node->da_name[len] = '\0';
	
	
		/*
		 * Recursively process the potential child node.
		 */
		child = ofw_get_child_node(current);
		if (child != 0 && child != -1) {
			ofw_tree_node_t *child_node;
		
			child_node = ofw_tree_node_alloc();
			if (child_node) {
				ofw_tree_node_process(child_node, current_node, child);
				current_node->child = child_node;
			}
		}
	
		/*
		 * Count properties.
		 */
		name[0] = '\0';
		while (ofw_next_property(current, name, name) == 1)
			current_node->properties++;
	
		if (!current_node->properties)
			return;
	
		/*
		 * Copy properties.
		 */
		current_node->property = ofw_tree_properties_alloc(current_node->properties);
		if (!current_node->property)
			return;
		
		name[0] = '\0';
		for (i = 0; ofw_next_property(current, name, name) == 1; i++) {
			size_t size;
		
			if (i == current_node->properties)
				break;
		
			memcpy(current_node->property[i].name, name,
				OFW_TREE_PROPERTY_MAX_NAMELEN);
			current_node->property[i].name[OFW_TREE_PROPERTY_MAX_NAMELEN] = '\0';

			size = ofw_get_proplen(current, name);
			current_node->property[i].size = size;
			if (size) {
				void *buf;
			
				buf = ofw_tree_space_alloc(size);
				if (current_node->property[i].value = buf) {
					/*
					 * Copy property value to memory node.
					 */
					(void) ofw_get_property(current, name, buf, size);
				}
			} else {
				current_node->property[i].value = NULL;
			}
		}

		current_node->properties = i;	/* Just in case we ran out of memory. */

		/*
		 * Iteratively process the next peer node.
		 * Note that recursion is a bad idea here.
		 * Due to the topology of the OpenFirmware device tree,
		 * the nesting of peer nodes could be to wide and the
		 * risk of overflowing the stack is too real.
		 */
		peer = ofw_get_peer_node(current);
		if (peer != 0 && peer != -1) {
			ofw_tree_node_t *peer_node;
		
			peer_node = ofw_tree_node_alloc();
			if (peer_node) { 
				current_node->peer = peer_node;
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
 * @return NULL on failure or pointer to the root node.
 */
ofw_tree_node_t *ofw_tree_build(void)
{
	ofw_tree_node_t *root;
	
	root = ofw_tree_node_alloc();
	if (root)
		ofw_tree_node_process(root, NULL, ofw_root);
	
	return root;
}
