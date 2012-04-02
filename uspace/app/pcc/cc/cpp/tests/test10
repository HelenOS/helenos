#define __CONCAT(x,y) x ## y
#define dev_type_open(n) int n(int)
#define dev_decl(n,t) __CONCAT(dev_type_,t)(__CONCAT(n,t))
#define cdev_decl(n) dev_decl(n,open)

cdev_decl(midi); 

# define __GETOPT_PREFIX foo_
# define __GETOPT_CONCAT(x, y) x ## y
# define __GETOPT_XCONCAT(x, y) __GETOPT_CONCAT (x, y)
# define __GETOPT_ID(y) __GETOPT_XCONCAT (__GETOPT_PREFIX, y)
# define optarg __GETOPT_ID (optarg)

optarg
