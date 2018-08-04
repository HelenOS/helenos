/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @file Ancestry resolver.
 *
 * A chicken-and-egg problem is that in order to match identifiers to CSI
 * definitions we need to know CSI ancestry. To know CSI ancestry we need
 * to match identifiers to CSI definitions. Thus both must be done at the
 * same time. Once we know the ancestry of some CSI, we are able to resolve
 * symbols referenced within the scope of that CSI (but not in nested scopes).
 *
 * Here lies probably the most complicated (although not so complicated)
 * algorithm. To process node N we must first process outer(N). This allows
 * us to find all base(N) nodes and process them.
 *
 * To ensure all nodes get processed correctly, we use a two-layer walk.
 * In the lower layer (ancr_csi_process) we follow the dependencies.
 * ancr_csi_process(N) ensures N (and possibly other nodes) get resolved.
 *
 * In the second layer we simply do a DFS of the CSI tree, calling
 * ancr_csi_process() on each node. This ensures that eventually all
 * nodes get processed.
 */

#include <stdlib.h>
#include <assert.h>
#include "builtin.h"
#include "cspan.h"
#include "list.h"
#include "mytypes.h"
#include "stree.h"
#include "strtab.h"
#include "symbol.h"

#include "ancr.h"

static void ancr_csi_dfs(stree_program_t *prog, stree_csi_t *csi);
static void ancr_csi_process(stree_program_t *prog, stree_csi_t *node);
static stree_csi_t *ancr_csi_get_pred(stree_program_t *prog, stree_csi_t *csi,
    stree_texpr_t *pred_ref);
static void ancr_csi_print_cycle(stree_program_t *prog, stree_csi_t *node);

/** Process ancestry of all CSIs in a module.
 *
 * Note that currently we expect there to be exactly one module in the
 * whole program.
 *
 * @param prog		Program being processed.
 * @param module	Module to process.
 */
void ancr_module_process(stree_program_t *prog, stree_module_t *module)
{
	list_node_t *node;
	stree_modm_t *modm;

	(void) module;
	node = list_first(&prog->module->members);

	while (node != NULL) {
		modm = list_node_data(node, stree_modm_t *);

		switch (modm->mc) {
		case mc_csi:
			ancr_csi_dfs(prog, modm->u.csi);
			break;
		case mc_enum:
			break;
		}

		node = list_next(&prog->module->members, node);
	}
}

/** Walk CSI node tree depth-first.
 *
 * This is the outer depth-first walk whose purpose is to eventually
 * process all CSI nodes by calling ancr_csi_process() on them.
 * (Which causes that and possibly some other nodes to be processed).
 *
 * @param prog		Program being processed.
 * @param csi		CSI node to visit.
 */
static void ancr_csi_dfs(stree_program_t *prog, stree_csi_t *csi)
{
	list_node_t *node;
	stree_csimbr_t *csimbr;

	/* Process this node first. */
	ancr_csi_process(prog, csi);

	/* Now visit all children. */
	node = list_first(&csi->members);
	while (node != NULL) {
		csimbr = list_node_data(node, stree_csimbr_t *);
		if (csimbr->cc == csimbr_csi)
			ancr_csi_dfs(prog, csimbr->u.csi);

		node = list_next(&csi->members, node);
	}
}

/** Process csi node.
 *
 * Fist processes the pre-required nodes (outer CSI and base CSIs),
 * then processes @a node. This is the core 'outward-and-baseward' walk.
 *
 * @param prog		Program being processed.
 * @param csi		CSI node to process.
 */
