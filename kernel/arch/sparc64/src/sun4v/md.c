/*
 * Copyright (c) 2009 Pavel Rimsky
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

/** @addtogroup sparc64
 * @{
 */
/** @file
 */

#include <debug.h>
#include <panic.h>
#include <halt.h>
#include <log.h>
#include <str.h>
#include <arch/sun4v/md.h>
#include <arch/sun4v/hypercall.h>
#include <arch/mm/page.h>

/* maximum MD size estimate (in bytes) */
#define MD_MAX_SIZE	(64 * 1024)

/** element types (element tag values) */
#define LIST_END	0x0	/**< End of element list */
#define NODE		0x4e	/**< Start of node definition */
#define NODE_END	0x45	/**< End of node definition */
#define NOOP		0x20	/**< NOOP list element - to be ignored */
#define PROP_ARC	0x61	/**< Node property arc'ing to another node */
#define PROP_VAL	0x76	/**< Node property with an integer value */
#define PROP_STR	0x73	/**< Node property with a string value */
#define PROP_DATA	0x64	/**< Node property with a block of data */


/** machine description header */
typedef struct {
	uint32_t transport_version;	/**< Transport version number */
	uint32_t node_blk_sz;		/**< Size in bytes of node block */
	uint32_t name_blk_sz;		/**< Size in bytes of name block */
	uint32_t data_blk_sz;		/**< Size in bytes of data block */
} __attribute__ ((packed)) md_header_t;

/** machine description element (in the node block) */
typedef struct {
	uint8_t tag;			/**< Type of element */
	uint8_t name_len;		/**< Length in bytes of element name */
	uint16_t _reserved_field;	/**< reserved field (zeros) */
	uint32_t name_offset;		/**< Location offset of name associated
					     with this element relative to
					     start of name block */
	union {
		/** for elements of type “PROP_STR” and of type “PROP_DATA” */
		struct {
			/** Length in bytes of data in data block */
			uint32_t data_len;

			/**
			 * Location offset of data associated with this
			 * element relative to start of data block
			 */
			uint32_t data_offset;
		} y;

		/**
		 *  64 bit value for elements of tag type “NODE”, “PROP_VAL”
		 *  or “PROP_ARC”
		 */
		uint64_t val;
	} d;
} __attribute__ ((packed)) md_element_t;

/** index of the element within the node block */
typedef unsigned int element_idx_t;

/** buffer to which the machine description will be saved */
static uint8_t mach_desc[MD_MAX_SIZE]
	 __attribute__ ((aligned (16)));


/** returns pointer to the element at the given index */
static md_element_t *get_element(element_idx_t idx)
{
	return (md_element_t *) (mach_desc +
	    sizeof(md_header_t) + idx * sizeof(md_element_t));
}

/** returns the name of the element represented by the index */
static const char *get_element_name(element_idx_t idx)
{
	md_header_t *md_header = (md_header_t *) mach_desc;
	uintptr_t name_offset = get_element(idx)->name_offset;
	return (char *) mach_desc + sizeof(md_header_t) +
	    md_header->node_blk_sz + name_offset;
}

/** finds the name of the node represented by "node" */
const char *md_get_node_name(md_node_t node)
{
	return get_element_name(node);
}

/**
 * Returns the value of the integer property of the given node.
 *
 * @param
 */
bool md_get_integer_property(md_node_t node, const char *key,
	uint64_t *result)
{
	element_idx_t idx = node;

	while (get_element(idx)->tag != NODE_END) {
		idx++;
		md_element_t *element = get_element(idx);
		if (element->tag == PROP_VAL &&
		    str_cmp(key, get_element_name(idx)) == 0) {
			*result = element->d.val;
			return true;
		}
	}

	return false;
}

/**
 * Returns the value of the string property of the given node.
 *
 * @param
 */
bool md_get_string_property(md_node_t node, const char *key,
	const char **result)
{
	md_header_t *md_header = (md_header_t *) mach_desc;
	element_idx_t idx = node;

	while (get_element(idx)->tag != NODE_END) {
		idx++;
		md_element_t *element = get_element(idx);
		if (element->tag == PROP_DATA &&
		    str_cmp(key, get_element_name(idx)) == 0) {
			*result = (char *) mach_desc + sizeof(md_header_t) +
			    md_header->node_blk_sz + md_header->name_blk_sz +
			    element->d.y.data_offset;
			return true;
		}
	}

	return false;
}

/**
 * Moves the child oterator to the next child (following sibling of the node
 * the oterator currently points to).
 *
 * @param it	pointer to the iterator to be moved
 */
bool md_next_child(md_child_iter_t *it)
{
	element_idx_t backup = *it;

	while (get_element(*it)->tag != NODE_END) {
		(*it)++;
		md_element_t *element = get_element(*it);
		if (element->tag == PROP_ARC &&
		    str_cmp("fwd", get_element_name(*it)) == 0) {
			return true;
		}
	}

	*it = backup;
	return false;
}

/**
 * Returns the node the iterator point to.
 */
md_node_t md_get_child_node(md_child_iter_t it)
{
	return get_element(it)->d.val;
}

/**
 * Helper function used to split a string to a part before the first
 * slash sign and a part after the slash sign.
 *
 * @param str	pointer to the string to be split; when the function finishes,
 * 		it will	contain only the part following the first slash sign of
 * 		the original string
 * @param head	pointer to the string which will be set to the part before the
 * 		first slash sign
 */
static bool str_parse_head(char **str, char **head)
{
	*head = *str;

	char *cur = *str;
	while (*cur != '\0') {
		if (*cur == '/') {
			*cur = '\0';
			*str = cur + 1;
			return true;
		}
		cur++;
	}

	return false;
}

/**
 * Returns the descendant of the given node. The descendant is identified
 * by a path where the node names are separated by a slash.
 *
 * Ex.: Let there be a node N with path "a/b/c/x/y/z" and let P represent the
 * node with path "a/b/c". Then md_get_child(P, "x/y/z") will return N.
 */
md_node_t md_get_child(md_node_t node, char *name)
{
	bool more;

	do {
		char *head;
		more = str_parse_head(&name, &head);

		while (md_next_child(&node)) {
			element_idx_t child = md_get_child_node(node);
			if (str_cmp(head, get_element_name(child)) == 0) {
				node = child;
				break;
			}
		}

	} while (more);

	return node;
}

/** returns the root node of MD */
md_node_t md_get_root(void)
{
	return 0;
}

/**
 * Returns the child iterator - a token to be passed to functions iterating
 * through all the children of a node.
 *
 * @param node	a node whose children the iterator will be used
 * 		to iterate through
 */
md_child_iter_t md_get_child_iterator(md_node_t node)
{
	return node;
}

/**
 * Moves "node" to the node following "node" in the list of all the existing
 * nodes of the MD whose name is "name".
 */
bool md_next_node(md_node_t *node, const char *name)
{
	md_element_t *element;
	(*node)++;

	do {
		element = get_element(*node);

		if (element->tag == NODE &&
		    str_cmp(name, get_element_name(*node)) == 0) {
			return true;
		}

		(*node)++;
	} while (element->tag != LIST_END);

	return false;
}

/**
 * Retrieves the machine description from the hypervisor and saves it to
 * a kernel buffer.
 */
void md_init(void)
{
	uint64_t retval = __hypercall_fast2(MACH_DESC, KA2PA(mach_desc),
	    MD_MAX_SIZE);

	retval = retval;
	if (retval != HV_EOK) {
		log(LF_ARCH, LVL_ERROR, "Could not retrieve machine "
		    "description, error=%" PRIu64 ".", retval);
	}
}

/** @}
 */
