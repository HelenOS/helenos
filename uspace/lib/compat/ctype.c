
#include "ctype.h"

int isxdigit(int ch) {
	return isdigit(ch) ||
	       (ch >= 'a' && ch <= 'f') ||
	       (ch >= 'A' && ch <= 'F');
}


