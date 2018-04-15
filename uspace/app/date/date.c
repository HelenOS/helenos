/*
 * Copyright (c) 2012 Maurizio Lombardi
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


#include <stdio.h>
#include <device/clock_dev.h>
#include <errno.h>
#include <loc.h>
#include <time.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <str.h>

#define NAME   "date"

static errno_t read_date_from_arg(char *wdate, struct tm *t);
static errno_t read_time_from_arg(char *wdate, struct tm *t);
static errno_t tm_sanity_check(struct tm *t);
static bool is_leap_year(int year);

static void usage(void);

static int days_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int
main(int argc, char **argv)
{
	errno_t rc;
	int c;
	category_id_t cat_id;
	size_t        svc_cnt;
	service_id_t  *svc_ids = NULL;
	service_id_t  svc_id;
	char          *svc_name = NULL;
	bool          read_only = true;
	char          *wdate = NULL;
	char          *wtime = NULL;
	struct tm     t;
	int           n_args = argc;

	while ((c = getopt(argc, argv, "hd:t:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			return 0;
		case 'd':
			if (wdate) {
				usage();
				return 1;
			}
			wdate = (char *)optarg;
			read_only = false;
			n_args -= 2;
			break;
		case 't':
			if (wtime) {
				usage();
				return 1;
			}
			wtime = (char *)optarg;
			read_only = false;
			n_args -= 2;
			break;
		case '?':
			usage();
			return 1;
		}
	}

	if (n_args != 1) {
		printf(NAME ": Unrecognized parameter\n");
		usage();
		return 1;
	}

	/* Get the id of the clock category */
	rc = loc_category_get_id("clock", &cat_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf(NAME ": Cannot get clock category id\n");
		goto exit;
	}

	/* Get the list of available services in the clock category */
	rc = loc_category_get_svcs(cat_id, &svc_ids, &svc_cnt);
	if (rc != EOK) {
		printf(NAME ": Cannot get the list of services in the clock "
		    "category\n");
		goto exit;
	}

	/* Check if there are available services in the clock category */
	if (svc_cnt == 0) {
		printf(NAME ": No available service found in "
		    "the clock category\n");
		goto exit;
	}

	/* Get the name of the clock service */
	rc = loc_service_get_name(svc_ids[0], &svc_name);
	if (rc != EOK) {
		printf(NAME ": Cannot get the name of the service\n");
		goto exit;
	}

	/* Get the service id for the device */
	rc = loc_service_get_id(svc_name, &svc_id, 0);
	if (rc != EOK) {
		printf(NAME ": Cannot get the service id for device %s",
		    svc_name);
		goto exit;
	}

	/* Connect to the device */
	async_sess_t *sess = loc_service_connect(svc_id, INTERFACE_DDF, 0);
	if (!sess) {
		printf(NAME ": Cannot connect to the device\n");
		goto exit;
	}

	/* Read the current date/time */
	rc = clock_dev_time_get(sess, &t);
	if (rc != EOK) {
		printf(NAME ": Cannot read the current time\n");
		goto exit;
	}

	if (read_only) {
		/* Print the current time and exit */
		printf("%02d/%02d/%d ", t.tm_mday,
		    t.tm_mon + 1, 1900 + t.tm_year);
		printf("%02d:%02d:%02d\n", t.tm_hour, t.tm_min, t.tm_sec);
	} else {
		if (wdate) {
			rc = read_date_from_arg(wdate, &t);
			if (rc != EOK) {
				printf(NAME ": error, date format not "
				    "recognized\n");
				usage();
				goto exit;
			}
		}
		if (wtime) {
			rc = read_time_from_arg(wtime, &t);
			if (rc != EOK) {
				printf(NAME ": error, time format not "
				    "recognized\n");
				usage();
				goto exit;
			}
		}

		rc = tm_sanity_check(&t);
		if (rc != EOK) {
			printf(NAME ": error, invalid date/time\n");
			goto exit;
		}

		rc = clock_dev_time_set(sess, &t);
		if (rc != EOK) {
			printf(NAME ": error, Unable to set date/time\n");
			goto exit;
		}
	}

