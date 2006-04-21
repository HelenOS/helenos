#include <arch/mm/vhpt.h>
#include <mm/frame.h>
#include <print.h>


static vhpt_entry_t* vhpt_base;

__address vhpt_set_up(void)
{
	vhpt_base=(vhpt_entry_t*) PA2KA(PFN2ADDR(frame_alloc(VHPT_WIDTH-FRAME_WIDTH,FRAME_KA)));
	if(!vhpt_base) panic("Kernel configured with VHPT but no memory for table.");
	vhpt_invalidate_all();
	return (__address) vhpt_base;
}


void vhpt_mapping_insert(__address va, asid_t asid, tlb_entry_t entry)
{
	region_register rr_save, rr;
	index_t vrn;
	rid_t rid;
	__u64 tag;

	vhpt_entry_t *ventry;


	vrn = va >> VRN_SHIFT;
	rid = ASID2RID(asid, vrn);
																												
  rr_save.word = rr_read(vrn);
  rr.word = rr_save.word;
  rr.map.rid = rid;
  rr_write(vrn, rr.word);
  srlz_i();
	
	ventry = (vhpt_entry_t *) thash(va);
  tag = ttag(va);
  rr_write(vrn, rr_save.word);
  srlz_i();
  srlz_d();

	ventry->word[0]=entry.word[0];
	ventry->word[1]=entry.word[1];
	ventry->present.tag.tag_word = tag;
	

}

void vhpt_invalidate_all()
{
	memsetb((__address)vhpt_base,1<<VHPT_WIDTH,0);
}

void vhpt_invalidate_asid(asid_t asid)
{
	vhpt_invalidate_all();
}


