#ifndef ISA_H
#define ISA_H

#include <adt/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int isa_bus_init();

struct bridge_to_isa;
struct bridge_to_isa_ops;
struct isa_drv_ops;
struct isa_drv;

typedef struct bridge_to_isa bridge_to_isa_t;
typedef struct bridge_to_isa_ops bridge_to_isa_ops_t;
typedef struct isa_drv_ops isa_drv_ops_t;
typedef struct isa_drv isa_drv_t;


struct isa_drv_ops {
	void (*probe)(bridge_to_isa_t *parent);
};

struct isa_drv {
	const char *name;
	link_t link;
	isa_drv_ops_t *ops; 	
};

struct bridge_to_isa {
	link_t link;
	void *data;
	bridge_to_isa_ops_t *ops;	
};

struct bridge_to_isa_ops {
	void * (*absolutize)(void *phys_addr);	
};

static inline bridge_to_isa_t * isa_alloc_bridge()
{
	bridge_to_isa_t *bridge = (bridge_to_isa_t *)malloc(sizeof(bridge_to_isa_t));
	link_initialize(&bridge->link);
	bridge->data = NULL;
	bridge->ops = NULL;	
	return bridge;
}

static inline void isa_init_bridge(bridge_to_isa_t *bridge, bridge_to_isa_ops_t *ops, void *data)
{
	bridge->data = data;
	bridge->ops = ops;	
}

void isa_register_bridge(bridge_to_isa_t *bridge);
void isa_register_driver(isa_drv_t *drv);

#endif
