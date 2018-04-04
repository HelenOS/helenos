/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_APIC_H_
#define KERN_ia32_APIC_H_

#define L_APIC_BASE	0xfee00000
#define IO_APIC_BASE	0xfec00000

#ifndef __ASSEMBLER__

#include <cpu.h>
#include <stdint.h>

#define FIXED  (0 << 0)
#define LOPRI  (1 << 0)

#define APIC_ID_COUNT  16

/* local APIC macros */
#define IPI_INIT     0
#define IPI_STARTUP  0

/** Delivery modes. */
#define DELMOD_FIXED    0x0U
#define DELMOD_LOWPRI   0x1U
#define DELMOD_SMI      0x2U
/* 0x3 reserved */
#define DELMOD_NMI      0x4U
#define DELMOD_INIT     0x5U
#define DELMOD_STARTUP  0x6U
#define DELMOD_EXTINT   0x7U

/** Destination modes. */
#define DESTMOD_PHYS   0x0U
#define DESTMOD_LOGIC  0x1U

/** Trigger Modes. */
#define TRIGMOD_EDGE   0x0U
#define TRIGMOD_LEVEL  0x1U

/** Levels. */
#define LEVEL_DEASSERT  0x0U
#define LEVEL_ASSERT    0x1U

/** Destination Shorthands. */
#define SHORTHAND_NONE      0x0U
#define SHORTHAND_SELF      0x1U
#define SHORTHAND_ALL_INCL  0x2U
#define SHORTHAND_ALL_EXCL  0x3U

/** Interrupt Input Pin Polarities. */
#define POLARITY_HIGH  0x0U
#define POLARITY_LOW   0x1U

/** Divide Values. (Bit 2 is always 0) */
#define DIVIDE_2    0x0U
#define DIVIDE_4    0x1U
#define DIVIDE_8    0x2U
#define DIVIDE_16   0x3U
#define DIVIDE_32   0x8U
#define DIVIDE_64   0x9U
#define DIVIDE_128  0xaU
#define DIVIDE_1    0xbU

/** Timer Modes. */
#define TIMER_ONESHOT   0x0U
#define TIMER_PERIODIC  0x1U

/** Delivery status. */
#define DELIVS_IDLE     0x0U
#define DELIVS_PENDING  0x1U

/** Destination masks. */
#define DEST_ALL  0xffU

/** Dest format models. */
#define MODEL_FLAT     0xfU
#define MODEL_CLUSTER  0x0U

/** Interrupt Command Register. */
#define ICRlo  (0x300U / sizeof(uint32_t))
#define ICRhi  (0x310U / sizeof(uint32_t))

typedef struct {
	union {
		uint32_t lo;
		struct {
			uint8_t vector;                 /**< Interrupt Vector. */
			unsigned int delmod : 3;        /**< Delivery Mode. */
			unsigned int destmod : 1;       /**< Destination Mode. */
			unsigned int delivs : 1;        /**< Delivery status (RO). */
			unsigned int : 1;               /**< Reserved. */
			unsigned int level : 1;         /**< Level. */
			unsigned int trigger_mode : 1;  /**< Trigger Mode. */
			unsigned int : 2;               /**< Reserved. */
			unsigned int shorthand : 2;     /**< Destination Shorthand. */
			unsigned int : 12;              /**< Reserved. */
		} __attribute__((packed));
	};
	union {
		uint32_t hi;
		struct {
			unsigned int : 24;  /**< Reserved. */
			uint8_t dest;       /**< Destination field. */
		} __attribute__((packed));
	};
} __attribute__((packed)) icr_t;

/* End Of Interrupt. */
#define EOI  (0x0b0U / sizeof(uint32_t))

/** Error Status Register. */
#define ESR  (0x280U / sizeof(uint32_t))

