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
int RAM_readArea(void *inst, WORD addr, int size, void *dst) { memcpy(dst, (void *)((RAM *)inst)->mem, size*4); }
int RAM_writeArea(void *inst, WORD addr, int size, void *src) { memcpy((void *)((RAM *)inst)->mem, src, size*4); }

int main() {
    Bus *bus = Bus_new();
    ARM *cpu = ARM_new(ARCH_ARM9);
    RAM ram; ram.bus = bus;
    Component ram_comp = {1, RAM_checkAddrSpace, RAM_readWord, RAM_writeWord, RAM_readArea, RAM_writeArea};
    ram_comp.inst = &ram;

    cpu->debug = stdout;
    cpu->bus = bus;

    ram.mem = malloc(512);
    memset(ram.mem, 0, 512);

    // LDR r0, [r2], r1, ASR #5
    // MOV r0, #0x80000000
    // MOV r0, r1, LSR r2
    // MOV r0, r1
    RAM_writeArea((void *)&ram, 0, 4, (void *)((WORD *)"\xc1\x02\x92\xe6\x02\x01\xa0\xe3\x31\x02\xa0\xe1\x01\x00\xa0\xe1"));

    Bus_registerComponent(bus, &ram_comp);

    ARM_reset(cpu);

    do {
        ARM_step(cpu);
    } while (getchar() != 104); // "H"

    ARM_registerDump(cpu);

    return 0;
}
