#ifndef _BUS_H_
#define _BUS_H_

#include <stdint.h>
#include "types.h"

typedef struct Component {
	int has_addr_space;	// Indicates whether the component occupies any address space.
	int (*is_in_addr_space)(void *inst, WORD addr);	// Must return 1 when the given address is in the
							// component's address space, otherwise 0.
	WORD (*read_word)(void *inst, WORD addr);	// Read a byte from the component's address space.
	int (*write_word)(void *inst, WORD addr, WORD data);	// Write a byte to the component's address space.
	int (*read_area)(void *inst, WORD addr, int size, void *dst);	// Read from an area of the component's address
									// space, the size is given in amount of WORDs.
	int (*write_area)(void *inst, WORD addr, int size, void *src);	// Overwrite an area of the component's address
									// space, the size is given in amount of WORDs.
	void *inst;	// Component instance, which is passed to the callback functions.
} Component;

typedef struct Bus {
	Component components[16];
	
	int component_count;
} Bus;

Bus *Bus_new();
int Bus_registerComponent(Bus *bus, Component *component);
int Bus_findComponentByAddr(Bus *bus, WORD addr);
uint8_t Bus_read8(Bus *bus, WORD addr, int *error);
uint16_t Bus_read16(Bus *bus, WORD addr, int *error);
uint32_t Bus_read32(Bus *bus, WORD addr, int *error);
int Bus_write8(Bus *bus, WORD addr, uint8_t data);
int Bus_write16(Bus *bus, WORD addr, uint16_t data);
int Bus_write32(Bus *bus, WORD addr, uint32_t data);
int Bus_readArea(Bus *bus, WORD addr, int size, void *dst);
int Bus_writeArea(Bus *bus, WORD addr, int size, void *src);

#endif
