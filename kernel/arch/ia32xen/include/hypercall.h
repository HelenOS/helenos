/*
 * Copyright (c) 2006 Martin Decky
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

#ifndef KERN_ia32xen_HYPERCALL_H_
#define KERN_ia32xen_HYPERCALL_H_

#ifndef __ASM__
#	include <arch/types.h>
#	include <macros.h>
#endif

#define GUEST_CMDLINE	1024
#define VIRT_CPUS	32
#define START_INFO_SIZE	1104

#define BOOT_OFFSET		0x0000
#define TEMP_STACK_SIZE 0x1000

#define XEN_VIRT_START	0xFC000000
#define XEN_CS			0xe019

#define XEN_ELFNOTE_INFO			0
#define XEN_ELFNOTE_ENTRY			1
#define XEN_ELFNOTE_HYPERCALL_PAGE	2
#define XEN_ELFNOTE_VIRT_BASE		3
#define XEN_ELFNOTE_PADDR_OFFSET	4
#define XEN_ELFNOTE_XEN_VERSION 	5
#define XEN_ELFNOTE_GUEST_OS		6
#define XEN_ELFNOTE_GUEST_VERSION	7
#define XEN_ELFNOTE_LOADER			8
#define XEN_ELFNOTE_PAE_MODE		9
#define XEN_ELFNOTE_FEATURES		10
#define XEN_ELFNOTE_BSD_SYMTAB		11

#define mp_map ((pfn_t *) XEN_VIRT_START)

#define SIF_PRIVILEGED	(1 << 0)  /**< Privileged domain */
#define SIF_INITDOMAIN	(1 << 1)  /**< Iinitial control domain */

#define XEN_CONSOLE_VGA		0x03
#define XEN_CONSOLE_VESA	0x23

#define XEN_SET_TRAP_TABLE		0
#define XEN_MMU_UPDATE			1
#define XEN_SET_CALLBACKS		4
#define XEN_UPDATE_VA_MAPPING	14
#define XEN_EVENT_CHANNEL_OP	16
#define XEN_VERSION				17
#define XEN_CONSOLE_IO			18
#define XEN_MMUEXT_OP			26


/*
 * Commands for XEN_CONSOLE_IO
 */
#define CONSOLE_IO_WRITE	0
#define CONSOLE_IO_READ		1


#define MMUEXT_PIN_L1_TABLE      0
#define MMUEXT_PIN_L2_TABLE      1
#define MMUEXT_PIN_L3_TABLE      2
#define MMUEXT_PIN_L4_TABLE      3
#define MMUEXT_UNPIN_TABLE       4
#define MMUEXT_NEW_BASEPTR       5
#define MMUEXT_TLB_FLUSH_LOCAL   6
#define MMUEXT_INVLPG_LOCAL      7
#define MMUEXT_TLB_FLUSH_MULTI   8
#define MMUEXT_INVLPG_MULTI      9
#define MMUEXT_TLB_FLUSH_ALL    10
#define MMUEXT_INVLPG_ALL       11
#define MMUEXT_FLUSH_CACHE      12
#define MMUEXT_SET_LDT          13
#define MMUEXT_NEW_USER_BASEPTR 15


#define EVTCHNOP_SEND			4


#define UVMF_NONE				0        /**< No flushing at all */
#define UVMF_TLB_FLUSH			1        /**< Flush entire TLB(s) */
#define UVMF_INVLPG				2        /**< Flush only one entry */
#define UVMF_FLUSHTYPE_MASK		3
#define UVMF_MULTI				0        /**< Flush subset of TLBs */
#define UVMF_LOCAL				0        /**< Flush local TLB */
#define UVMF_ALL				(1 << 2) /**< Flush all TLBs */


#define DOMID_SELF (0x7FF0U)
#define DOMID_IO   (0x7FF1U)

#ifndef __ASM__

typedef uint16_t domid_t;
typedef uint32_t evtchn_t;

typedef struct {
	uint32_t version;
	uint32_t pad0;
	uint64_t tsc_timestamp;   /**< TSC at last update of time vals */
	uint64_t system_time;     /**< Time, in nanosecs, since boot */
	uint32_t tsc_to_system_mul;
	int8_t tsc_shift;
	int8_t pad1[3];
} vcpu_time_info_t;

typedef struct {
	uint32_t cr2;
	uint32_t pad[5];
} arch_vcpu_info_t;

typedef struct arch_shared_info {
	pfn_t max_pfn;                  /**< max pfn that appears in table */
	uint32_t pfn_to_mfn_frame_list_list;
    uint32_t nmi_reason;
} arch_shared_info_t;

typedef struct {
	uint8_t evtchn_upcall_pending;
	ipl_t evtchn_upcall_mask;
	evtchn_t evtchn_pending_sel;
	arch_vcpu_info_t arch;
	vcpu_time_info_t time;
} vcpu_info_t;

typedef struct {
	vcpu_info_t vcpu_info[VIRT_CPUS];
	evtchn_t evtchn_pending[32];
	evtchn_t evtchn_mask[32];
	
	uint32_t wc_version;                  /**< Version counter */
	uint32_t wc_sec;                      /**< Secs  00:00:00 UTC, Jan 1, 1970 */
	uint32_t wc_nsec;                     /**< Nsecs 00:00:00 UTC, Jan 1, 1970 */
	
	arch_shared_info_t arch;
} shared_info_t;

