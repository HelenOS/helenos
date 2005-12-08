/*
 * Copyright (C) 2005 Ondrej Palkovsky
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

#ifndef __mips32_ARC_H__
#define __mips32_ARC_H__

#include <arch/types.h>

#define ARC_BASE_ADDR 0x1000;
#define ARC_MAGIC 0x53435241

typedef enum {
	SystemClass = 0,
	ProcessorClass,
	CacheClass,
	AdapterClass,
	ControllerClass,
	PeripheralClass,
	MemoryClass
} arc_component_class;

typedef enum {
	ARC_type = 0,
	CPU_type,
	FPU_type,
	PrimaryICache,
	PrimaryDCache,
	SecondaryICache,
	SecondaryDCache,
	SecondaryCache,
	Memory, /* Not in NT PROM */
	EISAAdapter,
	TCAdapter,
	SCSIAdapter,
	DTIAdapter,
	MultiFunctionAdapter,
	DiskController,
	TapeController,
	CDROMController,
	WORMController,
	SerialController,
	NetworkController,
	DisplayController,
	ParallelController,
	PointerController,
	KeyboardController,
	AudioController,
	OtherController,
	DiskPeripheral,
	FloppyDiskPeripheral,
	TapePeripheral,
	ModemPeripheral,
	MonitorPeripheral,
	PrinterPeripheral,
	PointerPeripheral,
	KeyboardPeripheral,
	TerminalPeripheral,
	LinePeripheral,
	NetworkPeripheral,
	OtherPeripheral,
	XTalkAdapter,
	PCIAdapter,
	GIOAdapter,
	TPUAdapter,
	Anonymous
}arc_component_type;

typedef enum {
	Failed = 1,
	ReadOnly = 2,
	Removable = 4,
	ConsoleIn = 8,
	ConsoleOut = 16,
	Input = 32,
	Output = 64
}arc_component_flags;

typedef struct  {
	arc_component_class class;
	arc_component_type type;
	arc_component_flags flags;
	__u16 revision;
	__u16 version;
	__u32 key;
	__u32 affinitymask;
	__u32 configdatasize;
	__u32 identifier_len;
	char *identifier;
} __attribute__ ((packed)) arc_component;

typedef struct {
	__u16 year;
	__u16 month;
	__u16 day;
	__u16 hour;
	__u16 minutes;
	__u16 seconds;
	__u16 mseconds;
} __attribute__ ((packed)) arc_timeinfo;

/* This is the SGI block structure, WinNT has it different */
typedef enum {
	ExceptionBlock,
	SystemParameterBlock,
	FreeContiguous,
	FreeMemory,
	BadMemory,
	LoadedProgram,
	FirmwareTemporary,
	FirmwarePermanent
}arc_memorytype_t;

typedef struct  {
	arc_memorytype_t type;
	__u32 basepage;  /* *4096 = baseaddr */
	__u32 basecount;
}arc_memdescriptor_t;

typedef struct {
	char vendorid[8];
	char prodid[8];
}arc_sysid_t;

typedef struct {
	long (*load)(void); /* ... */
	long (*invoke)(__u32 eaddr,__u32 saddr,__u32 argc,char **argv,
		       char **envp);
	long (*execute)(char *path,__u32 argc,char **argv,char **envp);
	void (*halt)(void);
	void (*powerdown)(void);
	void (*restart)(void);
	void (*reboot)(void);
	void (*enterinteractivemode)(void);
	long (*reserved)(void);
/* 10 */	
	arc_component * (*getpeer)(arc_component *c); 
	arc_component * (*getchild)(arc_component *c);
	arc_component * (*getparent)(arc_component *c);
	long (*getconfigurationdata)(void *configdata, arc_component *c);
	long (*addchild)(arc_component *c, arc_component *template,
			 void *configdata);
	long (*deletecomponet)(arc_component *current);
	long (*getcomponent)(char *path);
	long (*saveconfiguration)(void);
	arc_sysid_t (*getsystemid)(void);
	arc_memdescriptor_t * (*getmemorydescriptor)(arc_memdescriptor_t *cur);
/* 20 */
	long (*reserved2)(void);
	arc_timeinfo * (*gettime)(void);
	__u32 (*getrelativetime)(void);
	long (*getdirectoryentry)();
	long (*open)(void); /* ... */
	long (*close)(__u32 fileid);
	long (*read)(__u32 fileid,void *buf,__u32 n,__u32 *cnt);
	long (*getreadstatus)(__u32 fileid);
	long (*write)(__u32 fileid, void *buf,__u32 n,__u32 *cnt);
	long (*seek)(void); /* ... */
/* 30 */
	long (*mount)(void); /* ... */
	char * (*getenvironmentvariable)(char *name);
	char * (*setenvironmentvariable)(char *name, char *value);
	long (*getfileinformation)(void); /* ... */
	long (*setfileinformation)(__u32 fileid,__u32 attflags,__u32 attmask);
	void (*flushallcaches)(void);
	long (*testunicodecharacter)(void); /* ... */
	long (*getdisplaystatus)(void); /* ... */
} arc_func_vector_t;

typedef struct {
	__u32 signature;
	__u32 length;
	__u16 version;
	__u16 revision;
	void *restartblock;
	void *debugblock;
	void *gevector;
	void *utlbmissvector;
	__u32 firmwarevectorlen;
	arc_func_vector_t *firmwarevector;
	__u32 privvectorlen;
	void *privvector;
	__u32 adaptercount;
}__attribute__ ((packed)) arc_sbp;

extern int init_arc(void);
extern void arc_print_memory_map(void);
extern int arc_enabled(void);
extern void arc_putchar(char ch);
extern void arc_print_devices(void);
extern int arc_getchar(void);
#endif