static void ancr_csi_process(stree_program_t *prog, stree_csi_t *csi)
{
	stree_csi_t *base_csi, *outer_csi;
	stree_csi_t *gf_class;

	list_node_t *pred_n;
	stree_texpr_t *pred;
	stree_csi_t *pred_csi;

	if (csi->ancr_state == ws_visited) {
		/* Node already processed */
		return;
	}

	if (csi->ancr_state == ws_active) {
		/* Error, closed reference loop. */
		printf("Error: Circular class, struct or interface chain: ");
		ancr_csi_print_cycle(prog, csi);
		printf(".\n");
		exit(1);
	}

	csi->ancr_state = ws_active;

	outer_csi = csi_to_symbol(csi)->outer_csi;
	gf_class = builtin_get_gf_class(prog->builtin);

	if (csi != gf_class) {
		/* Implicit inheritance from grandfather class. */
		base_csi = gf_class;
	} else {
		/* Grandfather class has no base class. */
		base_csi = NULL;
	}

	/* Process outer CSI */
	if (outer_csi != NULL)
		ancr_csi_process(prog, outer_csi);

	/*
	 * Process inheritance list.
	 */
	pred_n = list_first(&csi->inherit);

	/* For a class node, the first entry can be a class. */
	if (csi->cc == csi_class && pred_n != NULL) {
		pred = list_node_data(pred_n, stree_texpr_t *);
		pred_csi = ancr_csi_get_pred(prog, csi, pred);
		assert(pred_csi != NULL);

		if (pred_csi->cc == csi_class) {
			/* Process base class */
			base_csi = pred_csi;
			ancr_csi_process(prog, pred_csi);

			pred_n = list_next(&csi->inherit, pred_n);
		}
	}

	/* Following entires can only be interfaces. */
	while (pred_n != NULL) {
		pred = list_node_data(pred_n, stree_texpr_t *);
		pred_csi = ancr_csi_get_pred(prog, csi, pred);
		assert(pred_csi != NULL);

		/* Process implemented or accumulated interface. */
		ancr_csi_process(prog, pred_csi);

		switch (pred_csi->cc) {
		case csi_class:
			switch (csi->cc) {
			case csi_class:
				cspan_print(csi->name->cspan);
				printf(" Error: Only the first predecessor "
				    "can be a class. ('");
				symbol_print_fqn(csi_to_symbol(csi));
				printf("' deriving from '");
				symbol_print_fqn(csi_to_symbol(pred_csi));
				printf("').\n");
				exit(1);
				break;
			case csi_struct:
				assert(b_false); /* XXX */
				break;
			case csi_interface:
				cspan_print(csi->name->cspan);
				printf(" Error: Interface predecessor must be "
				    "an interface ('");
				symbol_print_fqn(csi_to_symbol(csi));
				printf("' deriving from '");
				symbol_print_fqn(csi_to_symbol(pred_csi));
				printf("').\n");
				exit(1);
				break;
			}
			break;
		case csi_struct:
			assert(b_false); /* XXX */
			break;
		case csi_interface:
			break;
		}

		pred_n = list_next(&csi->inherit, pred_n);
	}

	/* Store base CSI and update node state. */
	csi->ancr_state = ws_visited;
	csi->base_csi = base_csi;
}

/** Resolve CSI predecessor reference.
 *
 * Returns the CSI predecessor referenced by @a pred_ref.
 * If the referenced CSI does not exist, an error is generated.
 *
 * @param prog		Program being processed.
 * @param csi		CSI node to process.
 * @param pred_ref	Type expression referencing the predecessor.
 * @return		Predecessor CSI.
 */
static stree_csi_t *ancr_csi_get_pred(stree_program_t *prog, stree_csi_t *csi,
    stree_texpr_t *pred_ref)
{
	stree_csi_t *outer_csi;
	stree_symbol_t *pred_sym;
	stree_csi_t *pred_csi;

	outer_csi = csi_to_symbol(csi)->outer_csi;
	pred_sym = symbol_xlookup_in_csi(prog, outer_csi, pred_ref);
	pred_csi = symbol_to_csi(pred_sym);
	assert(pred_csi != NULL); /* XXX */

	return pred_csi;
}

/** Print loop in CSI ancestry.
 *
 * We have detected a loop in CSI ancestry. Traverse it (by following the
 * nodes in ws_active state and print it.
 *
 * @param prog		Program.
 * @param node		CSI node participating in an ancestry cycle.
 */
static void ancr_csi_print_cycle(stree_program_t *prog, stree_csi_t *node)
{
	stree_csi_t *n;
	stree_symbol_t *pred_sym, *node_sym;
	stree_csi_t *pred_csi, *outer_csi;
	stree_texpr_t *pred;
	list_node_t *pred_n;

	n = node;
	do {
		node_sym = csi_to_symbol(node);
		symbol_print_fqn(node_sym);
		printf(", ");

		outer_csi = node_sym->outer_csi;

		if (outer_csi != NULL && outer_csi->ancr_state == ws_active) {
			node = outer_csi;
		} else {
			node = NULL;

			pred_n = list_first(&node->inherit);
			while (pred_n != NULL) {
				pred = list_node_data(pred_n, stree_texpr_t *);
				pred_sym = symbol_xlookup_in_csi(prog,
				    outer_csi, pred);
				pred_csi = symbol_to_csi(pred_sym);
				assert(pred_csi != NULL);

				if (pred_csi->ancr_state == ws_active) {
					node = pred_csi;
					break;
				}
			}

			assert(node != NULL);
		}
	} while (n != node);

	node_sym = csi_to_symbol(node);
	symbol_print_fqn(node_sym);
}
