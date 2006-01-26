/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __ia64_PAGE_H__
#define __ia64_PAGE_H__

#include <arch/types.h>
#include <arch/mm/frame.h>

#define PAGE_SIZE	FRAME_SIZE
#define PAGE_WIDTH	FRAME_WIDTH

#define KA2PA(x)	((__address) (x))
#define PA2KA(x)	((__address) (x))

#define GET_PTL0_ADDRESS_ARCH()			((pte_t *) 0)
#define SET_PTL0_ADDRESS_ARCH(ptl0)

/** Implementation of page hash table interface. */
#define HT_ENTRIES_ARCH			0
#define HT_HASH_ARCH(page, asid)	0
#define HT_COMPARE_ARCH(page, asid, t)	0
#define HT_SLOT_EMPTY_ARCH(t)		1
#define HT_INVALIDATE_SLOT_ARCH(t)
#define HT_GET_NEXT_ARCH(t)		0
#define HT_SET_NEXT_ARCH(t, s)
#define HT_SET_RECORD_ARCH(t, page, asid, frame, flags)

#define REGION_RID_MAIN 0
#define REGION_RID_FIRST_INVALID 16
#define REGION_REGISTERS 8

#define VHPT_WIDTH 16         /*64kB*/
#define VHPT_SIZE (1<<VHPT_WIDTH)

#define VHPT_BASE 0           /* Must be aligned to VHPT_SIZE */

struct VHPT_tag_info
{
	unsigned long long tag       :63;
	unsigned           ti        : 1;
}__attribute__ ((packed));

union VHPT_tag
{
	struct VHPT_tag_info tag_info;
	unsigned             tag_word;
};

struct VHPT_entry_present
{

	/* Word 0 */
	unsigned p              : 1;
	unsigned rv0            : 1;
	unsigned ma             : 3;
	unsigned a              : 1;
	unsigned d              : 1;
	unsigned pl             : 2;
	unsigned ar             : 3;
	unsigned long long ppn  :38;
	unsigned rv1            : 2;
	unsigned ed             : 1;
	unsigned ig1            :11;
	
	/* Word 1 */
	unsigned rv2            : 2;
	unsigned ps             : 6;
	unsigned key            :24;
	unsigned rv3            :32;
	
	/* Word 2 */
	union VHPT_tag tag;       /*This data is here as union because I'm not sure if anybody nead access to areas ti and tag in VHPT entry*/
                            /* But I'm almost sure we nead access to whole word so there are both possibilities*/
	/* Word 3 */													
	unsigned long long next :64; /* This ignored field will be (in my hopes ;-) ) used as pointer in link list of entries with same hash value*/
	
}__attribute__ ((packed));

struct VHPT_entry_not_present
{
	/* Word 0 */
	unsigned p              : 1;
	unsigned long long ig0  :52;
	unsigned ig1            :11;
	
	/* Word 1 */
	unsigned rv2            : 2;
	unsigned ps             : 6;
	unsigned long long ig2  :56;

	
	/* Word 2 */
	union VHPT_tag tag;       /*This data is here as union because I'm not sure if anybody nead access to areas ti and tag in VHPT entry*/
                            /* But I'm almost sure we nead access to whole word so there are both possibilities*/
	/* Word 3 */													
	unsigned long long next :64; /* This ignored field will be (in my hopes ;-) ) used as pointer in link list of entries with same hash value*/
	
}__attribute__ ((packed));

typedef union VHPT_entry 
{
	struct VHPT_entry_present        present;
	struct VHPT_entry_not_present    not_present;
}VHPT_entry;

extern void page_arch_init(void);


struct region_register_map
{
  unsigned ve            : 1;
	unsigned r0            : 1;
	unsigned ps            : 6;
	unsigned rid           :24;
	unsigned r1            :32;
}__attribute__ ((packed));


typedef union region_register
{
	struct region_register_map   map;
	unsigned long long           word;
}region_register;

struct PTA_register_map
{
  unsigned ve            : 1;
	unsigned r0            : 1;
	unsigned size          : 6;
	unsigned vf            : 1;
	unsigned r1            : 6;
	unsigned long long base:49;
}__attribute__ ((packed));


typedef union PTA_register
{
	struct PTA_register_map   map;
	unsigned long long        word;
}PTA_register;


#endif
