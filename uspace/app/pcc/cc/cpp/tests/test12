#define y 2
#define fe(p)       sfe(p) p
#define sfe(p)    p
#define Y fe(y) y  fe(y)

Y;

#    define S2B_QMIN 0
#  define S2B_CMIN (S2B_QMIN + 8)
#define S2B_1(i)        i,
#define S2B_2(i)        S2B_1(i) S2B_1(i)
#define S2B_4(i)        S2B_2(i) S2B_2(i)
#define S2B_8(i)        S2B_4(i) S2B_4(i)
#define S2B_16(i)       S2B_8(i) S2B_8(i)
#define S2B_32(i)       S2B_16(i) S2B_16(i)
#define S2B_64(i)       S2B_32(i) S2B_32(i)
#define S2B_128(i)      S2B_64(i) S2B_64(i)
#define S2B_256(i)      S2B_128(i) S2B_128(i)
S2B_256(S2B_CMIN + 0)
