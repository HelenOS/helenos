/* Various things that are used in many places including a few
 * tidbits left over from autoconf prior to the HelenOS port */

/* Specific port work-arounds : */
#ifndef PATH_MAX
#define PATH_MAX 255
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

/* Work around for getenv() */
#define PATH "/srv:/app"
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
#define PACKAGE_TARNAME "bdsh"
#define PACKAGE_VERSION "0.0.1"



