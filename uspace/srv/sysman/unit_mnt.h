#ifndef SYSMAN_UNIT_MNT_H
#define SYSMAN_UNIT_MNT_H

typedef struct {
	const char *type;
	const char *mountpoint;
	const char *device;
} unit_mnt_t;

#endif

