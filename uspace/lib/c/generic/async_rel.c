/*
 * Copyright (c) 2010 Jakub Jermar
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

/**
 * This file implements simple relation support for the async framework.
 *
 * By the term "relation", we mean a logical data path between a client and a
 * server over which the client can send multiple, potentially blocking,
 * requests to the server.
 *
 * Clients and servers are naturally connected using IPC phones, thus an IPC
 * phone represents a connection between a client and a server. In one
 * connection, there can be many relations.
 *
 * Relations are useful in situations in which there is only one IPC connection
 * between the client and the server, but the client wants to be able to make
 * multiple parallel requests. Using only a single phone and without any other
 * provisions, all requests would have to be serialized. On the other hand, the
 * client can make as many parallel requests as there are active relations.
 *
 * There are several possible implementations of relations. This implementation
 * uses additional phones to represent relations. Using phones both for the
 * primary connection and also for its relations has several advantages:
 *
 * - to make a series of requests over a relation, the client can continue to
 *   use the existing async framework APIs
 * - the server supports relations by the virtue of spawning a new connection
 *   fibril, just as it does for every new connection even without relations
 * - the implementation is pretty straightforward; a very naive implementation
 *   would be to make each request using a fresh phone (that is what we have
 *   done in the past); a slightly better approach would be to cache connected
 *   phones so that they can be reused by a later relation within the same
 *   connection (that is what this implementation does)
 *
 * The main disadvantages of using phones to represent relations are:
 *
 * - if there are too many relations (even cached ones), the task may hit its
 *   limit on the maximum number of connected phones, which could prevent the
 *   task from making new IPC connections to other tasks
 * - if there are too many IPC connections already, it may be impossible to
 *   create a relation by connecting a new phone thanks to the task's limit on
 *   the maximum number of connected phones
 *
 * These problems can be helped by increasing the limit on the maximum number of
 * connected phones to some reasonable value and by limiting the number of
 * phones cached to some fraction of this limit.
 *
 * The cache itself has a mechanism to close some number of unused phones if a
 * new phone cannot be connected, but the outter world currently does not have a
 * way to ask the phone cache to shrink.
 *
 * To minimize the confusion stemming from the fact that we use phones for two
 * things (the primary IPC connection and also each relation), this file makes
 * the distinction by using the term 'key phone' for the former and 'relation
 * phone' for the latter. Under the hood, all phones remain equal, of course.
 *
 * There is a small inefficiency in that the cache repeatedly allocates and
 * deallocated the rel_node_t structures when in fact it could keep the
 * allocated structures around and reuse them later. But such a solution would
 * be effectively implementing a poor man's slab allocator while it would be
 * better to have the slab allocator ported to uspace so that everyone could
 * benefit from it.
 */

#include <async_rel.h>
#include <ipc/ipc.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <adt/hash_table.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>

#define KEY_NODE_HASH_BUCKETS	16

typedef struct {
	link_t link;		/**< Key node hash table link. */
	int key_phone;		/**< The phone serving as a key. */
	link_t rel_head;	/**< List of open relation phones. */
} key_node_t;

typedef struct {
	link_t rel_link;	/**< Link for the list of relation phones. */
	link_t global_link;	/**< Link for the global list of phones. */
	int rel_phone;		/**< Connected relation phone. */
} rel_node_t;

/**
 * Mutex protecting the global_rel_head list and the key_node_hash hash table.
 */
static fibril_mutex_t async_rel_mutex;

/**
 * List of all currently unused relation phones.
 */
static LIST_INITIALIZE(global_rel_head);

/**
 * Hash table containing lists of available relation phones for all key
 * phones.
 */
static hash_table_t key_node_hash;

static hash_index_t kn_hash(unsigned long *key)
{
	return *key % KEY_NODE_HASH_BUCKETS;
}

static int kn_compare(unsigned long *key, hash_count_t keys, link_t *item)
{
	key_node_t *knp = hash_table_get_instance(item, key_node_t, link);

	return *key == (unsigned long) knp->key_phone;
}

static void kn_remove_callback(link_t *item)
{
}

static hash_table_operations_t key_node_hash_ops = {
	.hash = kn_hash,
	.compare = kn_compare,
	.remove_callback = kn_remove_callback
};

/** Initialize the async_rel subsystem.
 *
 * Needs to be called prior to any other interface in this file.
 */
