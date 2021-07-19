#ifndef _ARM_H_
#define _ARM_H_

#include <stdio.h>

#include "types.h"
#include "bus.h"

// Status Register
#define FLAG_N (1 << 31)	// Negative ALU result
#define FLAG_Z (1 << 30)	// Zero result from ALU
#define FLAG_C (1 << 29)	// ALU operation Carried out
#define FLAG_V (1 << 28)	// ALU operation oVerflowed
#define FLAG_I (1 << 7)		// Interrupts disabled
#define FLAG_F (1 << 6)		// Fast interrupts disabled
#define FLAG_T (1 << 5)		// Thumb State
#define M 0x1F				// Mode bits

// Modes
#define MODE_USER       0x10
#define MODE_FIQ        0x11
#define MODE_IRQ        0x12
#define MODE_SUPERVISOR 0x13
#define MODE_ABORT      0x17
#define MODE_UNDEFINED  0x1B
#define MODE_SYSTEM		0x1F

// Architecture
#define ARCH_ARM9	0
#define ARCH_ARM7	1

typedef struct ARM {
	Bus *bus;
	
	struct ARM_Pipeline {
		int size;
		int flushed;
	} pipeline;
	
	int arch;
	FILE *debug;
	
	WORD r[16]; // System/user registers
	WORD cpsr;
	
	// Banked registers
	WORD r13_svc, r14_svc, spsr_svc; // Supervisor
	WORD r13_abt, r14_abt, spsr_abt; // Abort
	WORD r13_und, r14_und, spsr_und; // Undefined
	WORD r13_irq, r14_irq, spsr_irq; // Interrupt
	WORD r8_fiq, r9_fiq, r10_fiq, r11_fiq, r12_fiq, r13_fiq, r14_fiq, spsr_fiq; // Fast interrupt
	
	WORD vec_base;		// Vector table address
	
	WORD instr;				// Current instruction
	int instr_cycles;		// Amount of clock cycles the current instruction takes
	int current_cycle;		// Current instruction clock cycle
	int total_cycle;		// Total clock cycle
	WORD next_instr_addr;	// Address of the next instruction
	WORD next_instr;		// Next instruction
} ARM;

int dbg_cycle(ARM *cpu);

ARM *ARM_new(int arch);
int ARM_checkCondition(ARM *cpu, int cond);
int ARM_cycle(ARM *cpu);
int ARM_step(ARM *cpu);
void ARM_switchMode(ARM *cpu, int new_mode);
int ARM_getMode(ARM *cpu);
void ARM_registerDump(ARM *cpu);
int ARM_prefetch(ARM *cpu);
int ARM_fetch(ARM *cpu);
void ARM_flushPipeline(ARM *cpu);
int ARM_getFlag(ARM *cpu, int flag_mask);
void ARM_setFlag(ARM *cpu, int flag_mask, int val);

// Exceptions
void ARM_reset(ARM *cpu);
void ARM_undefined(ARM *cpu, char *why);
void ARM_SWI(ARM *cpu);
void ARM_prefAbort(ARM *cpu, char *why);
void ARM_dataAbort(ARM *cpu, char *why);
void ARM_IRQ(ARM *cpu);
void ARM_FIQ(ARM *cpu);

#endif
