/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup disp
 * @{
 */
/** @file Display configuration utility.
 *
 * Configures the display service.
 */

#include <errno.h>
#include <dispcfg.h>
#include <io/table.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <str.h>
#include <str_error.h>

#define NAME  "disp"

static void print_syntax(void)
{
	printf("%s: Display configuration utility.\n", NAME);
	printf("Syntax:\n");
	printf("  %s list-seat\n", NAME);
	printf("  %s create-seat <name>\n", NAME);
	printf("  %s delete-seat <name>\n", NAME);
	printf("  %s assign-dev <device> <seat>\n", NAME);
	printf("  %s unassign-dev <device>\n", NAME);
	printf("  %s list-dev <seat>\n", NAME);
}

/** Find seat by name.
 *
 * @param dispcfg Display configuration
 * @param name Seat name
 * @param rseat_id Place to store seat ID
 * @return EOK on success or an error code
 */
static errno_t seat_find_by_name(dispcfg_t *dispcfg, const char *name,
    sysarg_t *rseat_id)
{
	dispcfg_seat_list_t *seat_list;
	dispcfg_seat_info_t *sinfo;

	size_t i;
	errno_t rc;

	rc = dispcfg_get_seat_list(dispcfg, &seat_list);
	if (rc != EOK) {
		printf(NAME ": Failed getting seat list.\n");
		return rc;
	}

	for (i = 0; i < seat_list->nseats; i++) {
		rc = dispcfg_get_seat_info(dispcfg, seat_list->seats[i],
		    &sinfo);
		if (rc != EOK)
			continue;

		if (str_cmp(sinfo->name, name) == 0) {
			*rseat_id = seat_list->seats[i];
			dispcfg_free_seat_info(sinfo);
			return EOK;
		}

		dispcfg_free_seat_info(sinfo);
	}

	return ENOENT;
}

/** Create seat subcommand.
 *
 * @param dcfg_svc Display configuration service name
 * @param argc Number of arguments
 * @param argv Arguments
 * @return EOK on success or an erro code
 */
static errno_t create_seat(const char *dcfg_svc, int argc, char *argv[])
{
	dispcfg_t *dispcfg;
	char *seat_name;
	sysarg_t seat_id;
	errno_t rc;

	if (argc < 1) {
		printf(NAME ": Missing arguments.\n");
		print_syntax();
		return EINVAL;
	}

	if (argc > 1) {
		printf(NAME ": Too many arguments.\n");
		print_syntax();
		return EINVAL;
	}

	seat_name = argv[0];

	rc = dispcfg_open(dcfg_svc, NULL, NULL, &dispcfg);
	if (rc != EOK) {
		printf(NAME ": Failed connecting to display configuration "
		    "service: %s.\n", str_error(rc));
		return rc;
	}

	rc = dispcfg_seat_create(dispcfg, seat_name, &seat_id);
	if (rc != EOK) {
		printf(NAME ": Failed creating seat '%s' (%s)\n",
		    seat_name, str_error(rc));
		dispcfg_close(dispcfg);
		return EIO;
	}

	(void)seat_id;
	dispcfg_close(dispcfg);
	return EOK;
}

/** Delete seat subcommand.
 *
 * @param dcfg_svc Display configuration service name
 * @param argc Number of arguments
 * @param argv Arguments
 * @return EOK on success or an erro code
 */
static errno_t delete_seat(const char *dcfg_svc, int argc, char *argv[])
{
	dispcfg_t *dispcfg;
	char *seat_name;
	sysarg_t seat_id;
	errno_t rc;

	if (argc < 1) {
		printf(NAME ": Missing arguments.\n");
		print_syntax();
		return EINVAL;
	}

	if (argc > 1) {
		printf(NAME ": Too many arguments.\n");
		print_syntax();
		return EINVAL;
	}

	seat_name = argv[0];

	rc = dispcfg_open(dcfg_svc, NULL, NULL, &dispcfg);
	if (rc != EOK) {
		printf(NAME ": Failed connecting to display configuration "
		    "service: %s.\n", str_error(rc));
		return rc;
	}

	rc = seat_find_by_name(dispcfg, seat_name, &seat_id);
	if (rc != EOK) {
		printf(NAME ": Seat '%s' not found.\n", seat_name);
		dispcfg_close(dispcfg);
		return ENOENT;
	}

	rc = dispcfg_seat_delete(dispcfg, seat_id);
	if (rc != EOK) {
		printf(NAME ": Failed deleting seat '%s': %s\n", seat_name,
		    str_error(rc));
		dispcfg_close(dispcfg);
		return EIO;
	}

	dispcfg_close(dispcfg);
	return EOK;
}

/** List seats subcommand.
 *
 * @param dcfg_svc Display configuration service name
 * @param argc Number of arguments
 * @param argv Arguments
 * @return EOK on success or an erro code
 */
static errno_t list_seat(const char *dcfg_svc, int argc, char *argv[])
{
	dispcfg_t *dispcfg;
	dispcfg_seat_list_t *seat_list;
	dispcfg_seat_info_t *sinfo;
	table_t *table = NULL;
	size_t i;
	errno_t rc;

	if (argc > 0) {
		printf(NAME ": Too many arguments.\n");
		print_syntax();
		return EINVAL;
	}

	rc = dispcfg_open(dcfg_svc, NULL, NULL, &dispcfg);
	if (rc != EOK) {
		printf(NAME ": Failed connecting to display configuration "
		    "service: %s.\n", str_error(rc));
		return rc;
	}

	rc = dispcfg_get_seat_list(dispcfg, &seat_list);
	if (rc != EOK) {
		printf(NAME ": Failed getting seat list.\n");
		dispcfg_close(dispcfg);
		return rc;
	}

	rc = table_create(&table);
	if (rc != EOK) {
		printf("Memory allocation failed.\n");
		dispcfg_close(dispcfg);
		goto out;
	}

	table_header_row(table);
	table_printf(table, "Seat Name\n");

	for (i = 0; i < seat_list->nseats; i++) {
		rc = dispcfg_get_seat_info(dispcfg, seat_list->seats[i],
		    &sinfo);
		if (rc != EOK) {
			printf("Failed getting properties of seat %zu.\n",
			    (size_t)seat_list->seats[i]);
			continue;
		}

		table_printf(table, "%s\n", sinfo->name);

		dispcfg_free_seat_info(sinfo);
	}

	if (seat_list->nseats != 0) {
		rc = table_print_out(table, stdout);
		if (rc != EOK) {
			printf("Error printing table.\n");
			goto out;
		}
	}

	rc = EOK;
out:
	table_destroy(table);
	dispcfg_close(dispcfg);
	return rc;
}

/** Assign device subcommand.
 *
 * @param dcfg_svc Display configuration service name
 * @param argc Number of arguments
 * @param argv Arguments
 * @return EOK on success or an erro code
 */
static errno_t dev_assign(const char *dcfg_svc, int argc, char *argv[])
{
	dispcfg_t *dispcfg;
	char *dev_name;
	service_id_t svc_id;
	char *seat_name;
	sysarg_t seat_id;
	errno_t rc;

	if (argc < 2) {
		printf(NAME ": Missing arguments.\n");
		print_syntax();
		return EINVAL;
	}

	if (argc > 2) {
		printf(NAME ": Too many arguments.\n");
		print_syntax();
		return EINVAL;
	}

	dev_name = argv[0];
	seat_name = argv[1];

	rc = loc_service_get_id(dev_name, &svc_id, 0);
	if (rc != EOK) {
		printf(NAME ": Device service '%s' not found.\n",
		    dev_name);
		return ENOENT;
	}

	rc = dispcfg_open(dcfg_svc, NULL, NULL, &dispcfg);
	if (rc != EOK) {
		printf(NAME ": Failed connecting to display configuration "
		    "service: %s.\n", str_error(rc));
		return rc;
	}

	rc = seat_find_by_name(dispcfg, seat_name, &seat_id);
	if (rc != EOK) {
		printf(NAME ": Seat '%s' not found.\n", seat_name);
		dispcfg_close(dispcfg);
		return ENOENT;
	}

	rc = dispcfg_dev_assign(dispcfg, svc_id, seat_id);
	if (rc != EOK) {
		printf(NAME ": Failed assigning device '%s' to seat '%s': "
		    "%s\n", dev_name, seat_name, str_error(rc));
		dispcfg_close(dispcfg);
		return EIO;
	}

	dispcfg_close(dispcfg);
	return EOK;
}

/** Unassign device subcommand.
 *
 * @param dcfg_svc Display configuration service name
 * @param argc Number of arguments
 * @param argv Arguments
 * @return EOK on success or an erro code
 */
static errno_t dev_unassign(const char *dcfg_svc, int argc, char *argv[])
{
	dispcfg_t *dispcfg;
	char *dev_name;
	service_id_t svc_id;
	errno_t rc;

	if (argc < 1) {
		printf(NAME ": Missing arguments.\n");
		print_syntax();
		return EINVAL;
	}

	if (argc > 1) {
		printf(NAME ": Too many arguments.\n");
		print_syntax();
		return EINVAL;
	}

	dev_name = argv[0];

	rc = loc_service_get_id(dev_name, &svc_id, 0);
	if (rc != EOK) {
		printf(NAME ": Device service '%s' not found.\n",
		    dev_name);
		return ENOENT;
	}

	rc = dispcfg_open(dcfg_svc, NULL, NULL, &dispcfg);
	if (rc != EOK) {
		printf(NAME ": Failed connecting to display configuration "
		    "service: %s.\n", str_error(rc));
		return rc;
	}

	rc = dispcfg_dev_unassign(dispcfg, svc_id);
	if (rc != EOK) {
		printf(NAME ": Failed unassigning device '%s': %s\n",
		    dev_name, str_error(rc));
		dispcfg_close(dispcfg);
		return EIO;
	}

	dispcfg_close(dispcfg);
	return EOK;
}

/** List dev subcommand.
 *
 * @param dcfg_svc Display configuration service name
 * @param argc Number of arguments
 * @param argv Arguments
 * @return EOK on success or an erro code
 */
static errno_t list_dev(const char *dcfg_svc, int argc, char *argv[])
{
	dispcfg_t *dispcfg;
	char *seat_name;
	sysarg_t seat_id;
	dispcfg_dev_list_t *dev_list;
	size_t i;
	char *svc_name;
	table_t *table = NULL;
	errno_t rc;

	if (argc < 1) {
		printf(NAME ": Missing arguments.\n");
		print_syntax();
		return EINVAL;
	}

	if (argc > 1) {
		printf(NAME ": Too many arguments.\n");
		print_syntax();
		return EINVAL;
	}

	seat_name = argv[0];

	rc = dispcfg_open(dcfg_svc, NULL, NULL, &dispcfg);
	if (rc != EOK) {
		printf(NAME ": Failed connecting to display configuration "
		    "service: %s.\n", str_error(rc));
		return rc;
	}

	rc = seat_find_by_name(dispcfg, seat_name, &seat_id);
	if (rc != EOK) {
		printf(NAME ": Seat '%s' not found.\n", seat_name);
		dispcfg_close(dispcfg);
		return ENOENT;
	}

	rc = dispcfg_get_asgn_dev_list(dispcfg, seat_id, &dev_list);
	if (rc != EOK) {
		printf(NAME ": Failed getting seat list.\n");
		dispcfg_close(dispcfg);
		return rc;
	}

	rc = table_create(&table);
	if (rc != EOK) {
		printf("Memory allocation failed.\n");
		dispcfg_free_dev_list(dev_list);
		dispcfg_close(dispcfg);
		return rc;
	}

	table_header_row(table);
	table_printf(table, "Device Name\n");

	for (i = 0; i < dev_list->ndevs; i++) {
		rc = loc_service_get_name(dev_list->devs[i], &svc_name);
		if (rc != EOK) {
			printf("Failed getting name of service %zu\n",
			    (size_t)dev_list->devs[i]);
			continue;
		}

		table_printf(table, "%s\n", svc_name);
		free(svc_name);
	}

	if (dev_list->ndevs != 0) {
		rc = table_print_out(table, stdout);
		if (rc != EOK) {
			printf("Error printing table.\n");
			table_destroy(table);
			dispcfg_free_dev_list(dev_list);
			dispcfg_close(dispcfg);
			return rc;
		}
	}

	dispcfg_close(dispcfg);
	return EOK;
}

int main(int argc, char *argv[])
{
	const char *dispcfg_svc = DISPCFG_DEFAULT;
	errno_t rc;

	if (argc < 2 || str_cmp(argv[1], "-h") == 0) {
		print_syntax();
		return 0;
	}

	if (str_cmp(argv[1], "list-seat") == 0) {
		rc = list_seat(dispcfg_svc, argc - 2, argv + 2);
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "create-seat") == 0) {
		rc = create_seat(dispcfg_svc, argc - 2, argv + 2);
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "delete-seat") == 0) {
		rc = delete_seat(dispcfg_svc, argc - 2, argv + 2);
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "assign-dev") == 0) {
		rc = dev_assign(dispcfg_svc, argc - 2, argv + 2);
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "unassign-dev") == 0) {
		rc = dev_unassign(dispcfg_svc, argc - 2, argv + 2);
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "list-dev") == 0) {
		rc = list_dev(dispcfg_svc, argc - 2, argv + 2);
		if (rc != EOK)
			return 1;
	} else {
		printf(NAME ": Unknown command '%s'.\n", argv[1]);
		print_syntax();
		return 1;
	}

	return 0;
}

/** @}
 */