typedef struct {
	int8_t magic[32];           /**< "xen-<version>-<platform>" */
	uint32_t frames;            /**< Available frames */
	shared_info_t *shared_info; /**< Shared info structure (machine address) */
	uint32_t flags;             /**< SIF_xxx flags */
	pfn_t store_mfn;            /**< Shared page (machine page) */
	evtchn_t store_evtchn;      /**< Event channel for store communication */
	
	union {
        struct {
			pfn_t mfn;          /**< Console page (machine page) */
			evtchn_t evtchn;    /**< Event channel for console messages */
        } domU;
		
        struct {
            uint32_t info_off;  /**< Offset of console_info struct */
            uint32_t info_size; /**< Size of console_info struct from start */
        } dom0;
    } console;
	
	pte_t *ptl0;                /**< Boot PTL0 (kernel address) */
	uint32_t pt_frames;         /**< Number of bootstrap page table frames */
	pfn_t *pm_map;              /**< Physical->machine frame map (kernel address) */
	void *mod_start;            /**< Modules start (kernel address) */
	uint32_t mod_len;           /**< Modules size (bytes) */
	int8_t cmd_line[GUEST_CMDLINE];
} start_info_t;

typedef struct {
    uint8_t video_type;  

    union {
        struct {
            uint16_t font_height;
            uint16_t cursor_x;
			uint16_t cursor_y;
            uint16_t rows;
			uint16_t columns;
        } vga;

        struct {
            uint16_t width;
			uint16_t height;
            uint16_t bytes_per_line;
            uint16_t bits_per_pixel;
            uint32_t lfb_base;
            uint32_t lfb_size;
            uint8_t red_pos;
			uint8_t red_size;
            uint8_t green_pos;
			uint8_t green_size;
            uint8_t blue_pos;
			uint8_t blue_size;
            uint8_t rsvd_pos;
			uint8_t rsvd_size;
        } vesa_lfb;
    } info;
} console_info_t;

typedef struct {
	pfn_t start;
	pfn_t size;
	pfn_t reserved;
} memzone_t;

extern start_info_t start_info;
extern shared_info_t shared_info;
extern memzone_t meminfo;

typedef struct {
	uint8_t vector;     /**< Exception vector */
	uint8_t flags;      /**< 0-3: privilege level; 4: clear event enable */
	uint16_t cs;        /**< Code selector */
	void *address;      /**< Code offset */
} trap_info_t;

typedef struct {
	evtchn_t port;
} evtchn_send_t;

typedef struct {
	uint32_t cmd;
	union {
		evtchn_send_t send;
    };
} evtchn_op_t;

#define force_evtchn_callback() ((void) xen_version(0, 0))

#define hypercall0(id)	\
	({	\
		unative_t ret;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret)	\
			:	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall1(id, p1)	\
	({	\
		unative_t ret, __ign1;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1)	\
			: "1" (p1)	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall2(id, p1, p2)	\
	({	\
		unative_t ret, __ign1, __ign2;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1),	\
			  "=c" (__ign2)	\
			: "1" (p1),	\
			  "2" (p2)	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall3(id, p1, p2, p3)	\
	({	\
		unative_t ret, __ign1, __ign2, __ign3;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1),	\
			  "=c" (__ign2),	\
			  "=d" (__ign3)	\
			: "1" (p1),	\
			  "2" (p2),	\
			  "3" (p3)	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall4(id, p1, p2, p3, p4)	\
	({	\
		unative_t ret, __ign1, __ign2, __ign3, __ign4;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1),	\
			  "=c" (__ign2),	\
			  "=d" (__ign3),	\
			  "=S" (__ign4)	\
			: "1" (p1),	\
			  "2" (p2),	\
			  "3" (p3),	\
			  "4" (p4)	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall5(id, p1, p2, p3, p4, p5)	\
	({	\
		unative_t ret, __ign1, __ign2, __ign3, __ign4, __ign5;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1),	\
			  "=c" (__ign2),	\
			  "=d" (__ign3),	\
			  "=S" (__ign4),	\
			  "=D" (__ign5)	\
			: "1" (p1),	\
			  "2" (p2),	\
			  "3" (p3),	\
			  "4" (p4),	\
			  "5" (p5)	\
			: "memory"	\
		);	\
		ret;	\
	})


static inline int xen_console_io(const unsigned int cmd, const unsigned int count, const char *str)
{
	return hypercall3(XEN_CONSOLE_IO, cmd, count, str);
}

static inline int xen_set_callbacks(const unsigned int event_selector, const void *event_address, const	unsigned int failsafe_selector, void *failsafe_address)
{
	return hypercall4(XEN_SET_CALLBACKS, event_selector, event_address, failsafe_selector, failsafe_address);
}

static inline int xen_set_trap_table(const trap_info_t *table)
{
	return hypercall1(XEN_SET_TRAP_TABLE, table);
}

static inline int xen_version(const unsigned int cmd, const void *arg)
{
	return hypercall2(XEN_VERSION, cmd, arg);
}

static inline int xen_notify_remote(evtchn_t channel)
{
    evtchn_op_t op;
	
    op.cmd = EVTCHNOP_SEND;
    op.send.port = channel;
    return hypercall1(XEN_EVENT_CHANNEL_OP, &op);
}

#endif

#endif
