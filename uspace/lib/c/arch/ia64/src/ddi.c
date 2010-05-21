#include <libarch/ddi.h>
#include <sysinfo.h>

uint64_t ia64_iospace_address = 0;

uint64_t get_ia64_iospace_address(void)
{
	sysarg_t addr;
	if (sysinfo_get_value("ia64_iospace.address.virtual", &addr) != 0)
		addr = 0;
	
	return addr;
}

