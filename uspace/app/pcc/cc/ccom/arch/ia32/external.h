#ifndef _EXTERNAL_H_
#define _EXTERNAL_H_
#define MAXOPLEN 42
#define NUMBITS 32
#define BIT2BYTE(bits) ((((bits)+NUMBITS-1)/NUMBITS)*(NUMBITS/8))
#define BITSET(arr, bit) (arr[bit/NUMBITS] |= ((int)1 << (bit & (NUMBITS-1))))
#define BITCLEAR(arr, bit) (arr[bit/NUMBITS] &= ~((int)1 << (bit & (NUMBITS-1))))
#define TESTBIT(arr, bit) (arr[bit/NUMBITS] & ((int)1 << (bit & (NUMBITS-1))))
typedef int bittype;
extern int tempregs[], permregs[];
#define NTEMPREG 4
#define FREGS 3
#define NPERMREG 4
extern bittype validregs[];
#define AREGCNT 6
#define BREGCNT 8
#define CREGCNT 15
#define DREGCNT 8
#define EREGCNT 0
#define FREGCNT 0
#define GREGCNT 0
int aliasmap(int class, int regnum);
int color2reg(int color, int class);
int interferes(int reg1, int reg2);
#endif /* _EXTERNAL_H_ */
