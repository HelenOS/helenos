#include <assert.h>
#include <errno.h>
#include "mfs.h"
#include "mfs_utils.h"

static int read_map_v1(uint16_t *b, const struct mfs_node *mnode, int rblock);
static int read_map_v2(uint32_t *b, const struct mfs_node *mnode, int rblock);

/*Given the position in the file expressed in
 *bytes, this function returns the on-disk block
 *relative to that position.
 *Returns zero if the block does not exist.
 */
int read_map(uint32_t *b, const struct mfs_node *mnode, const uint32_t pos)
{
	int r;

	assert(mnode);
	assert(mnode->instance);

	const struct mfs_sb_info *sbi = mnode->instance->sbi;
	assert(sbi);

	const mfs_version_t fs_version = sbi->fs_version;
	const int block_size = sbi->block_size;

	/*Compute relative block number in file*/
	int rblock = pos / block_size;

	if (fs_version == MFS_VERSION_V1) {
		if (mnode->ino->i_size < (int32_t) pos)
			return -1;

		uint16_t b16 = 0;
		r = read_map_v1(&b16, mnode, rblock);
		*b = b16;
	} else {
		if (mnode->ino2->i_size < (int32_t) pos)
			return -1;

		r = read_map_v2(b, mnode, rblock);
	}

	return r;
}

static int read_map_v1(uint16_t *b, const struct mfs_node *mnode, int rblock)
{
	const int ptrs_per_block = MFS_BLOCKSIZE / sizeof(uint16_t);
	block_t *bi1, *bi2;
	const struct mfs_inode *ino = mnode->ino;
	int r;

	assert(ino);
	assert(mnode->instance);

	if (rblock < V1_NR_DIRECT_ZONES) {
		*b = ino->i_dzone[rblock];
		r = EOK;
		goto out;
	}

	rblock -= V1_NR_DIRECT_ZONES - 1;

	/*Check if the wanted block is in the single indirect zone*/
	if (rblock < ptrs_per_block) {
		if (ino->i_izone[0] == 0) {
			r = -1;
			goto out;
		}

		r = read_ind_block(bi1, mnode->instance, ino->i_izone[0]);

		if (r != EOK)
			goto out;

		*b = ((uint16_t *) bi1->data)[rblock];
		goto out_block;
	}

	rblock -= ptrs_per_block - 1;

	/*The wanted block is in the double indirect zone*/
	uint16_t di_block = rblock / ptrs_per_block;

	/*read the first indirect zone*/
	if (ino->i_izone[1] == 0) {
		r = -1;
		goto out;
	}

	r = read_ind_block(bi1, mnode->instance, ino->i_izone[1]);

	if (r != EOK)
		goto out;

	/*read the dobule indirect zone*/
	r = read_ind_block(bi2, mnode->instance,
			((uint16_t *) bi1->data)[di_block]);

	if (r != EOK)
		goto out_block;

	*b = ((uint16_t *) bi2->data)[rblock % ptrs_per_block];
	block_put(bi2);

out_block:
	block_put(bi1);
out:
	return r;
}

static int read_map_v2(uint32_t *b, const struct mfs_node *mnode, int rblock)
{
	block_t *bi1, *bi2;
	int r;

	assert(mnode);
	const struct mfs2_inode *ino2 = mnode->ino2;

	assert(ino2);
	assert(mnode->instance);

	const struct mfs_sb_info *sbi = mnode->instance->sbi;
	assert(sbi);

	const int ptrs_per_block = sbi->block_size / sizeof(uint32_t);

	if (rblock < V2_NR_DIRECT_ZONES) {
		*b = ino2->i_dzone[rblock];
		r = EOK;
		goto out;
	}
	rblock -= V2_NR_DIRECT_ZONES - 1;

	/*Check if the wanted block is in the single indirect zone*/
	if (rblock < ptrs_per_block) {
		if (ino2->i_izone[0] == 0) {
			r = -1;
			goto out;
		}

		r = read_ind_block(bi2, mnode->instance, ino2->i_izone[0]);

		if (r != EOK)
			goto out;

		*b = ((uint32_t *) bi1->data)[rblock];
		goto out_block;
	}

	rblock -= ptrs_per_block - 1;

	/*The wanted block is in the double indirect zone*/
	uint32_t di_block = rblock / ptrs_per_block;

	/*read the first indirect zone*/
	if (ino2->i_izone[1] == 0) {
		r = -1;
		goto out;
	}

	r = read_ind_block(bi1, mnode->instance, ino2->i_izone[1]);

	if (r != EOK)
		goto out;

	/*read the second indirect zone*/
	r = read_ind_block(bi2, mnode->instance,
			((uint32_t *) bi1->data)[di_block]);

	if (r != EOK)
		goto out_block;

	*b = ((uint32_t *) bi2->data)[rblock % ptrs_per_block];
	block_put(bi2);

out_block:
	block_put(bi1);
out:
	return r;
}

