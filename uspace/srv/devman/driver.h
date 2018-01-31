/*
 * Copyright (c) 2010 Lenka Trochtova
 * Copyright (c) 2013 Jiri Svoboda
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

#ifndef DRIVER_H_
#define DRIVER_H_

#include <stdbool.h>
#include "devman.h"

extern void init_driver_list(driver_list_t *);
extern driver_t *create_driver(void);
extern bool get_driver_info(const char *, const char *, driver_t *);
extern int lookup_available_drivers(driver_list_t *, const char *);

extern driver_t *find_best_match_driver(driver_list_t *, dev_node_t *);
extern bool assign_driver(dev_node_t *, driver_list_t *, dev_tree_t *);

extern void add_driver(driver_list_t *, driver_t *);
extern void attach_driver(dev_tree_t *, dev_node_t *, driver_t *);
extern void detach_driver(dev_tree_t *, dev_node_t *);
extern bool start_driver(driver_t *);
extern errno_t stop_driver(driver_t *);
extern void add_device(driver_t *, dev_node_t *, dev_tree_t *);
extern errno_t driver_dev_remove(dev_tree_t *, dev_node_t *);
extern errno_t driver_dev_gone(dev_tree_t *, dev_node_t *);
extern errno_t driver_fun_online(dev_tree_t *, fun_node_t *);
extern errno_t driver_fun_offline(dev_tree_t *, fun_node_t *);

extern driver_t *driver_find(driver_list_t *, devman_handle_t);
extern driver_t *driver_find_by_name(driver_list_t *, const char *);
extern void initialize_running_driver(driver_t *, dev_tree_t *);

extern void init_driver(driver_t *);
extern void clean_driver(driver_t *);
extern void delete_driver(driver_t *);
extern errno_t driver_get_list(driver_list_t *, devman_handle_t *, size_t, size_t *);
extern errno_t driver_get_devices(driver_t *, devman_handle_t *, size_t, size_t *);

#endif

/** @}
 */
