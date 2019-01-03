/*
 * Copyright (c) 2019 Jiri Svoboda
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

/** @addtogroup pci
 * @{
 */

/**
 * @file
 * @brief Tool for listing PCI devices.
 */

#include <devman.h>
#include <errno.h>
#include <io/table.h>
#include <loc.h>
#include <pci.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>

#define NAME	"mkext4"

#define MAX_NAME_LENGTH 1024

static void syntax_print(void);
static errno_t pci_list(void);
static errno_t pci_list_bridge(const char *);
static errno_t pci_list_bridge_id(service_id_t);

int main(int argc, char **argv)
{
	errno_t rc;
	const char *bridge = NULL;

	--argc;
	++argv;

	while (argc > 0 && argv[0][0] == '-') {
		if (str_cmp(argv[0], "--bridge") == 0) {
			--argc;
			++argv;
			if (argc < 1) {
				printf("Option argument missing.\n");
				return 1;
			}

			bridge = argv[0];
			--argc;
			++argv;
		} else {
			syntax_print();
			return 1;
		}
	}

	if (argc != 0) {
		syntax_print();
		return 1;
	}

	if (bridge != NULL)
		rc = pci_list_bridge(bridge);
	else
		rc = pci_list();

	if (rc != EOK)
		return 1;

	return 0;
}

static void syntax_print(void)
{
	printf("syntax: pci [<options>]\n");
	printf("options:\n"
	    "\t--bridge <svc-name> Only devices under host bridge <svc-name>\n");
}

/** List PCI devices. */
static errno_t pci_list(void)
{
	errno_t rc;
	category_id_t pci_cat_id;
	service_id_t *svc_ids = NULL;
	size_t svc_cnt;
	size_t i;

	rc = loc_category_get_id("pci", &pci_cat_id, 0);
	if (rc != EOK) {
		printf("Error getting 'pci' category ID.\n");
		goto error;
	}

	rc = loc_category_get_svcs(pci_cat_id, &svc_ids, &svc_cnt);
	if (rc != EOK) {
		printf("Error getting list of PCI services.\n");
		goto error;
	}

	for (i = 0; i < svc_cnt; i++) {
		if (i > 0)
			putchar('\n');

		rc = pci_list_bridge_id(svc_ids[i]);
		if (rc != EOK)
			goto error;
	}

	free(svc_ids);
	return EOK;
error:
	if (svc_ids != NULL)
		free(svc_ids);
	return rc;
}

/** List PCI devices under a host bridge specified by name.
 *
 * @param svc_name PCI service name
 * @return EOK on success or an error code
 */
static errno_t pci_list_bridge(const char *svc_name)
{
	errno_t rc;
	service_id_t svc_id;

	rc = loc_service_get_id(svc_name, &svc_id, 0);
	if (rc != EOK) {
		printf("Error looking up host bridge '%s'.\n", svc_name);
		return rc;
	}

	return pci_list_bridge_id(svc_id);
}

/** List PCI devices under a host bridge specified by ID.
 *
 * @param svc_id PCI service ID
 * @return EOK on success or an error code
 */
static errno_t pci_list_bridge_id(service_id_t svc_id)
{
	errno_t rc;
	devman_handle_t *dev_ids = NULL;
	size_t dev_cnt;
	pci_dev_info_t dev_info;
	size_t i;
	pci_t *pci = NULL;
	char *svc_name = NULL;
	char *drv_name = NULL;
	table_t *table = NULL;

	drv_name = malloc(MAX_NAME_LENGTH);
	if (drv_name == NULL) {
		printf("Out of memory.\n");
		rc = ENOMEM;
		goto error;
	}

	rc = table_create(&table);
	if (rc != EOK) {
		printf("Out of memory.\n");
		rc = ENOMEM;
		goto error;
	}

	table_header_row(table);
	table_printf(table, "Address\t" "Type\t" "Driver\n");

	rc = loc_service_get_name(svc_id, &svc_name);
	if (rc != EOK) {
		printf("Error getting service name.\n");
		goto error;
	}

	rc = pci_open(svc_id, &pci);
	if (rc != EOK) {
		printf("Error opening PCI service '%s'.\n", svc_name);
		goto error;
	}

	rc = pci_get_devices(pci, &dev_ids, &dev_cnt);
	if (rc != EOK) {
		printf("Error getting PCI device list.\n");
		goto error;
	}

	for (i = 0; i < dev_cnt; i++) {
		rc = pci_dev_get_info(pci, dev_ids[i], &dev_info);
		if (rc != EOK) {
			printf("Error getting PCI device info.\n");
			goto error;
		}

		rc = devman_fun_get_driver_name(dev_info.dev_handle,
		    drv_name, MAX_NAME_LENGTH);
		if (rc != EOK && rc != EINVAL) {
			printf("Error getting driver name.\n");
			goto error;
		}

		if (rc == EINVAL)
			drv_name[0] = '\0';

		table_printf(table, "%02x.%02x.%x\t" "%04x:%04x\t"
		    "%s\n", dev_info.bus_num, dev_info.dev_num,
		    dev_info.fn_num, dev_info.vendor_id,
		    dev_info.device_id, drv_name);
	}

	printf("Device listing for host bridge %s:\n\n", svc_name);
	rc = table_print_out(table, stdout);
	if (rc != EOK) {
		printf("Error printing table.\n");
		goto error;
	}

	table_destroy(table);
	table = NULL;
	free(dev_ids);
	dev_ids = NULL;
	pci_close(pci);
	pci = NULL;
	free(svc_name);
	svc_name = NULL;

	free(drv_name);
	return EOK;
error:
	if (drv_name != NULL)
		free(drv_name);
	if (table != NULL)
		table_destroy(table);
	if (pci != NULL)
		pci_close(pci);
	if (svc_name != NULL)
		free(svc_name);
	if (dev_ids != NULL)
		free(dev_ids);
	return rc;
}

/**
 * @}
 */
