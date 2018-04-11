/*
 * Copyright (c) 2017 Jiri Svoboda
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

#include <label/label.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stddef.h>
#include <stdlib.h>

PCUT_INIT;

PCUT_TEST_SUITE(label);

static errno_t label_test_get_bsize(void *, size_t *);
static errno_t label_test_get_nblocks(void *, aoff64_t *);
static errno_t label_test_read(void *, aoff64_t, size_t, void *);
static errno_t label_test_write(void *, aoff64_t, size_t, const void *);

label_bd_ops_t label_test_ops = {
	.get_bsize = label_test_get_bsize,
	.get_nblocks = label_test_get_nblocks,
	.read = label_test_read,
	.write = label_test_write
};

/** Pretended block device for testing */
typedef struct {
	char *data;
	size_t bsize;
	aoff64_t nblocks;
} test_bd_t;

enum {
	test_block_size = 512,
	test_nblocks = 1024
};

/** Create pretended block device.
 *
 * @param bsize Block size
 * @param nblocks Number of blocks
 * @param rbd Place to store pointer to new pretended block device
 */
static errno_t test_bd_create(size_t bsize, aoff64_t nblocks, test_bd_t **rbd)
{
	test_bd_t *bd;

	bd = calloc(1, sizeof(test_bd_t));
	if (bd == NULL)
		return ENOMEM;

	bd->data = calloc(bsize, nblocks);
	if (bd->data == NULL) {
		free(bd);
		return ENOMEM;
	}

	bd->bsize = bsize;
	bd->nblocks = nblocks;
	*rbd = bd;

	return EOK;
}

/** Destroy pretended block device.
 *
 * @param bd Pretended block device
 */
static void test_bd_destroy(test_bd_t *bd)
{
	free(bd->data);
	free(bd);
}

/** Get block size wrapper for liblabel */
static errno_t label_test_get_bsize(void *arg, size_t *bsize)
{
	test_bd_t *bd = (test_bd_t *)arg;

	*bsize = bd->bsize;
	return EOK;
}

/** Get number of blocks wrapper for liblabel */
static errno_t label_test_get_nblocks(void *arg, aoff64_t *nblocks)
{
	test_bd_t *bd = (test_bd_t *)arg;

	*nblocks = bd->nblocks;
	return EOK;
}

/** Read blocks wrapper for liblabel */
static errno_t label_test_read(void *arg, aoff64_t ba, size_t cnt, void *buf)
{
	test_bd_t *bd = (test_bd_t *)arg;

	if (ba >= bd->nblocks)
		return EINVAL;

	memcpy(buf, bd->data + ba * bd->bsize, bd->bsize);
	return EOK;
}

/** Write blocks wrapper for liblabel */
static errno_t label_test_write(void *arg, aoff64_t ba, size_t cnt, const void *data)
{
	test_bd_t *bd = (test_bd_t *)arg;

	if (ba >= bd->nblocks)
		return EINVAL;

	memcpy(bd->data + ba * bd->bsize, data, bd->bsize);
	return EOK;
}

