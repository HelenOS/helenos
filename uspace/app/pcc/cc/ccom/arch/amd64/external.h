#ifndef _EXTERNAL_H_
#define _EXTERNAL_H_
#define NEED_SMULLL
#define NEED_SDIVLL
#define NEED_SMODLL
#define NEED_SPLUSLL
#define NEED_SMINUSLL
#define NEED_UMULLL
#define NEED_UDIVLL
#define NEED_UMODLL
#define NEED_UPLUSLL
#define NEED_UMINUSLL
#define MAXOPLEN 48
#define NUMBITS 64
#define BIT2BYTE(bits) ((((bits)+NUMBITS-1)/NUMBITS)*(NUMBITS/8))
#define BITSET(arr, bit) (arr[bit/NUMBITS] |= ((long)1 << (bit & (NUMBITS-1))))
#define BITCLEAR(arr, bit) (arr[bit/NUMBITS] &= ~((long)1 << (bit & (NUMBITS-1))))
#define TESTBIT(arr, bit) (arr[bit/NUMBITS] & ((long)1 << (bit & (NUMBITS-1))))
typedef long bittype;
extern int tempregs[], permregs[];
#define NTEMPREG 26
#define FREGS 25
#define NPERMREG 6
extern bittype validregs[];
#define AREGCNT 14
#define BREGCNT 16
#define CREGCNT 8
#define DREGCNT 0
#define EREGCNT 0
#define FREGCNT 0
#define GREGCNT 0
int aliasmap(int class, int regnum);
int color2reg(int color, int class);
int interferes(int reg1, int reg2);
#endif /* _EXTERNAL_H_ */
