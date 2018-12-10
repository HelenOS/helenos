/*
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup lprint
 * @{
 */

/**
 * @file
 * @brief Print on a printer
 *
 */

#include <errno.h>
#include <io/chardev.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>

#define NAME	"lprint"

#define BUF_SIZE 1024

static void syntax_print(void);

/** Get default printer port.
 *
 * @param sid Place to store service ID of the printer port
 * @return EOK on success or error code
 */
static errno_t lprint_get_def_printer_port(service_id_t *sid)
{
	category_id_t cid;
	service_id_t *sids;
	size_t nsids;
	errno_t rc;

	rc = loc_category_get_id("printer-port", &cid, 0);
	if (rc != EOK)
		return EIO;

	rc = loc_category_get_svcs(cid, &sids, &nsids);
	if (rc != EOK)
		return EIO;

	if (nsids < 1) {
		free(sids);
		return EIO;
	}

	*sid = sids[0];
	free(sids);
	return EOK;
}

/** Print a message.
 *
 * @param chardev Character device
 * @param argc Number of arguments
 * @param argv Arguments - strings to print
 *
 * @return EOK on success or error code
 */
static errno_t lprint_msg(chardev_t *chardev, int argc, char *argv[])
{
	const char *msg;
	size_t nbytes;
	const char *sep;
	errno_t rc;

	while (argc > 0) {
		msg = *argv;
		--argc;
		++argv;

		rc = chardev_write(chardev, msg, str_size(msg), &nbytes);
		if (rc != EOK) {
			printf(NAME ": Failed sending data.\n");
			return EIO;
		}

		sep = argc > 0 ? " " : "\n";

		rc = chardev_write(chardev, sep, str_size(sep), &nbytes);
		if (rc != EOK) {
			printf(NAME ": Failed sending data.\n");
			return EIO;
		}
	}

	return EOK;
}

/** Print a file.
 *
 * @param chardev Character device
 * @param fname File name
 *
 * @return EOK on success or error code
 */
static errno_t lprint_file(chardev_t *chardev, const char *fname)
{
	void *buf;
	size_t nread, nwritten;
	FILE *f = NULL;
	errno_t rc;

	buf = malloc(BUF_SIZE);
	if (buf == NULL) {
		printf(NAME ": Out of memory.\n");
		rc = ENOMEM;
		goto error;
	}

	f = fopen(fname, "rb");
	if (f == NULL) {
		printf(NAME ": Cannot open '%s'.\n", fname);
		rc = EIO;
		goto error;
	}

	while (true) {
		nread = fread(buf, 1, BUF_SIZE, f);
		if (ferror(f)) {
			printf(NAME ": Error reading file.\n");
			rc = EIO;
			goto error;
		}

		if (nread == 0)
			break;

		rc = chardev_write(chardev, buf, nread, &nwritten);
		if (rc != EOK) {
			printf(NAME ": Failed sending data.\n");
			rc = EIO;
			goto error;
		}

	}

	fclose(f);
	return EOK;
error:
	if (buf != NULL)
		free(buf);
	if (f != NULL)
		fclose(f);
	return rc;
}

int main(int argc, char **argv)
{
	chardev_t *chardev;
	errno_t rc;
	async_sess_t *sess;
	service_id_t sid;
	const char *svc_name = NULL;
	bool msg_mode = false;

	if (argc < 2) {
		printf(NAME ": Error, argument missing.\n");
		syntax_print();
		return 1;
	}

	--argc;
	++argv;

	while (*argv != NULL && *argv[0] == '-') {
		if (str_cmp(*argv, "-d") == 0) {
			--argc;
			++argv;

			if (*argv == NULL) {
				printf(NAME ": Error, argument missing.\n");
				syntax_print();
				return 1;
			}

			svc_name = *argv;
			--argc;
			++argv;
			continue;
		}

		if (str_cmp(*argv, "-m") == 0) {
			msg_mode = true;
			--argc;
			++argv;
			continue;
		}

		if (str_cmp(*argv, "--help") == 0) {
			--argc;
			++argv;

			if (*argv != NULL) {
				printf(NAME ": Error, unexpected argument.\n");
				syntax_print();
				return 1;
			}

			syntax_print();
			return 0;
		}

		printf(NAME ": Error, invalid argument.\n");
		syntax_print();
		return 1;
	}

	if (argc < 1) {
		printf(NAME ": Error, argument missing.\n");
		syntax_print();
		return 1;
	}

	if (!msg_mode && argc > 1) {
		printf(NAME ": Error, too many arguments.\n");
		syntax_print();
		return 1;
	}

	if (svc_name != NULL) {
		rc = loc_service_get_id(svc_name, &sid, 0);
		if (rc != EOK) {
			printf(NAME ": Failed resolving printer port service "
			    "'%s'.\n", svc_name);
			return 1;
		}
	} else {
		rc = lprint_get_def_printer_port(&sid);
		if (rc != EOK) {
			printf(NAME ": No printer found.\n");
			return 1;
		}
	}

	sess = loc_service_connect(sid, INTERFACE_DDF, 0);
	if (sess == NULL) {
		printf(NAME ": Failed connecting printer port service.\n");
		return 1;
	}

	rc = chardev_open(sess, &chardev);
	if (rc != EOK) {
		async_hangup(sess);
		printf(NAME ": Failed opening printer port device.\n");
		return 1;
	}

	if (msg_mode) {
		rc = lprint_msg(chardev, argc, argv);
		if (rc != EOK) {
			chardev_close(chardev);
			return 1;
		}
	} else {
		rc = lprint_file(chardev, argv[0]);
		if (rc != EOK) {
			chardev_close(chardev);
			return 1;
		}
	}

	chardev_close(chardev);
	return 0;
}

/** Print syntax help. */
static void syntax_print(void)
{
	printf("syntax:\n"
	    "\tlprint [<options>] <file>\n"
	    "\tlprint [<options>] -m <message...>\n"
	    "options:\n"
	    "\t-d <device>Print to the specified device\n");
}

/**
 * @}
 */
