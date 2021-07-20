#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ARM.h"
#include "bus.h"
#include "isa.h"

typedef struct RAM {
	Bus *bus;
	WORD *mem;
} RAM;

int RAM_checkAddrSpace(void *inst, WORD addr) { return 1; }
WORD RAM_readWord(void *inst, WORD addr) { return ((RAM *)inst)->mem[addr >> 2]; }
int RAM_writeWord(void *inst, WORD addr, WORD data) { ((RAM *)inst)->mem[addr >> 2] = data; }

int main() {
	Bus *bus = Bus_new();
	ARM *cpu = ARM_new(ARCH_ARM9);
	RAM ram;
	Component ram_comp = {1, RAM_checkAddrSpace, RAM_readWord, RAM_writeWord};
	ram_comp.inst = &ram;
	
	cpu->debug = stdout;
	cpu->bus = bus;
	
	ram.mem = malloc(512);
	memset(ram.mem, 0, 512);
	
	ram.mem[0] = ((WORD *)"\x05\x00\xa0\xe3")[0];
	ram.mem[1] = ((WORD *)"\x01\x10\xa0\x03")[0];
	ram.mem[2] = ((WORD *)"\x06\x20\xa0\x03")[0];
	ram.mem[3] = ((WORD *)"\x01\x00\xa0\xe1")[0];
	
	Bus_registerComponent(bus, &ram_comp);
	
	ARM_reset(cpu);
	
	while (getchar() != 104) {	// "H"
		ARM_step(cpu);
	}
	
	ARM_registerDump(cpu);
	
	return 0;
}