typedef union {
	uint32_t value;
	uint8_t err_bitmap;
	struct {
		unsigned int send_checksum_error : 1;
		unsigned int receive_checksum_error : 1;
		unsigned int send_accept_error : 1;
		unsigned int receive_accept_error : 1;
		unsigned int : 1;
		unsigned int send_illegal_vector : 1;
		unsigned int received_illegal_vector : 1;
		unsigned int illegal_register_address : 1;
		unsigned int : 24;
	} __attribute__((packed));
} esr_t;

/* Task Priority Register */
#define TPR  (0x080U / sizeof(uint32_t))

typedef union {
	uint32_t value;
	struct {
		unsigned int pri_sc : 4;  /**< Task Priority Sub-Class. */
		unsigned int pri : 4;     /**< Task Priority. */
	} __attribute__((packed));
} tpr_t;

/** Spurious-Interrupt Vector Register. */
#define SVR  (0x0f0U / sizeof(uint32_t))

typedef union {
	uint32_t value;
	struct {
		uint8_t vector;                   /**< Spurious Vector. */
		unsigned int lapic_enabled : 1;   /**< APIC Software Enable/Disable. */
		unsigned int focus_checking : 1;  /**< Focus Processor Checking. */
		unsigned int : 22;                /**< Reserved. */
	} __attribute__((packed));
} svr_t;

/** Time Divide Configuration Register. */
#define TDCR  (0x3e0U / sizeof(uint32_t))

typedef union {
	uint32_t value;
	struct {
		unsigned int div_value : 4;  /**< Divide Value, bit 2 is always 0. */
		unsigned int : 28;           /**< Reserved. */
	} __attribute__((packed));
} tdcr_t;

/* Initial Count Register for Timer */
#define ICRT  (0x380U / sizeof(uint32_t))

/* Current Count Register for Timer */
#define CCRT  (0x390U / sizeof(uint32_t))

/** LVT Timer register. */
#define LVT_Tm  (0x320U / sizeof(uint32_t))

typedef union {
	uint32_t value;
	struct {
		uint8_t vector;           /**< Local Timer Interrupt vector. */
		unsigned int : 4;         /**< Reserved. */
		unsigned int delivs : 1;  /**< Delivery status (RO). */
		unsigned int : 3;         /**< Reserved. */
		unsigned int masked : 1;  /**< Interrupt Mask. */
		unsigned int mode : 1;    /**< Timer Mode. */
		unsigned int : 14;        /**< Reserved. */
	} __attribute__((packed));
} lvt_tm_t;

/** LVT LINT registers. */
#define LVT_LINT0  (0x350U / sizeof(uint32_t))
#define LVT_LINT1  (0x360U / sizeof(uint32_t))

typedef union {
	uint32_t value;
	struct {
		uint8_t vector;                 /**< LINT Interrupt vector. */
		unsigned int delmod : 3;        /**< Delivery Mode. */
		unsigned int : 1;               /**< Reserved. */
		unsigned int delivs : 1;        /**< Delivery status (RO). */
		unsigned int intpol : 1;        /**< Interrupt Input Pin Polarity. */
		unsigned int irr : 1;           /**< Remote IRR (RO). */
		unsigned int trigger_mode : 1;  /**< Trigger Mode. */
		unsigned int masked : 1;        /**< Interrupt Mask. */
		unsigned int : 15;              /**< Reserved. */
	} __attribute__((packed));
} lvt_lint_t;

/** LVT Error register. */
#define LVT_Err  (0x370U / sizeof(uint32_t))

typedef union {
	uint32_t value;
	struct {
		uint8_t vector;           /**< Local Timer Interrupt vector. */
		unsigned int : 4;         /**< Reserved. */
		unsigned int delivs : 1;  /**< Delivery status (RO). */
		unsigned int : 3;         /**< Reserved. */
		unsigned int masked : 1;  /**< Interrupt Mask. */
		unsigned int : 15;        /**< Reserved. */
	} __attribute__((packed));
} lvt_error_t;

/** Local APIC ID Register. */
#define L_APIC_ID  (0x020U / sizeof(uint32_t))

typedef union {
	uint32_t value;
	struct {
		unsigned int : 24;  /**< Reserved. */
		uint8_t apic_id;    /**< Local APIC ID. */
	} __attribute__((packed));
} l_apic_id_t;

