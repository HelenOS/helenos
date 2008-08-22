/* Various things that are used in many files
 * Various temporary port work-arounds are addressed in __HELENOS__ , this
 * serves as a convenience and later as a guide to make "phony.h" for future
 * ports */

/* Specific port work-arounds : */
#define PATH_MAX 255
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 0

/* Work around for getenv() */
#define PATH "/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin"
#define PATH_DELIM ":"

/* Used in many places */
#define SMALL_BUFLEN 256
#define LARGE_BUFLEN 1024

/* How many words (arguments) are permitted, how big can a whole
 * sentence be? Similar to ARG_MAX */
#define WORD_MAX 255
#define INPUT_MAX 1024

/* Leftovers from Autoconf */
#define PACKAGE_MAINTAINER "Tim Post"
#define PACKAGE_BUGREPORT "echo@echoreply.us"
#define PACKAGE_NAME "bdsh"
#define PACKAGE_STRING "The brain dead shell"
#define PACKAGE_TARNAME "scli"
#define PACKAGE_VERSION "0.0.1"



