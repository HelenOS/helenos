#ifndef _INODE_H_
#define _INODE_H_

#include "const.h"

/*Declaration of the V2 inode as it is on disk.*/

struct mfs_v2_inode {
	uint16_t 	mode;		/*File type, protection, etc.*/
	uint16_t 	n_links;	/*How many links to this file.*/
	int16_t 	uid;		/*User id of the file owner.*/
	uint16_t 	gid;		/*Group number.*/
	int32_t 	size;		/*Current file size in bytes.*/
	int32_t		atime;		/*When was file data last accessed.*/
	int32_t		mtime;		/*When was file data last changed.*/
	int32_t		ctime;		/*When was inode data last changed.*/
	/*Block nums for direct, indirect, and double indirect zones.*/
	uint32_t	zone[V2_NR_TZONES];
};

#endif

