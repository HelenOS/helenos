/*
 *	The PCI Library -- System-Dependent Stuff
 *
 *	Copyright (c) 1997--2004 Martin Mares <mj@ucw.cz>
 *
 *	Modified and ported to HelenOS by Jakub Jermar.
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#define NONRET __attribute__((noreturn))
#else
#define UNUSED
#define NONRET
#define inline
#endif

typedef u8 byte;
typedef u16 word;

#define cpu_to_le16(x) (x)
#define cpu_to_le32(x) (x)
#define le16_to_cpu(x) (x)
#define le32_to_cpu(x) (x)
