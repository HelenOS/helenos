
#include <print.h>
#include <test.h>

void test(void)
{
	__u64 u64const = 0x0123456789ABCDEFLL;
	printf(" Printf test \n");
	printf(" Q  %Q  %q \n",u64const, u64const);
	printf(" L  %L  %l \n",0x01234567 ,0x01234567);   
	printf(" W  %W  %w \n",0x0123 ,0x0123);   
	printf(" B  %B  %b \n",0x01 ,0x01);
	return;
}
