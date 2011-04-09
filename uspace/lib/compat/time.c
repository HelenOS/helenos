
#include "time.h"


struct tm *localtime(const time_t *timep) {
	// FIXME: stub
	static struct tm result = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	return &result;
}

char *asctime(const struct tm *tm) {
	// FIXME: stub
	static char result[] = "Mon Jan 01 00:00:00 1900\n";
	return result;
}

char *ctime(const time_t *timep) {
	return asctime(localtime(timep));
}

