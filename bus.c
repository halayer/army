#include <string.h>
#include <stdlib.h>
#include "bus.h"

Bus *Bus_new() {
	Bus *bus = (Bus *)malloc(sizeof(Bus));
	memset((void *)bus, 0, sizeof(Bus));
	
	return bus;
}

int Bus_registerComponent(Bus *bus, void *component) {
	memcpy(component, &bus, sizeof(Bus *));
	
	if (bus->total_components == 16)
		return -1;
	
	bus->components[bus->total_components] = component;
	bus->total_components++;
	
	return 0;
}

WORD Bus_read(Bus *bus, WORD address, int *error) {
	return ((WORD **)(bus->components[1] + sizeof(Bus *)))[0][address>>2];
}

int Bus_write(Bus *bus, WORD address, WORD data, int mask, int *error) {
	((WORD **)(bus->components[1] + sizeof(Bus *)))[0][address>>2] = (Bus_read(bus, address, error) & ~mask) | data;
}