#define D1(s, ...) s
#define D2(s, ...) s D1(__VA_ARGS__)
#define D3(s, ...) s D2(__VA_ARGS__)
#define D4(s, ...) s D3(__VA_ARGS__)

D1(a)
D2(a, b)
D3(a, b, c)
D4(a, b, c, d) 


#define __sun_attr___noreturn__ __attribute__((__noreturn__))
#define ___sun_attr_inner(__a) __sun_attr_##__a
#define __sun_attr__(__a) ___sun_attr_inner __a
#define __NORETURN __sun_attr__((__noreturn__))
__NORETURN
#define X(...)
#define Y(...)  1 __VA_ARGS__ 2
Y(X X() ())

