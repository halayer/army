#ifndef _BUS_H_
#define _BUS_H_

#include "types.h"

typedef struct Bus {
	void *components[16];
	
	int total_components;
} Bus;

Bus *Bus_new();
int Bus_registerComponent(Bus *bus, void *component);
WORD Bus_read(Bus *bus, WORD address, int *error);
int Bus_write(Bus *bus, WORD address, WORD data, int mask, int *error);

#endif