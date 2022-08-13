/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 */

#include <typedefs.h>
#include <genarch/multiboot/multiboot.h>
#include <genarch/multiboot/multiboot2.h>
#include <genarch/fb/bfb.h>
#include <config.h>
#include <align.h>

#define MULTIBOOT2_TAG_ALIGN  8

static void multiboot2_cmdline(const multiboot2_cmdline_t *module)
{
	multiboot_cmdline(module->string);
}

static void multiboot2_module(const multiboot2_module_t *module)
{
	if (init.cnt < CONFIG_INIT_TASKS) {
		init.tasks[init.cnt].paddr = module->start;
		init.tasks[init.cnt].size = module->end - module->start;
		multiboot_extract_command(init.tasks[init.cnt].name,
		    CONFIG_TASK_NAME_BUFLEN, module->string);
		multiboot_extract_argument(init.tasks[init.cnt].arguments,
		    CONFIG_TASK_ARGUMENTS_BUFLEN, module->string);

		init.cnt++;
	}
}

static void multiboot2_memmap(uint32_t length, const multiboot2_memmap_t *memmap)
{
	multiboot2_memmap_entry_t *entry = (multiboot2_memmap_entry_t *)
	    ((uintptr_t) memmap + sizeof(*memmap));
	uint32_t pos = offsetof(multiboot2_tag_t, memmap) + sizeof(*memmap);

	while ((pos < length) && (e820counter < MEMMAP_E820_MAX_RECORDS)) {
		e820table[e820counter].base_address = entry->base_address;
		e820table[e820counter].size = entry->size;
		e820table[e820counter].type = entry->type;

		/* Compute address of next entry. */
		entry = (multiboot2_memmap_entry_t *)
		    ((uintptr_t) entry + memmap->entry_size);
		pos += memmap->entry_size;

		e820counter++;
	}
}

static void multiboot2_fbinfo(const multiboot2_fbinfo_t *fbinfo)
{
#ifdef CONFIG_FB
	if (fbinfo->visual == MULTIBOOT2_VISUAL_RGB) {
		bfb_addr = fbinfo->addr;
		bfb_width = fbinfo->width;
		bfb_height = fbinfo->height;
		bfb_bpp = fbinfo->bpp;
		bfb_scanline = fbinfo->scanline;

		bfb_red_pos = fbinfo->rgb.red_pos;
		bfb_red_size = fbinfo->rgb.red_size;

		bfb_green_pos = fbinfo->rgb.green_pos;
		bfb_green_size = fbinfo->rgb.green_size;

		bfb_blue_pos = fbinfo->rgb.blue_pos;
		bfb_blue_size = fbinfo->rgb.blue_size;
	}
#endif
}

/** Parse multiboot2 information structure.
 *
 * If @a signature does not contain a valid multiboot2 signature,
 * assumes no multiboot2 information is available.
 *
 * @param signature Should contain the multiboot2 signature.
 * @param info      Multiboot2 information structure.
 *
 */
void multiboot2_info_parse(uint32_t signature, const multiboot2_info_t *info)
{
	if (signature != MULTIBOOT2_LOADER_MAGIC)
		return;

	const multiboot2_tag_t *tag = (const multiboot2_tag_t *)
	    ALIGN_UP((uintptr_t) info + sizeof(*info), MULTIBOOT2_TAG_ALIGN);

	while (tag->type != MULTIBOOT2_TAG_TERMINATOR) {
		switch (tag->type) {
		case MULTIBOOT2_TAG_CMDLINE:
			multiboot2_cmdline(&tag->cmdline);
			break;
		case MULTIBOOT2_TAG_MODULE:
			multiboot2_module(&tag->module);
			break;
		case MULTIBOOT2_TAG_MEMMAP:
			multiboot2_memmap(tag->size, &tag->memmap);
			break;
		case MULTIBOOT2_TAG_FBINFO:
			multiboot2_fbinfo(&tag->fbinfo);
			break;
		}

		tag = (const multiboot2_tag_t *)
		    ALIGN_UP((uintptr_t) tag + tag->size, MULTIBOOT2_TAG_ALIGN);
	}
}

/** @}
 */
