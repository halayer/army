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

    /* MOV r0, #3
    MOV r1, #5
    MUL r2, r1, r0
    AND r0, r2, r1, LSL r0*/
    RAM_writeArea((void *)&ram, 0, 7, (void *)((WORD *)"\x03\x00\xa0\xe3\x05\x10\xa0\xe3\x91\x00\x02\xe0\x11\x00\x02\xe0"));
    
    Bus_registerComponent(bus, &ram_comp);

    ARM_reset(cpu);
    
    char c;
    int running = 1;
    WORD addr;

    do {
        ARM_step(cpu);
    } while (getchar() != 104);

    ARM_registerDump(cpu);

    return 0;
}
