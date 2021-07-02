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

int main() {
	Bus *bus = Bus_new();
	ARM *cpu = ARM_new();
	RAM ram;
	
	cpu->arch = ARCH_ARM9;
	cpu->debug = stdout;
	
	ram.mem = malloc(512);
	memset(ram.mem, 0, 512);
	
	ram.mem[0] = ((WORD *)"\x05\x00\xa0\xe3")[0];
	ram.mem[1] = ((WORD *)"\x01\x10\xa0\xe3")[0];
	ram.mem[2] = ((WORD *)"\x06\x20\xa0\xe3")[0];
	ram.mem[3] = ((WORD *)"\x91\x02\x47\xe0")[0];
	
	Bus_registerComponent(bus, (void *)cpu);
	Bus_registerComponent(bus, (void *)&ram);
	
	ARM_reset(cpu);
	
	while (getchar() != 104) {	// "H"
		ARM_step(cpu);
	}
	
	ARM_registerDump(cpu);
	
	return 0;
}