exit:
	free(svc_name);
	free(svc_ids);
	return rc;
}

/** Read the day, month and year from a string
 *  with the following format: DD/MM/YYYY
 */
static errno_t
read_date_from_arg(char *wdate, struct tm *t)
{
	errno_t rc;
	uint32_t tmp;

	if (str_size(wdate) != 10) /* str_size("DD/MM/YYYY") == 10 */
		return EINVAL;

	if (wdate[2] != '/' ||
	    wdate[5] != '/') {
		return EINVAL;
	}

	rc = str_uint32_t(&wdate[0], NULL, 10, false, &tmp);
	if (rc != EOK)
		return rc;

	t->tm_mday = tmp;

	rc = str_uint32_t(&wdate[3], NULL, 10, false, &tmp);
	if (rc != EOK)
		return rc;

	t->tm_mon = tmp - 1;

	rc = str_uint32_t(&wdate[6], NULL, 10, false, &tmp);
	t->tm_year = tmp - 1900;

	return rc;
}

/** Read the hours, minutes and seconds from a string
 *  with the following format: HH:MM:SS or HH:MM
 */
static errno_t
read_time_from_arg(char *wtime, struct tm *t)
{
	errno_t rc;
	size_t len = str_size(wtime);
	bool sec_present = len == 8;
	uint32_t tmp;

	/* str_size("HH:MM") == 5 */
	/* str_size("HH:MM:SS") == 8 */
	if (len != 8 && len != 5)
		return EINVAL;

	if (sec_present && wtime[5] != ':')
		return EINVAL;

	if (wtime[2] != ':')
		return EINVAL;

	rc = str_uint32_t(&wtime[0], NULL, 10, false, &tmp);
	if (rc != EOK)
		return rc;

	t->tm_hour = tmp;

	rc = str_uint32_t(&wtime[3], NULL, 10, false, &tmp);
	if (rc != EOK)
		return rc;

	t->tm_min = tmp;

	if (sec_present) {
		rc = str_uint32_t(&wtime[6], NULL, 10, false, &tmp);
		t->tm_sec = tmp;
	} else
		t->tm_sec = 0;

	return rc;
}

/** Check if the tm structure contains valid values
 *
 * @param t     The tm structure to check
 *
 * @return      EOK on success or EINVAL
 */
static errno_t
tm_sanity_check(struct tm *t)
{
	int ndays;

	if (t->tm_sec < 0 || t->tm_sec > 59)
		return EINVAL;
	else if (t->tm_min < 0 || t->tm_min > 59)
		return EINVAL;
	else if (t->tm_hour < 0 || t->tm_hour > 23)
		return EINVAL;
	else if (t->tm_mday < 1 || t->tm_mday > 31)
		return EINVAL;
	else if (t->tm_mon < 0 || t->tm_mon > 11)
		return EINVAL;
	else if (t->tm_year < 0 || t->tm_year > 199)
		return EINVAL;

	if (t->tm_mon == 1/* FEB */ && is_leap_year(t->tm_year))
		ndays = 29;
	else
		ndays = days_month[t->tm_mon];

	if (t->tm_mday > ndays)
		return EINVAL;

	return EOK;
}

/** Check if a year is a leap year
 *
 * @param year   The year to check
 *
 * @return       true if it is a leap year, false otherwise
 */
static bool
is_leap_year(int year)
{
	bool r = false;

	if (year % 4 == 0) {
		if (year % 100 == 0)
			r = year % 400 == 0;
		else
			r = true;
	}

	return r;
}

static void
usage(void)
{
	printf("Usage: date [-d DD/MM/YYYY] [-t HH:MM[:SS]]\n");
	printf("       -d   Change the current date\n");
	printf("       -t   Change the current time\n");
	printf("       -h   Display this information\n");
}

