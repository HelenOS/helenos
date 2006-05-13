#include <arch/types.h>

typedef union sysinfo_item_val
{
	__native val;
	void *fn; 
}sysinfo_item_val_t;

typedef struct sysinfo_item
{
	char *name;
	union
	{
		__native val;
		void *fn; 
	}val;

	union
	{
		struct sysinfo_item *table;
		void *fn;
	}subinfo;

	struct sysinfo_item *next;
	int val_type;
	int subinfo_type;
}sysinfo_item_t;

#define SYSINFO_VAL_VAL 0
#define SYSINFO_VAL_FUNCTION 1
#define SYSINFO_VAL_UNDEFINED '?'

#define SYSINFO_SUBINFO_NONE 0
#define SYSINFO_SUBINFO_TABLE 1
#define SYSINFO_SUBINFO_FUNCTION 2


typedef __native (*sysinfo_val_fn_t)(sysinfo_item_t *root);
typedef __native (*sysinfo_subinfo_fn_t)(const char *subname);

typedef struct sysinfo_rettype
{
	__native val;
	__native valid;
}sysinfo_rettype_t;

void sysinfo_set_item_val(const char *name,sysinfo_item_t **root,__native val);
void sysinfo_dump(sysinfo_item_t **root,int depth);
void sysinfo_set_item_function(const char *name,sysinfo_item_t **root,sysinfo_val_fn_t fn);
void sysinfo_set_item_undefined(const char *name,sysinfo_item_t **root);

sysinfo_rettype_t sysinfo_get_val(const char *name,sysinfo_item_t **root);

__native sys_sysinfo_valid(__native ptr,__native len);
__native sys_sysinfo_value(__native ptr,__native len);


