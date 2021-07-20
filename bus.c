#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "bus.h"

Bus *Bus_new() {
	Bus *bus = (Bus *)malloc(sizeof(Bus));
	memset((void *)bus, 0, sizeof(Bus));
	
	return bus;
}

int Bus_registerComponent(Bus *bus, Component *component) {
	if (bus->component_count == 16)
		return -1;
	
	bus->components[bus->component_count] = *component;
	bus->component_count++;
	
	return 0;
}

int Bus_findComponentByAddr(Bus *bus, WORD addr) {
	Component *comp;
	
	for (int i = 0; i < bus->component_count; i++) {
		comp = &bus->components[i];
		if (!comp->has_addr_space) continue;
		if (!comp->is_in_addr_space(comp->inst, addr)) continue;
		return i;
	}
	
	return -1;
}

uint8_t Bus_read8(Bus *bus, WORD addr, int *error) {
	return Bus_read32(bus, addr, error) & 0xFF;
}

uint16_t Bus_read16(Bus *bus, WORD addr, int *error) {
	return Bus_read32(bus, addr, error) & 0xFFFF;
}

uint32_t Bus_read32(Bus *bus, WORD addr, int *error) {
	int comp_index = Bus_findComponentByAddr(bus, addr);
	if (comp_index < 0) {
		*error = 1;
		return 0;
	}
	
	Component *comp = &bus->components[comp_index];
	return comp->read_word(comp->inst, addr);
}

int Bus_write8(Bus *bus, WORD addr, uint8_t data) {
	return Bus_write32(bus, addr, (Bus_read32(bus, addr, NULL) & 0xFFFFFF00) | data);
}

int Bus_write16(Bus *bus, WORD addr, uint16_t data) {
	return Bus_write32(bus, addr, (Bus_read32(bus, addr, NULL) & 0xFFFF0000) | data);
}

int Bus_write32(Bus *bus, WORD addr, uint32_t data) {
	int comp_index = Bus_findComponentByAddr(bus, addr);
	if (comp_index < 0) return -1;
	
	Component *comp = &bus->components[comp_index];
	return comp->write_word(comp->inst, addr, data);
}

int Bus_readArea(Bus *bus, WORD addr, int size, void *dst) {
	int comp_index = Bus_findComponentByAddr(bus, addr);
	if (comp_index < 0) return -1;
	
	Component *comp = &bus->components[comp_index];
	return comp->read_area(comp->inst, addr, size, dst);
}

int Bus_writeArea(Bus *bus, WORD addr, int size, void *src) {
	int comp_index = Bus_findComponentByAddr(bus, addr);
	if (comp_index < 0) return -1;
	
	Component *comp = &bus->components[comp_index];
	return comp->write_area(comp->inst, addr, size, src);
}