PCUT_TEST(open_empty)
{
	label_t *label;
	label_bd_t lbd;
	label_info_t linfo;
	label_part_t *part;
	test_bd_t *bd = NULL;
	errno_t rc;

	rc = test_bd_create(test_block_size, test_nblocks, &bd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	lbd.ops = &label_test_ops;
	lbd.arg = (void *)bd;

	rc = label_open(&lbd, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = label_get_info(label, &linfo);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(lt_none, linfo.ltype);
	PCUT_ASSERT_INT_EQUALS(0, linfo.flags);

	/* There should be exactly one pseudo partition */
	part = label_part_first(label);
	PCUT_ASSERT_NOT_NULL(part);

	part = label_part_next(part);
	PCUT_ASSERT_NULL(part);

	label_close(label);

	test_bd_destroy(bd);
}

PCUT_TEST(create_destroy_mbr)
{
	label_t *label;
	label_bd_t lbd;
	label_info_t linfo;
	label_part_t *part;
	test_bd_t *bd = NULL;
	errno_t rc;

	rc = test_bd_create(test_block_size, test_nblocks, &bd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	lbd.ops = &label_test_ops;
	lbd.arg = (void *)bd;

	rc = label_create(&lbd, lt_mbr, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = label_get_info(label, &linfo);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(lt_mbr, linfo.ltype);
	PCUT_ASSERT_INT_EQUALS(lf_ext_supp | lf_can_create_pri |
	    lf_can_create_ext, linfo.flags);

	/* There should be no partitions */
	part = label_part_first(label);
	PCUT_ASSERT_NULL(part);

	/* Close and reopen */
	label_close(label);

	rc = label_open(&lbd, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = label_get_info(label, &linfo);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Everything should still be the same */
	PCUT_ASSERT_INT_EQUALS(lt_mbr, linfo.ltype);
	PCUT_ASSERT_INT_EQUALS(lf_ext_supp | lf_can_create_pri |
	    lf_can_create_ext, linfo.flags);

	rc = label_destroy(label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* There should be no label now */

	rc = label_open(&lbd, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = label_get_info(label, &linfo);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(lt_none, linfo.ltype);
	PCUT_ASSERT_INT_EQUALS(0, linfo.flags);

	label_close(label);

	test_bd_destroy(bd);
}

PCUT_TEST(create_destroy_gpt)
{
	label_t *label;
	label_bd_t lbd;
	label_info_t linfo;
	label_part_t *part;
	test_bd_t *bd = NULL;
	errno_t rc;

	rc = test_bd_create(test_block_size, test_nblocks, &bd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	lbd.ops = &label_test_ops;
	lbd.arg = (void *)bd;

	rc = label_create(&lbd, lt_gpt, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = label_get_info(label, &linfo);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(lt_gpt, linfo.ltype);
	PCUT_ASSERT_INT_EQUALS(lf_can_create_pri | lf_ptype_uuid, linfo.flags);

	/* There should be no partitions */
	part = label_part_first(label);
	PCUT_ASSERT_NULL(part);

	/* Close and reopen */
	label_close(label);

	rc = label_open(&lbd, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = label_get_info(label, &linfo);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Everything should still be the same */
	PCUT_ASSERT_INT_EQUALS(lt_gpt, linfo.ltype);
	PCUT_ASSERT_INT_EQUALS(lf_can_create_pri | lf_ptype_uuid, linfo.flags);

	rc = label_destroy(label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* There should be no label now */

	rc = label_open(&lbd, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = label_get_info(label, &linfo);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(lt_none, linfo.ltype);
	PCUT_ASSERT_INT_EQUALS(0, linfo.flags);

	label_close(label);

	test_bd_destroy(bd);
}

PCUT_TEST(mbr_primary_part)
{
	label_t *label;
	label_bd_t lbd;
	label_info_t linfo;
	label_part_t *part;
	label_part_spec_t pspec;
	label_part_info_t pinfo;
	label_ptype_t ptype;
	test_bd_t *bd = NULL;
	errno_t rc;

	rc = test_bd_create(test_block_size, test_nblocks, &bd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	lbd.ops = &label_test_ops;
	lbd.arg = (void *)bd;

	rc = label_create(&lbd, lt_mbr, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = label_get_info(label, &linfo);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* There should be no partitions */
	part = label_part_first(label);
	PCUT_ASSERT_NULL(part);

	rc = label_suggest_ptype(label, lpc_ext4, &ptype);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	memset(&pspec, 0, sizeof(pspec));
	pspec.index = 1;
	pspec.block0 = linfo.ablock0;
	pspec.nblocks = linfo.anblocks;
	pspec.hdr_blocks = 0;
	pspec.pkind = lpk_primary;
	pspec.ptype = ptype;

	rc = label_part_create(label, &pspec, &part);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	label_part_get_info(part, &pinfo);
	PCUT_ASSERT_INT_EQUALS(1, pinfo.index);
	PCUT_ASSERT_INT_EQUALS(lpk_primary, pinfo.pkind);
	PCUT_ASSERT_INT_EQUALS(linfo.ablock0, pinfo.block0);
	PCUT_ASSERT_INT_EQUALS(linfo.anblocks, pinfo.nblocks);

	/* Close and reopen */
	label_close(label);

	rc = label_open(&lbd, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = label_get_info(label, &linfo);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(lt_mbr, linfo.ltype);
	PCUT_ASSERT_INT_EQUALS(lf_ext_supp | lf_can_create_pri |
	    lf_can_create_ext | lf_can_delete_part, linfo.flags);

	part = label_part_first(label);
	PCUT_ASSERT_NOT_NULL(part);
	PCUT_ASSERT_NULL(label_part_next(part));

	label_part_get_info(part, &pinfo);
	PCUT_ASSERT_INT_EQUALS(1, pinfo.index);
	PCUT_ASSERT_INT_EQUALS(lpk_primary, pinfo.pkind);
	PCUT_ASSERT_INT_EQUALS(linfo.ablock0, pinfo.block0);
	PCUT_ASSERT_INT_EQUALS(linfo.anblocks, pinfo.nblocks);

	/* Destroy the partition */
	rc = label_part_destroy(part);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Close and reopen */
	label_close(label);

	rc = label_open(&lbd, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* There should be no partitions */
	part = label_part_first(label);
	PCUT_ASSERT_NULL(part);

	label_close(label);

	test_bd_destroy(bd);
}

PCUT_TEST(mbr_logical_part)
{
	label_t *label;
	label_bd_t lbd;
	label_info_t linfo;
	label_part_t *part, *lpart, *epart;
	label_part_spec_t pspec;
	label_ptype_t ptype;
	label_part_info_t pinfo, lpinfo, epinfo;
	test_bd_t *bd = NULL;
	errno_t rc;

	rc = test_bd_create(test_block_size, test_nblocks, &bd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	lbd.ops = &label_test_ops;
	lbd.arg = (void *)bd;

	rc = label_create(&lbd, lt_mbr, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = label_get_info(label, &linfo);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* There should be no partitions */
	part = label_part_first(label);
	PCUT_ASSERT_NULL(part);

	memset(&pspec, 0, sizeof(pspec));
	pspec.index = 1;
	pspec.block0 = linfo.ablock0;
	pspec.nblocks = linfo.anblocks;
	pspec.hdr_blocks = 0;
	pspec.pkind = lpk_extended;

	rc = label_part_create(label, &pspec, &epart);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	label_part_get_info(epart, &epinfo);
	PCUT_ASSERT_INT_EQUALS(1, epinfo.index);
	PCUT_ASSERT_INT_EQUALS(lpk_extended, epinfo.pkind);
	PCUT_ASSERT_INT_EQUALS(linfo.ablock0, epinfo.block0);
	PCUT_ASSERT_INT_EQUALS(linfo.anblocks, epinfo.nblocks);

	/* Close and reopen */
	label_close(label);

	rc = label_open(&lbd, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = label_get_info(label, &linfo);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(lt_mbr, linfo.ltype);
	PCUT_ASSERT_INT_EQUALS(lf_ext_supp | lf_can_create_pri |
	    lf_can_create_log | lf_can_delete_part, linfo.flags);

	epart = label_part_first(label);
	PCUT_ASSERT_NOT_NULL(epart);
	PCUT_ASSERT_NULL(label_part_next(epart));

	label_part_get_info(epart, &epinfo);
	PCUT_ASSERT_INT_EQUALS(1, epinfo.index);
	PCUT_ASSERT_INT_EQUALS(lpk_extended, epinfo.pkind);
	PCUT_ASSERT_INT_EQUALS(linfo.ablock0, epinfo.block0);
	PCUT_ASSERT_INT_EQUALS(linfo.anblocks, epinfo.nblocks);

	/* Create logical partition */
	rc = label_suggest_ptype(label, lpc_ext4, &ptype);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	memset(&pspec, 0, sizeof(pspec));
	pspec.index = 0;
	pspec.block0 = epinfo.block0 + 1;
	pspec.nblocks = epinfo.nblocks - 1;
	pspec.hdr_blocks = 1;
	pspec.pkind = lpk_logical;
	pspec.ptype = ptype;

	rc = label_part_create(label, &pspec, &lpart);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	label_part_get_info(lpart, &lpinfo);
	PCUT_ASSERT_INT_EQUALS(5, lpinfo.index);
	PCUT_ASSERT_INT_EQUALS(lpk_logical, lpinfo.pkind);
	PCUT_ASSERT_INT_EQUALS(epinfo.block0 + 1, lpinfo.block0);
	PCUT_ASSERT_INT_EQUALS(epinfo.nblocks - 1, lpinfo.nblocks);

	/* Close and reopen */
	label_close(label);

	rc = label_open(&lbd, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Find the extended and the logical partition */

	epart = NULL;
	lpart = NULL;

	part = label_part_first(label);
	while (part != NULL) {
		label_part_get_info(part, &pinfo);
		if (pinfo.pkind == lpk_extended) {
			epart = part;
		} else {
			PCUT_ASSERT_INT_EQUALS(lpk_logical, pinfo.pkind);
			lpart = part;
		}

		part = label_part_next(part);
	}

	PCUT_ASSERT_NOT_NULL(epart);
	PCUT_ASSERT_NOT_NULL(lpart);

	/* Destroy the logical partition */
	rc = label_part_destroy(lpart);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Destroy the extended partition */
	rc = label_part_destroy(epart);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Close and reopen */
	label_close(label);

	rc = label_open(&lbd, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* There should be no partitions */
	part = label_part_first(label);
	PCUT_ASSERT_NULL(part);

	label_close(label);

	test_bd_destroy(bd);
}


PCUT_TEST(gpt_part)
{
	label_t *label;
	label_bd_t lbd;
	label_info_t linfo;
	label_part_t *part;
	label_part_spec_t pspec;
	label_part_info_t pinfo;
	label_ptype_t ptype;
	test_bd_t *bd = NULL;
	errno_t rc;

	rc = test_bd_create(test_block_size, test_nblocks, &bd);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	lbd.ops = &label_test_ops;
	lbd.arg = (void *)bd;

	rc = label_create(&lbd, lt_gpt, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = label_get_info(label, &linfo);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* There should be no partitions */
	part = label_part_first(label);
	PCUT_ASSERT_NULL(part);

	rc = label_suggest_ptype(label, lpc_ext4, &ptype);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	memset(&pspec, 0, sizeof(pspec));
	pspec.index = 1;
	pspec.block0 = linfo.ablock0;
	pspec.nblocks = linfo.anblocks;
	pspec.hdr_blocks = 0;
	pspec.pkind = lpk_primary;
	pspec.ptype = ptype;

	rc = label_part_create(label, &pspec, &part);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	label_part_get_info(part, &pinfo);
	PCUT_ASSERT_INT_EQUALS(1, pinfo.index);
	PCUT_ASSERT_INT_EQUALS(lpk_primary, pinfo.pkind);
	PCUT_ASSERT_INT_EQUALS(linfo.ablock0, pinfo.block0);
	PCUT_ASSERT_INT_EQUALS(linfo.anblocks, pinfo.nblocks);

	/* Close and reopen */
	label_close(label);

	rc = label_open(&lbd, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = label_get_info(label, &linfo);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(lt_gpt, linfo.ltype);
	PCUT_ASSERT_INT_EQUALS(lf_can_create_pri | lf_ptype_uuid |
	    lf_can_delete_part, linfo.flags);

	part = label_part_first(label);
	PCUT_ASSERT_NOT_NULL(part);
	PCUT_ASSERT_NULL(label_part_next(part));

	label_part_get_info(part, &pinfo);
	PCUT_ASSERT_INT_EQUALS(1, pinfo.index);
	PCUT_ASSERT_INT_EQUALS(lpk_primary, pinfo.pkind);
	PCUT_ASSERT_INT_EQUALS(linfo.ablock0, pinfo.block0);
	PCUT_ASSERT_INT_EQUALS(linfo.anblocks, pinfo.nblocks);

	/* Destroy the partition */
	rc = label_part_destroy(part);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Close and reopen */
	label_close(label);

	rc = label_open(&lbd, &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* There should be no partitions */
	part = label_part_first(label);
	PCUT_ASSERT_NULL(part);

	label_close(label);

	test_bd_destroy(bd);
}

PCUT_EXPORT(label);
