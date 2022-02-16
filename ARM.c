#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ARM.h"
#include "cbs/bus.h"
#include "isa.h"

typedef struct RAM {
    Bus *bus;
    WORD *mem;
} RAM;

int RAM_checkAddrSpace(void *inst, uint64_t addr) { return 1; }
int RAM_read(void *inst, uint64_t addr, int size, void *dst) {
    RAM *r = (RAM *)inst;
    memcpy(dst, (void *)(r->mem) + addr, size);
    return 0;
}
int RAM_write(void *inst, uint64_t addr, int size, void *src) {
    RAM *r = (RAM *)inst;
    memcpy((void *)(r->mem) + addr, src, size);
    return 0;
}

int main() {
    Bus bus;
    ARM *cpu = ARM_new(ARCH_ARM9);
    RAM ram; ram.bus = &bus;
    Component ram_comp = {1, RAM_checkAddrSpace, RAM_read, RAM_write};
    ram_comp.inst = &ram;

    cpu->debug = bus.debug = stdout;
    cpu->bus = &bus;

    ram.mem = (WORD *)malloc(512);
    memset((void *)ram.mem, 0, 512);

    /* BL 8
    MOV r1, #1

    MOV r0, #1
    MOV pc, lr */
    char prog[16] = "\x00\x00\x00\xeb\x01\x10\xa0\xe3\x01\x00\xa0\xe3\x0e\xf0\xa0\xe1";
    memcpy((void *)ram.mem, (void *)&prog, sizeof(prog));
    
    Bus_attachComponent(&bus, &ram_comp);

    ARM_reset(cpu);
    ARM_switchMode(cpu, MODE_USER);

    do {
        ARM_step(cpu);
    } while (getchar() != 104);

    ARM_registerDump(cpu);
    if (ARM_getMode(cpu) == MODE_USER) { printf("USR\n"); }
    else if (ARM_getMode(cpu) == MODE_SUPERVISOR) { printf("SVC\n"); }

    return 0;
}
