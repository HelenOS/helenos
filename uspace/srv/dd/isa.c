#include <futex.h>
#include <assert.h>

#include "isa.h"

LIST_INITIALIZE(isa_bridges_list);
LIST_INITIALIZE(isa_drivers_list);

static atomic_t isa_bus_futex = FUTEX_INITIALIZER;

static void isa_probe_all(bridge_to_isa_t *bridge);
static void isa_drv_probe(isa_drv_t *drv);

int isa_bus_init()
{
	return 1;
}

void isa_register_bridge(bridge_to_isa_t *bridge)
{
	futex_down(&isa_bus_futex);
	
	printf("ISA: registering new sth-to-isa bridge.\n");
	
	// add bridge to the list 
	list_append(&(bridge->link), &isa_bridges_list);
	
	// call probe function of all registered  drivers of isa devices
	isa_probe_all(bridge);
	
	futex_up(&isa_bus_futex);
}

void isa_register_driver(isa_drv_t *drv)
{
	assert(drv->name != NULL);
	
	futex_down(&isa_bus_futex);
	
	printf("ISA: registering new driver '%s'.\n", drv->name);
	
	// add bridge to the list 
	list_append(&(drv->link), &isa_drivers_list);
	
	// call driver's probe function on all registered bridges
	isa_drv_probe(drv);
	
	futex_up(&isa_bus_futex);
}

static void isa_probe_all(bridge_to_isa_t *bridge)
{
	link_t *item = isa_drivers_list.next;
	isa_drv_t *drv = NULL; 
	
	while (item != &isa_drivers_list) {
		drv = list_get_instance(item, isa_drv_t, link);
		if (drv->ops != NULL && drv->ops->probe != NULL) {
			drv->ops->probe(bridge);
		}
		item = item->next;
	}
}

static void isa_drv_probe(isa_drv_t *drv)
{
	link_t *item = isa_bridges_list.next;
	bridge_to_isa_t *bridge = NULL; 
	
	if (drv->ops != NULL && drv->ops->probe != NULL) {
		while (item != &isa_bridges_list) {
			bridge = list_get_instance(item, bridge_to_isa_t, link);
			{
				drv->ops->probe(bridge);
			}
			item = item->next;
		}
	}	
}
