/*
 * Copyright (c) 2014 Jiri Svoboda
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

/** @addtogroup nic
 * @{
 */
/** @file NIC configuration utility.
 *
 */

#include <errno.h>
#include <loc.h>
#include <nic_iface.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <sys/types.h>

#define NAME  "nic"

typedef struct {
	nic_device_info_t device_info;
	nic_address_t address;
	nic_cable_state_t link_state;
	nic_channel_mode_t duplex;
	int speed;
} nic_info_t;

static void print_syntax(void)
{
	printf("syntax:\n");
	printf("\t" NAME "\n");
}

static int nic_get_info(service_id_t svc_id, char *svc_name,
    nic_info_t *info)
{
	async_sess_t *sess;
	nic_role_t role;
	int rc;

	sess = loc_service_connect(EXCHANGE_SERIALIZE, svc_id, 0);
	if (sess == NULL) {
		printf("Error connecting '%s'.\n", svc_name);
		rc = EIO;
		goto error;
	}

	rc = nic_get_address(sess, &info->address);
	if (rc != EOK) {
		printf("Error getting NIC address.\n");
		rc = EIO;
		goto error;
	}

	rc = nic_get_device_info(sess, &info->device_info);
	if (rc != EOK) {
		printf("Error getting NIC device info.\n");
		rc = EIO;
		goto error;
	}

	rc = nic_get_cable_state(sess, &info->link_state);
	if (rc != EOK) {
		printf("Error getting link state.\n");
		rc = EIO;
		goto error;
	}

	rc = nic_get_operation_mode(sess, &info->speed, &info->duplex, &role);
	if (rc != EOK) {
		printf("Error getting NIC speed and duplex mode.\n");
		rc = EIO;
		goto error;
	}


	return EOK;
error:
	return rc;
}

static const char *nic_link_state_str(nic_cable_state_t link_state)
{
	switch (link_state) {
	case NIC_CS_UNKNOWN: return "unknown";
	case NIC_CS_PLUGGED: return "up";
	case NIC_CS_UNPLUGGED: return "down";
	default: assert(false); return NULL;
	}
}

static const char *nic_duplex_mode_str(nic_channel_mode_t mode)
{
	switch (mode) {
	case NIC_CM_FULL_DUPLEX: return "full-duplex";
	case NIC_CM_HALF_DUPLEX: return "half-duplex";
	default: assert(false); return NULL;
	}
}

static char *nic_addr_format(nic_address_t *a)
{
	int rc;
	char *s;

	rc = asprintf(&s, "%02x:%02x:%02x:%02x:%02x:%02x",
	    a->address[0], a->address[1], a->address[2],
	    a->address[3], a->address[4], a->address[5]);
	if (rc < 0)
		return NULL;

	return s;
}

static int nic_list(void)
{
	category_id_t nic_cat;
	service_id_t *nics = NULL;
	nic_info_t nic_info;
	size_t count, i;
	char *svc_name;
	char *addr_str;
	int rc;

	rc = loc_category_get_id("nic", &nic_cat, 0);
	if (rc != EOK) {
		printf("Error resolving category 'nic'.\n");
		goto error;
	}

	rc = loc_category_get_svcs(nic_cat, &nics, &count);
	if (rc != EOK) {
		printf("Error getting list of NICs.\n");
		goto error;
	}

	printf("[Address] [Service Name]\n");
	for (i = 0; i < count; i++) {
		rc = loc_service_get_name(nics[i], &svc_name);
		if (rc != EOK) {
			printf("Error getting service name.\n");
			goto error;
		}

		rc = nic_get_info(nics[i], svc_name, &nic_info);
		if (rc != EOK)
			goto error;

		addr_str = nic_addr_format(&nic_info.address);
		if (addr_str == NULL) {
			printf("Out of memory.\n");
			rc = ENOMEM;
			goto error;
		}

		printf("%s %s\n", addr_str, svc_name);
		printf("\tVendor name: %s\n",
		    nic_info.device_info.vendor_name);
		printf("\tModel name: %s\n",
		    nic_info.device_info.model_name);
		printf("\tLink state: %s\n",
		    nic_link_state_str(nic_info.link_state));

		if (nic_info.link_state == NIC_CS_PLUGGED) {
			printf("\tSpeed: %dMbps %s\n", nic_info.speed,
			    nic_duplex_mode_str(nic_info.duplex));
		}

		free(svc_name);
		free(addr_str);
	}

	return EOK;
error:
	free(nics);
	return rc;
}

int main(int argc, char *argv[])
{
	int rc;

	if (argc == 1) {
		rc = nic_list();
		if (rc != EOK)
			return 1;
	} else {
		printf(NAME ": Invalid argument.\n");
		print_syntax();
		return 1;
	}

	return 0;
}

/** @}
 */