int async_rel_init(void)
{
	fibril_mutex_initialize(&async_rel_mutex);
	list_initialize(&global_rel_head);
	return hash_table_create(&key_node_hash, KEY_NODE_HASH_BUCKETS, 1,
	    &key_node_hash_ops);
}

static void key_node_initialize(key_node_t *knp)
{
	link_initialize(&knp->link);
	knp->key_phone = -1;
	list_initialize(&knp->rel_head);
}

static void rel_node_initialize(rel_node_t *rnp)
{
	link_initialize(&rnp->rel_link);
	link_initialize(&rnp->global_link);
	rnp->rel_phone = -1;
}

/** Create a new relation for a connection represented by a key phone.
 *
 * @param key_phone	Phone representing the connection.
 * @return		Phone representing the new relation or a negative error
 *			code.
 */
int async_relation_create(int key_phone)
{
	unsigned long key = (unsigned long) key_phone;
	link_t *lnk;
	key_node_t *knp;
	rel_node_t *rnp;
	int rel_phone;

	fibril_mutex_lock(&async_rel_mutex);
	lnk = hash_table_find(&key_node_hash, &key);
	if (!lnk) {
		/*
		 * The key node was not found in the hash table. Try to allocate
		 * and hash in a new one.
		 */
		knp = (key_node_t *) malloc(sizeof(key_node_t));
		if (!knp) {
			/*
			 * As a possible improvement, we could make a one-time
			 * attempt to create a phone without trying to add the
			 * key node into the hash.
			 */
			fibril_mutex_unlock(&async_rel_mutex);
			return ENOMEM;
		}
		key_node_initialize(knp);
		knp->key_phone = key_phone;
		hash_table_insert(&key_node_hash, &key, &knp->link);
	} else {
		/*
		 * Found the key node.
		 */
		knp = hash_table_get_instance(lnk, key_node_t, link);
	}

	if (!list_empty(&knp->rel_head)) {
		/*
		 * There are available relation phones for the key phone.
		 */
		rnp = list_get_instance(knp->rel_head.next, rel_node_t,
		    rel_link);
		list_remove(&rnp->rel_link);
		list_remove(&rnp->global_link);
		
		rel_phone = rnp->rel_phone;
		free(rnp);
	} else {
		/*
		 * There are no available relation phones for the key phone.
		 * Make a one-time attempt to connect a new relation phone.
		 */
retry:
		rel_phone = async_connect_me_to(key_phone, 0, 0, 0);
		if (rel_phone >= 0) {
			/* success, do nothing */
		} else if (!list_empty(&global_rel_head)) {
			/*
			 * We did not manage to connect a new phone. But we can
			 * try to hangup some currently unused phones and try
			 * again.
			 */
			rnp = list_get_instance(global_rel_head.next,
			    rel_node_t, global_link);
			list_remove(&rnp->global_link);
			list_remove(&rnp->rel_link);
			rel_phone = rnp->rel_phone;
			free(rnp);
			ipc_hangup(rel_phone);
			goto retry;
		} else {
			/*
			 * This is unfortunate. We failed both to find a cached
			 * phone or to create a new one even after cleaning up
			 * the cache. This is most likely due to too many key
			 * phones being kept connected.
			 */
			rel_phone = ELIMIT;
		}
	}

	fibril_mutex_unlock(&async_rel_mutex);
	return rel_phone;
}

/** Destroy a relation.
 *
 * @param key_phone	Phone representing the connection.
 * @param rel_phone	Phone representing the relation within the connection.
 */
void async_relation_destroy(int key_phone, int rel_phone)
{
	unsigned long key = (unsigned long) key_phone;
	key_node_t *knp;
	rel_node_t *rnp;
	link_t *lnk;

	fibril_mutex_lock(&async_rel_mutex);
	lnk = hash_table_find(&key_node_hash, &key);
	assert(lnk);
	knp = hash_table_get_instance(lnk, key_node_t, link);
	rnp = (rel_node_t *) malloc(sizeof(rel_node_t));
	if (!rnp) {
		/*
		 * Being unable to remember the connected relation phone here
		 * means that we simply hangup.
		 */
		fibril_mutex_unlock(&async_rel_mutex);
		ipc_hangup(rel_phone);
		return;
	}
	rel_node_initialize(rnp);
	rnp->rel_phone = rel_phone;
	list_append(&rnp->rel_link, &knp->rel_head);
	list_append(&rnp->global_link, &global_rel_head);
	fibril_mutex_unlock(&async_rel_mutex);
}

/** @}
 */
