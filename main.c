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
    Bus bus = {0};
    ARM *cpu = ARM_new(ARCH_ARM7);
    RAM ram; ram.bus = &bus;
    Component ram_comp = {1, RAM_checkAddrSpace, RAM_read, RAM_write};
    ram_comp.inst = &ram;

    cpu->debug = stdout;
    //bus.debug = stdout;
    cpu->bus = &bus;

    ram.mem = (WORD *)malloc(512);
    memset((void *)ram.mem, 0, 512);

    char hndlr[4] = "\x0e\xf0\xb0\xe1";
    memcpy((void *)ram.mem + 0x8, (void *)hndlr, sizeof(hndlr));
    /* MOV r0, #1
    SWI #0
    MOV r1, #1 */
    char prog[12] = "\x01\x00\xa0\xe3\x00\x00\x00\xef\x01\x10\xa0\xe3";
    memcpy((void *)ram.mem + 0x10, (void *)&prog, sizeof(prog));
    
    Bus_attachComponent(&bus, &ram_comp);

    ARM_reset(cpu); cpu->r[15] = 0x10;
    ARM_switchMode(cpu, MODE_USER);
    
    do {
        ARM_step(cpu);
    } while (getchar() != 104);

    ARM_registerDump(cpu);
    if (ARM_getMode(cpu) == MODE_USER) { printf("USR\n"); }
    else if (ARM_getMode(cpu) == MODE_SUPERVISOR) { printf("SVC\n"); }

    return 0;
}
