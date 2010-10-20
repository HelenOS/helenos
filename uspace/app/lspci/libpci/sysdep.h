/*
 *	The PCI Library -- System-Dependent Stuff
 *
 *	Copyright (c) 1997--2004 Martin Mares <mj@ucw.cz>
 *
 *	May 8, 2006 - Modified and ported to HelenOS by Jakub Jermar.
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

static inline void swap(u8 *x, u8 *y)
{
	u8 z = *x;
	*x = *y;
	*y = z;
}

static inline u16 invert_endianess_16(u16 x) 
{
	u8 *px = (u8 *)&x;
	swap(&px[0], &px[1]);
	return x;
}

static inline u32 invert_endianess_32(u32 x) 
{
	u8 *px = (u8 *)&x;
	swap(&px[0], &px[3]);
	swap(&px[1], &px[2]);
	return x;
}

#ifdef UARCH_sparc64
	#define cpu_to_le16(x) (invert_endianess_16(x))
	#define cpu_to_le32(x) (invert_endianess_32(x))
	#define le16_to_cpu(x) (invert_endianess_16(x))
	#define le32_to_cpu(x) (invert_endianess_32(x))
#else
	#define cpu_to_le16(x) (x)
	#define cpu_to_le32(x) (x)
	#define le16_to_cpu(x) (x)
	#define le32_to_cpu(x) (x)
#endif 
