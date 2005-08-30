
#include <print.h>
#include <test.h>

void test(void)
{
    printf("char %c \n",'c');
    __u64 u64const = 0x0123456789ABCDEFLL;
//    printf(" p  %P  %p \n",0x1234567898765432);
    printf(" Printf test \n");
    printf(" Q  %Q  %q %Q \n",u64const, u64const);
    printf(" L  %L  %l \n",0x12345678 ,0x12345678);   
    printf(" W  %W  %w \n",0x1234 ,0x1234);   
    printf(" B  %B  %B \n",0x12 ,0x12);   
    
    return;
}

