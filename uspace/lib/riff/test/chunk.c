/*
 * Copyright (c) 2020 Jiri Svoboda
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
#include <pcut/pcut.h>
#include <riff/chunk.h>
#include <str.h>

PCUT_INIT;

PCUT_TEST_SUITE(chunk);

enum {
	CKID_dat1 = 0x31746164,
	CKID_dat2 = 0x32746164,
	LTYPE_lst1 = 0x3174736C,
	LTYPE_lst2 = 0x3274736C
};

/** Write and read back RIFF file containing just empty RIFF chunk */
PCUT_TEST(empty)
{
	char fname[L_tmpnam];
	char *p;
	riffw_t *rw;
	riffr_t *rr;
	riff_wchunk_t wriffck;
	riff_rchunk_t rriffck;
	errno_t rc;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	/* Write RIFF file */

	rc = riff_wopen(p, &rw);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(rw);

	rc = riff_wchunk_start(rw, CKID_RIFF, &wriffck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wriffck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wclose(rw);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Read back RIFF file */

	rc = riff_ropen(p, &rriffck, &rr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(rr);

	PCUT_ASSERT_INT_EQUALS(CKID_RIFF, rriffck.ckid);
	PCUT_ASSERT_INT_EQUALS(0, riff_rchunk_size(&rriffck));

	rc = riff_rclose(rr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	(void) remove(p);
}

/** Write and read back RIFF file containing two data chunks */
PCUT_TEST(data_chunks)
{
	char fname[L_tmpnam];
	char *p;
	char str1[] = "Hello";
	char str2[] = "World!";
	char buf[10];
	size_t nread;
	riffw_t *rw;
	riffr_t *rr;
	riff_wchunk_t wriffck;
	riff_wchunk_t wdatack;
	riff_rchunk_t rriffck;
	riff_rchunk_t rdatack;
	errno_t rc;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	/* Write RIFF file */

	rc = riff_wopen(p, &rw);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(rw);

	rc = riff_wchunk_start(rw, CKID_RIFF, &wriffck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Write first data chunk */

	rc = riff_wchunk_start(rw, CKID_dat1, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write(rw, str1, str_size(str1));
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Write second data chunk */

	rc = riff_wchunk_start(rw, CKID_dat2, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write(rw, str2, str_size(str2));
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wriffck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wclose(rw);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Read back RIFF file */

	rc = riff_ropen(p, &rriffck, &rr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(rr);

	PCUT_ASSERT_INT_EQUALS(CKID_RIFF, rriffck.ckid);

	/* Read first data chunk */

	rc = riff_rchunk_start(&rriffck, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(CKID_dat1, rdatack.ckid);

	rc = riff_read(&rdatack, buf, sizeof(buf), &nread);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(str_size(str1), nread);
	PCUT_ASSERT_INT_EQUALS(0, memcmp(buf, str1, nread));

	rc = riff_rchunk_end(&rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Read second data chunk */

	rc = riff_rchunk_start(&rriffck, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(CKID_dat2, rdatack.ckid);

	rc = riff_read(&rdatack, buf, sizeof(buf), &nread);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(str_size(str2), nread);
	PCUT_ASSERT_INT_EQUALS(0, memcmp(buf, str2, nread));

	rc = riff_rchunk_end(&rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_rclose(rr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	(void) remove(p);
}

/** Write and read back RIFF file containing two list chunks */
PCUT_TEST(list_chunks)
{
	char fname[L_tmpnam];
	char *p;
	riffw_t *rw;
	riffr_t *rr;
	riff_wchunk_t wriffck;
	riff_wchunk_t wlistck;
	riff_wchunk_t wdatack;
	riff_rchunk_t rriffck;
	riff_rchunk_t rlistck;
	riff_rchunk_t rdatack;
	uint32_t ltype;
	errno_t rc;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	/* Write RIFF file */

	rc = riff_wopen(p, &rw);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(rw);

	rc = riff_wchunk_start(rw, CKID_RIFF, &wriffck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Write first list chunk with two data chunks */

	rc = riff_wchunk_start(rw, CKID_LIST, &wlistck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, LTYPE_lst1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_start(rw, CKID_dat1, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_start(rw, CKID_dat2, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wlistck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Write second list chunk with one data chunk */

	rc = riff_wchunk_start(rw, CKID_LIST, &wlistck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, LTYPE_lst2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_start(rw, CKID_dat1, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wlistck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wriffck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wclose(rw);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Read back RIFF file */

	rc = riff_ropen(p, &rriffck, &rr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(rr);

	PCUT_ASSERT_INT_EQUALS(CKID_RIFF, rriffck.ckid);

	/* Read first list chunk with two data chunks */

	rc = riff_rchunk_start(&rriffck, &rlistck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(CKID_LIST, rlistck.ckid);

	rc = riff_read_uint32(&rlistck, &ltype);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(LTYPE_lst1, ltype);

	rc = riff_rchunk_start(&rlistck, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(CKID_dat1, rdatack.ckid);

	rc = riff_rchunk_end(&rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_rchunk_start(&rlistck, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(CKID_dat2, rdatack.ckid);

	rc = riff_rchunk_end(&rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_rchunk_start(&rlistck, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(ELIMIT, rc);

	rc = riff_rchunk_end(&rlistck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Read second list chunk with one data chunk */

	rc = riff_rchunk_start(&rriffck, &rlistck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(CKID_LIST, rlistck.ckid);

	rc = riff_read_uint32(&rlistck, &ltype);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(LTYPE_lst2, ltype);

	rc = riff_rchunk_start(&rlistck, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(CKID_dat1, rdatack.ckid);

	rc = riff_rchunk_end(&rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_rchunk_start(&rlistck, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(ELIMIT, rc);

	rc = riff_rchunk_start(&rlistck, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(ELIMIT, rc);

	rc = riff_rchunk_end(&rlistck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_rclose(rr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	(void) remove(p);
}

/** Match specific chunk type in a RIFF file */
PCUT_TEST(match_chunk)
{
	char fname[L_tmpnam];
	char *p;
	riffw_t *rw;
	riffr_t *rr;
	riff_wchunk_t wriffck;
	riff_wchunk_t wdatack;
	riff_rchunk_t rriffck;
	riff_rchunk_t rdatack;
	uint32_t rword;
	errno_t rc;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	/* Write RIFF file */

	rc = riff_wopen(p, &rw);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(rw);

	rc = riff_wchunk_start(rw, CKID_RIFF, &wriffck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Write first data chunk */

	rc = riff_wchunk_start(rw, CKID_dat1, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Write second data chunk */

	rc = riff_wchunk_start(rw, CKID_dat2, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Write third data chunk */

	rc = riff_wchunk_start(rw, CKID_dat1, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, 3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wriffck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wclose(rw);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Read back RIFF file */

	rc = riff_ropen(p, &rriffck, &rr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(rr);

	PCUT_ASSERT_INT_EQUALS(CKID_RIFF, rriffck.ckid);

	/* Match second data chunk */

	rc = riff_rchunk_match(&rriffck, CKID_dat2, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(CKID_dat2, rdatack.ckid);

	rc = riff_read_uint32(&rdatack, &rword);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(2, rword);

	rc = riff_rchunk_end(&rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Try matching dat2 again (should not match) */

	rc = riff_rchunk_match(&rriffck, CKID_dat2, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	/* Try matching dat1 again (but there's nothing left) */

	rc = riff_rchunk_match(&rriffck, CKID_dat1, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	rc = riff_rclose(rr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	(void) remove(p);
}

/** Match specific LIST chunk type in a RIFF file */
PCUT_TEST(list_match)
{
	char fname[L_tmpnam];
	char *p;
	riffw_t *rw;
	riffr_t *rr;
	riff_wchunk_t wriffck;
	riff_wchunk_t wdatack;
	riff_rchunk_t rriffck;
	riff_rchunk_t rdatack;
	uint32_t rword;
	errno_t rc;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	/* Write RIFF file */

	rc = riff_wopen(p, &rw);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(rw);

	rc = riff_wchunk_start(rw, CKID_RIFF, &wriffck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Write first LIST chunk */

	rc = riff_wchunk_start(rw, CKID_LIST, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, LTYPE_lst1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Write second data chunk */

	rc = riff_wchunk_start(rw, CKID_LIST, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, LTYPE_lst2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Write third data chunk */

	rc = riff_wchunk_start(rw, CKID_LIST, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, LTYPE_lst1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, 3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wriffck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wclose(rw);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Read back RIFF file */

	rc = riff_ropen(p, &rriffck, &rr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(rr);

	PCUT_ASSERT_INT_EQUALS(CKID_RIFF, rriffck.ckid);

	/* Match second LIST chunk */

	rc = riff_rchunk_list_match(&rriffck, LTYPE_lst2, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_read_uint32(&rdatack, &rword);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(2, rword);

	rc = riff_rchunk_end(&rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Try matching lst2 again (should not match) */

	rc = riff_rchunk_list_match(&rriffck, LTYPE_lst2, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	/* Try matching lst1 again (but there's nothing left) */

	rc = riff_rchunk_list_match(&rriffck, LTYPE_lst1, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	rc = riff_rclose(rr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	(void) remove(p);
}

/** Seek back to different positions in a chunk */
PCUT_TEST(rchunk_seek)
{
	char fname[L_tmpnam];
	char *p;
	riffw_t *rw;
	riffr_t *rr;
	riff_wchunk_t wriffck;
	riff_wchunk_t wdatack;
	riff_rchunk_t rriffck;
	riff_rchunk_t rdatack;
	uint32_t rword;
	errno_t rc;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	/* Write RIFF file */

	rc = riff_wopen(p, &rw);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(rw);

	rc = riff_wchunk_start(rw, CKID_RIFF, &wriffck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Write data chunk */

	rc = riff_wchunk_start(rw, CKID_dat1, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, 3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_write_uint32(rw, 4);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wchunk_end(rw, &wriffck);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_wclose(rw);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Read back RIFF file */

	rc = riff_ropen(p, &rriffck, &rr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(rr);

	PCUT_ASSERT_INT_EQUALS(CKID_RIFF, rriffck.ckid);

	/* Read data chunk */

	rc = riff_rchunk_start(&rriffck, &rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(CKID_dat1, rdatack.ckid);

	rc = riff_read_uint32(&rdatack, &rword);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(1, rword);

	rc = riff_rchunk_end(&rdatack);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Try reading first word of data chunk again */

	rc = riff_rchunk_seek(&rdatack, 0, SEEK_SET);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_read_uint32(&rdatack, &rword);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(1, rword);

	/* Try reading last word of data chunk */

	rc = riff_rchunk_seek(&rdatack, -4, SEEK_END);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_read_uint32(&rdatack, &rword);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(4, rword);

	/* Try reading previous word of data chunk */

	rc = riff_rchunk_seek(&rdatack, -8, SEEK_CUR);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = riff_read_uint32(&rdatack, &rword);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_INT_EQUALS(3, rword);

	rc = riff_rclose(rr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	(void) remove(p);
}

PCUT_EXPORT(chunk);