/** Local APIC Version Register */
#define LAVR       (0x030U / sizeof(uint32_t))
#define LAVR_Mask  0xffU

#define is_local_apic(x)    (((x) & LAVR_Mask & 0xf0U) == 0x1U)
#define is_82489DX_apic(x)  ((((x) & LAVR_Mask & 0xf0U) == 0x0U))
#define is_local_xapic(x)   (((x) & LAVR_Mask) == 0x14U)

/** Logical Destination Register. */
#define  LDR  (0x0d0U / sizeof(uint32_t))

typedef union {
	uint32_t value;
	struct {
		unsigned int : 24;  /**< Reserved. */
		uint8_t id;         /**< Logical APIC ID. */
	} __attribute__((packed));
} ldr_t;

/** Destination Format Register. */
#define DFR  (0x0e0U / sizeof(uint32_t))

typedef union {
	uint32_t value;
	struct {
		unsigned int : 28;       /**< Reserved, all ones. */
		unsigned int model : 4;  /**< Model. */
	} __attribute__((packed));
} dfr_t;

/* IO APIC */
#define IOREGSEL  (0x00U / sizeof(uint32_t))
#define IOWIN     (0x10U / sizeof(uint32_t))

#define IOAPICID   0x00U
#define IOAPICVER  0x01U
#define IOAPICARB  0x02U
#define IOREDTBL   0x10U

/** I/O Register Select Register. */
typedef union {
	uint32_t value;
	struct {
		uint8_t reg_addr;   /**< APIC Register Address. */
		unsigned int : 24;  /**< Reserved. */
	} __attribute__((packed));
} io_regsel_t;

/** I/O Redirection Register. */
typedef struct io_redirection_reg {
	union {
		uint32_t lo;
		struct {
			uint8_t intvec;                 /**< Interrupt Vector. */
			unsigned int delmod : 3;        /**< Delivery Mode. */
			unsigned int destmod : 1;       /**< Destination mode. */
			unsigned int delivs : 1;        /**< Delivery status (RO). */
			unsigned int intpol : 1;        /**< Interrupt Input Pin Polarity. */
			unsigned int irr : 1;           /**< Remote IRR (RO). */
			unsigned int trigger_mode : 1;  /**< Trigger Mode. */
			unsigned int masked : 1;        /**< Interrupt Mask. */
			unsigned int : 15;              /**< Reserved. */
		} __attribute__((packed));
	};
	union {
		uint32_t hi;
		struct {
			unsigned int : 24;  /**< Reserved. */
			uint8_t dest : 8;   /**< Destination Field. */
		} __attribute__((packed));
	};

} __attribute__((packed)) io_redirection_reg_t;


/** IO APIC Identification Register. */
typedef union {
	uint32_t value;
	struct {
		unsigned int : 24;         /**< Reserved. */
		unsigned int apic_id : 4;  /**< IO APIC ID. */
		unsigned int : 4;          /**< Reserved. */
	} __attribute__((packed));
} io_apic_id_t;

extern volatile uint32_t *l_apic;
extern volatile uint32_t *io_apic;

extern uint32_t apic_id_mask;
extern uint8_t bsp_l_apic;

extern void apic_init(void);

extern void l_apic_init(void);
extern void l_apic_eoi(void);
extern int l_apic_send_custom_ipi(uint8_t, uint8_t);
extern int l_apic_broadcast_custom_ipi(uint8_t);
extern int l_apic_send_init_ipi(uint8_t);
extern void l_apic_debug(void);

extern uint32_t io_apic_read(uint8_t);
extern void io_apic_write(uint8_t, uint32_t);
extern void io_apic_change_ioredtbl(uint8_t pin, uint8_t dest, uint8_t v, unsigned int);
extern void io_apic_disable_irqs(uint16_t);
extern void io_apic_enable_irqs(uint16_t);

#endif	/* __ASSEMBLER__ */

#endif

/** @}
 */
