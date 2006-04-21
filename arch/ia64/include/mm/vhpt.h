#ifndef __ia64_VHPT_H__
#define __ia64_VHPT_H__

#include <arch/mm/tlb.h>
#include <arch/mm/page.h>

__address vhpt_set_up(void);

static inline vhpt_entry_t tlb_entry_t2vhpt_entry_t(tlb_entry_t tentry) 
{
	vhpt_entry_t ventry;
	
	ventry.word[0]=tentry.word[0];
	ventry.word[1]=tentry.word[1];
	
	return ventry;
}

void vhpt_mapping_insert(__address va, asid_t asid, tlb_entry_t entry);
void vhpt_invalidate_all(void);
void vhpt_invalidate_asid(asid_t asid);


#endif

