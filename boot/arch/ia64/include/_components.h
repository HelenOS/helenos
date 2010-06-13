/***************************************
 * AUTO-GENERATED FILE, DO NOT EDIT!!! *
 ***************************************/

#ifndef BOOT_COMPONENTS_H_
#define BOOT_COMPONENTS_H_

#include <typedefs.h>

#define COMPONENTS  9

typedef struct {
	const char *name;
	void *start;
	size_t size;
	size_t inflated;
} component_t;

extern component_t components[];

extern int _binary_kernel_bin_start;
extern int _binary_kernel_bin_size;

extern int _binary_ns_start;
extern int _binary_ns_size;

extern int _binary_loader_start;
extern int _binary_loader_size;

extern int _binary_init_start;
extern int _binary_init_size;

extern int _binary_devmap_start;
extern int _binary_devmap_size;

extern int _binary_rd_start;
extern int _binary_rd_size;

extern int _binary_vfs_start;
extern int _binary_vfs_size;

extern int _binary_fat_start;
extern int _binary_fat_size;

extern int _binary_initrd_img_start;
extern int _binary_initrd_img_size;

#endif
