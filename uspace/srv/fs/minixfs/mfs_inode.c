#include <stdlib.h>
#include <mem.h>
#include "mfs.h"
#include "mfs_utils.h"

struct mfs_inode *mfs_read_inode_raw(const struct mfs_instance *instance, 
					uint16_t inum)
{
	struct mfs_inode *ino;
	struct mfs_sb_info *sbi;
	block_t *b;
	int i;

	const size_t ino_size = sizeof(struct mfs_inode);

	ino = (struct mfs_inode *) malloc(ino_size);

	if (!ino)
		return NULL;

	sbi = instance->sbi;

	block_get(&b, instance->handle, 2 + inum / V1_INODES_PER_BLOCK,
			BLOCK_FLAGS_NONE);

	memcpy(ino, ((uint8_t *) b->data) + inum * ino_size, ino_size);

	ino->i_mode = conv16(sbi->native, ino->i_mode);
	ino->i_uid = conv16(sbi->native, ino->i_uid);
	ino->i_size = conv32(sbi->native, ino->i_size);
	ino->i_mtime = conv32(sbi->native, ino->i_mtime);

	for (i = 0; i < V1_NR_DIRECT_ZONES; ++i)
		ino->i_dzone[i] = conv16(sbi->native, ino->i_dzone[i]);

	for (i = 0; i < V1_NR_INDIRECT_ZONES; ++i)
		ino->i_izone[i] = conv16(sbi->native, ino->i_izone[i]);

	block_put(b);

	return ino;
}

struct mfs2_inode *mfs2_read_inode_raw(const struct mfs_instance *instance,
					uint32_t inum)
{
	struct mfs2_inode *ino;
	struct mfs_sb_info *sbi;
	block_t *b;
	int i;

	const size_t ino_size = sizeof(struct mfs2_inode);

	ino = (struct mfs2_inode *) malloc(ino_size);

	if (!ino)
		return NULL;

	sbi = instance->sbi;

	block_get(&b, instance->handle, 
			2 + inum / V3_INODES_PER_BLOCK(sbi->block_size),
			BLOCK_FLAGS_NONE);

	memcpy(ino, ((uint8_t *)b->data) + inum * ino_size, ino_size);

	ino->i_mode = conv16(sbi->native, ino->i_mode);
	ino->i_nlinks = conv16(sbi->native, ino->i_nlinks);
	ino->i_uid = conv16(sbi->native, ino->i_uid);
	ino->i_gid = conv16(sbi->native, ino->i_gid);
	ino->i_size = conv32(sbi->native, ino->i_size);
	ino->i_atime = conv32(sbi->native, ino->i_atime);
	ino->i_mtime = conv32(sbi->native, ino->i_mtime);
	ino->i_ctime = conv32(sbi->native, ino->i_ctime);

	for (i = 0; i < V2_NR_DIRECT_ZONES; ++i)
		ino->i_dzone[i] = conv32(sbi->native, ino->i_dzone[i]);

	for (i = 0; i < V2_NR_INDIRECT_ZONES; ++i)
		ino->i_izone[i] = conv32(sbi->native, ino->i_izone[i]);

	block_put(b);

	return ino;
}

