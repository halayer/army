#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ARM.h"
#include "bus.h"
#include "isa.h"

#define XCHG(a, b) do { int _tmp = a; a = b; b = _tmp; } while (0)
	
int dbg_cycle(ARM *cpu) {
	fprintf(cpu->debug, "DBG CYCLE\n");
	return 0;
}

ARM *ARM_new() {
	ARM *cpu = (ARM *)malloc(sizeof(ARM));
	memset((void *)cpu, 0, sizeof(ARM));
	
	cpu->pipeline.size = 3;
	
	return cpu;
}

int ARM_cycle(ARM *cpu) {
	cycleFunc f = ARMISA_getInstrFunc(cpu->cpsr & FLAG_T, cpu->instr);
	
	if (cpu->pipeline.flushed) {
		switch (cpu->current_cycle) {
			case 0:	// FETCH CYCLE
				if (ARM_fetch(cpu) != 0)	// Fetch current instruction
					return 0;
				goto ret;
			case 1:	// SECOND CYCLE
				if (ARM_prefetch(cpu) != 0)	// Prefetching next instruction
					return 0;
				goto ret;
			default:
				// Execute instruction on last cycle
				if (cpu->current_cycle == cpu->pipeline.size - 1) {
					if (cpu->debug)
						fprintf(cpu->debug, "[0x%.8x]: %s\n", cpu->r[15]-8, ARMISA_disasm(cpu->cpsr & FLAG_T, cpu->instr));
					f(cpu, ARMISA_getInstrInfo(cpu->cpsr & FLAG_T, cpu->instr));	// Execute instruction
					cpu->pipeline.flushed = 0;
					goto instr_complete;
				}
		}
	} else {
		if (cpu->current_cycle == 0) {
			if (ARM_prefetch(cpu) != 0)
				return 0;
			
			if (cpu->debug) fprintf(cpu->debug, "[0x%.8x]: %s\n", cpu->r[15]-8, ARMISA_disasm(cpu->cpsr & FLAG_T, cpu->instr));
			f(cpu, ARMISA_getInstrInfo(cpu->cpsr & FLAG_T, cpu->instr));	// Execute instruction
		}
		
		if (cpu->current_cycle == cpu->instr_cycles - 1) {
			goto instr_complete;
		}
	}
	
	ret:
	cpu->current_cycle++;
	cpu->total_cycle++;
	
	return 0;
	
	instr_complete:
	cpu->instr = cpu->next_instr;
	//cpu->r[15] = cpu->next_instr_addr;
	cpu->current_cycle = 0;
	cpu->total_cycle++;
	
	return 1;
}

int ARM_step(ARM *cpu) {
	int ret;
	
	while (1) {
		ret = ARM_cycle(cpu);
		
		if (ret > 0)
			return ret;
	}
}

void ARM_switchMode(ARM *cpu, int new_mode) {
	int old_mode = cpu->cpsr & M;
	
	if (new_mode == MODE_FIQ || ((new_mode == MODE_USER || new_mode == MODE_SYSTEM) && old_mode == MODE_FIQ)) {
		XCHG(cpu->r[8], cpu->r8_fiq);
		XCHG(cpu->r[9], cpu->r9_fiq);
		XCHG(cpu->r[10], cpu->r10_fiq);
		XCHG(cpu->r[11], cpu->r11_fiq);
		XCHG(cpu->r[12], cpu->r12_fiq);
		XCHG(cpu->r[13], cpu->r13_fiq);
		XCHG(cpu->r[14], cpu->r14_fiq);
		XCHG(cpu->cpsr, cpu->spsr_fiq);
	} else if (new_mode == MODE_SUPERVISOR || ((new_mode == MODE_USER || new_mode == MODE_SYSTEM) && old_mode == MODE_SUPERVISOR)) {
		XCHG(cpu->r[13], cpu->r13_svc);
		XCHG(cpu->r[14], cpu->r14_svc);
		XCHG(cpu->cpsr, cpu->spsr_svc);
	} else if (new_mode == MODE_ABORT || ((new_mode == MODE_USER || new_mode == MODE_SYSTEM) && old_mode == MODE_ABORT)) {
		XCHG(cpu->r[13], cpu->r13_abt);
		XCHG(cpu->r[14], cpu->r14_abt);
		XCHG(cpu->cpsr, cpu->spsr_abt);
	} else if (new_mode == MODE_IRQ || ((new_mode == MODE_USER || new_mode == MODE_SYSTEM) && old_mode == MODE_IRQ)) {
		XCHG(cpu->r[13], cpu->r13_irq);
		XCHG(cpu->r[14], cpu->r14_irq);
		XCHG(cpu->cpsr, cpu->spsr_irq);
	} else if (new_mode == MODE_UNDEFINED || ((new_mode == MODE_USER || new_mode == MODE_SYSTEM) && old_mode == MODE_UNDEFINED)) {
		XCHG(cpu->r[13], cpu->r13_und);
		XCHG(cpu->r[14], cpu->r14_und);
		XCHG(cpu->cpsr, cpu->spsr_und);
	}
	
	cpu->cpsr &= ~M;
	cpu->cpsr |= new_mode;
}

int ARM_getMode(ARM *cpu) {
	return cpu->cpsr & M;
}

void ARM_registerDump(ARM *cpu) {
	for (int i = 0; i < 8; i++) {
		fprintf(cpu->debug, "r%d:  0x%.8x\n", i, cpu->r[i]);
	}
	
	if (ARM_getMode(cpu) == MODE_FIQ) {
		fprintf(cpu->debug, "r8:  0x%.8x	r8_fiq:  0x%.8x\n", cpu->r8_fiq, cpu->r[8]);
		fprintf(cpu->debug, "r9:  0x%.8x	r9_fiq:  0x%.8x\n", cpu->r9_fiq, cpu->r[9]);
		fprintf(cpu->debug, "r10: 0x%.8x	r10_fiq: 0x%.8x\n", cpu->r10_fiq, cpu->r[10]);
		fprintf(cpu->debug, "r11: 0x%.8x	r11_fiq: 0x%.8x\n", cpu->r11_fiq, cpu->r[11]);
		fprintf(cpu->debug, "r12: 0x%.8x	r12_fiq: 0x%.8x\n", cpu->r12_fiq, cpu->r[12]);
		fprintf(cpu->debug, "r13: 0x%.8x	r13_fiq: 0x%.8x	r13_svc: 0x%.8x	r13_abt: 0x%.8x	r13_irq: 0x%.8x	r13_und: 0x%.8x\n", cpu->r13_fiq, cpu->r[13], cpu->r13_svc, cpu->r13_abt, cpu->r13_irq, cpu->r13_und);
		fprintf(cpu->debug, "r14: 0x%.8x	r14_fiq: 0x%.8x	r14_svc: 0x%.8x	r14_abt: 0x%.8x	r14_irq: 0x%.8x	r14_und: 0x%.8x\n", cpu->r14_fiq, cpu->r[14], cpu->r14_svc, cpu->r14_abt, cpu->r14_irq, cpu->r14_und);
	} else {
		fprintf(cpu->debug, "r8:  0x%.8x	r8_fiq:  0x%.8x\n", cpu->r[8], cpu->r8_fiq);
		fprintf(cpu->debug, "r9:  0x%.8x	r9_fiq:  0x%.8x\n", cpu->r[9], cpu->r9_fiq);
		fprintf(cpu->debug, "r10: 0x%.8x	r10_fiq: 0x%.8x\n", cpu->r[10], cpu->r10_fiq);
		fprintf(cpu->debug, "r11: 0x%.8x	r11_fiq: 0x%.8x\n", cpu->r[11], cpu->r11_fiq);
		fprintf(cpu->debug, "r12: 0x%.8x	r12_fiq: 0x%.8x\n", cpu->r[12], cpu->r12_fiq);
	}
	
	switch (ARM_getMode(cpu)) {
		case (MODE_USER):
		case (MODE_SYSTEM):
			fprintf(cpu->debug, "r13: 0x%.8x	r13_fiq: 0x%.8x	r13_svc: 0x%.8x	r13_abt: 0x%.8x	r13_irq: 0x%.8x	r13_und: 0x%.8x\n", cpu->r[13], cpu->r13_fiq, cpu->r13_svc, cpu->r13_abt, cpu->r13_irq, cpu->r13_und);
			fprintf(cpu->debug, "r14: 0x%.8x	r14_fiq: 0x%.8x	r14_svc: 0x%.8x	r14_abt: 0x%.8x	r14_irq: 0x%.8x	r14_und: 0x%.8x\n", cpu->r[14], cpu->r14_fiq, cpu->r14_svc, cpu->r14_abt, cpu->r14_irq, cpu->r14_und);
			break;
		case (MODE_SUPERVISOR):
			fprintf(cpu->debug, "r13: 0x%.8x	r13_fiq: 0x%.8x	r13_svc: 0x%.8x	r13_abt: 0x%.8x	r13_irq: 0x%.8x	r13_und: 0x%.8x\n", cpu->r13_svc, cpu->r13_fiq, cpu->r[13], cpu->r13_abt, cpu->r13_irq, cpu->r13_und);
			fprintf(cpu->debug, "r14: 0x%.8x	r14_fiq: 0x%.8x	r14_svc: 0x%.8x	r14_abt: 0x%.8x	r14_irq: 0x%.8x	r14_und: 0x%.8x\n", cpu->r14_svc, cpu->r14_fiq, cpu->r[14], cpu->r14_abt, cpu->r14_irq, cpu->r14_und);
			break;
		case (MODE_ABORT):
			fprintf(cpu->debug, "r13: 0x%.8x	r13_fiq: 0x%.8x	r13_svc: 0x%.8x	r13_abt: 0x%.8x	r13_irq: 0x%.8x	r13_und: 0x%.8x\n", cpu->r13_abt, cpu->r13_fiq, cpu->r13_svc, cpu->r[13], cpu->r13_irq, cpu->r13_und);
			fprintf(cpu->debug, "r14: 0x%.8x	r14_fiq: 0x%.8x	r14_svc: 0x%.8x	r14_abt: 0x%.8x	r14_irq: 0x%.8x	r14_und: 0x%.8x\n", cpu->r14_abt, cpu->r14_fiq, cpu->r14_svc, cpu->r[14], cpu->r14_irq, cpu->r14_und);
			break;
		case (MODE_IRQ):
			fprintf(cpu->debug, "r13: 0x%.8x	r13_fiq: 0x%.8x	r13_svc: 0x%.8x	r13_abt: 0x%.8x	r13_irq: 0x%.8x	r13_und: 0x%.8x\n", cpu->r13_irq, cpu->r13_fiq, cpu->r13_svc, cpu->r13_abt, cpu->r[13], cpu->r13_und);
			fprintf(cpu->debug, "r14: 0x%.8x	r14_fiq: 0x%.8x	r14_svc: 0x%.8x	r14_abt: 0x%.8x	r14_irq: 0x%.8x	r14_und: 0x%.8x\n", cpu->r14_irq, cpu->r14_fiq, cpu->r14_svc, cpu->r14_abt, cpu->r[14], cpu->r14_und);
			break;
		case (MODE_UNDEFINED):
			fprintf(cpu->debug, "r13: 0x%.8x	r13_fiq: 0x%.8x	r13_svc: 0x%.8x	r13_abt: 0x%.8x	r13_irq: 0x%.8x	r13_und: 0x%.8x\n", cpu->r13_und, cpu->r13_fiq, cpu->r13_svc, cpu->r13_abt, cpu->r13_irq, cpu->r[13]);
			fprintf(cpu->debug, "r14: 0x%.8x	r14_fiq: 0x%.8x	r14_svc: 0x%.8x	r14_abt: 0x%.8x	r14_irq: 0x%.8x	r14_und: 0x%.8x\n", cpu->r14_und, cpu->r14_fiq, cpu->r14_svc, cpu->r14_abt, cpu->r14_irq, cpu->r[14]);
			break;
		default:
			break;
	}
	
	fprintf(cpu->debug, "r15: 0x%.8x (%s)\n", cpu->r[15], ARMISA_disasm(cpu->cpsr & FLAG_T, cpu->instr));
	fprintf(cpu->debug, "CPSR: %s %s %s %s %s %s %s\n", (ARM_getFlag(cpu, FLAG_T)) ? "T" : "t",
		(ARM_getFlag(cpu, FLAG_F)) ? "F" : "f",
		(ARM_getFlag(cpu, FLAG_I)) ? "I" : "i",
		(ARM_getFlag(cpu, FLAG_V)) ? "N" : "n",
		(ARM_getFlag(cpu, FLAG_C)) ? "C" : "c",
		(ARM_getFlag(cpu, FLAG_Z)) ? "Z" : "z",
		(ARM_getFlag(cpu, FLAG_N)) ? "N" : "n");
}

int ARM_prefetch(ARM *cpu) {
	if (cpu->r[15] & 3) {
		ARM_prefAbort(cpu, "fetching from an unaligned address.");
		return -1;
	}
	
	cpu->next_instr = Bus_read(cpu->bus, cpu->r[15], NULL);
	cpu->next_instr_addr = cpu->r[15];
	
	cpu->r[15] += (cpu->cpsr & FLAG_T) ? 2 : 4;
	
	return 0;
}

int ARM_fetch(ARM *cpu) {
	if (cpu->r[15] & 3) {
		ARM_prefAbort(cpu, "fetching from an unaligned address.");
		return -1;
	}
	
	cpu->instr = Bus_read(cpu->bus, cpu->r[15], NULL);
	cpu->next_instr_addr = cpu->r[15];
	
	cpu->r[15] += (cpu->cpsr & FLAG_T) ? 2 : 4;
	
	return 0;
}

void ARM_flushPipeline(ARM *cpu) {
	cpu->pipeline.flushed = 1;
	cpu->current_cycle = 0;
}

int ARM_getFlag(ARM *cpu, int flag_mask) {
	return (cpu->cpsr & flag_mask) ? 1 : 0;
}

void ARM_setFlag(ARM *cpu, int flag_mask, int val) {
	if (val) { cpu->cpsr |= flag_mask; }
	else { cpu->cpsr &= ~flag_mask; }
}

// Exceptions
void ARM_reset(ARM *cpu) {
	cpu->r14_svc = cpu->r[15];									// Store the program counter in the link register
	ARM_switchMode(cpu, MODE_SUPERVISOR);						// Switch into supervisor mode
	cpu->cpsr &= ~FLAG_T;										// Switch into ARM state
	cpu->cpsr |= FLAG_I;										// Disable IRQs
	cpu->cpsr |= FLAG_F;										// Disable FIQs
	cpu->next_instr_addr = cpu->r[15] = cpu->vec_base + 0x00;	// Jump to the interrupt vector
	ARM_flushPipeline(cpu);
}

void ARM_undefined(ARM *cpu, char *why) {
	if (cpu->debug) fprintf(cpu->debug, "UNDEFINED because %s\n", why);
	cpu->r14_und = cpu->r[15];
	ARM_switchMode(cpu, MODE_UNDEFINED);	// Switch into undefined mode
	cpu->cpsr &= ~FLAG_T;
	cpu->cpsr |= FLAG_I;
	cpu->next_instr_addr = cpu->r[15] = cpu->vec_base + 0x04;
	ARM_flushPipeline(cpu);
}

void ARM_SWI(ARM *cpu) {
	cpu->r14_svc = cpu->r[15];
	ARM_switchMode(cpu, MODE_SUPERVISOR);	// Switch into supervisor mode
	cpu->cpsr &= ~FLAG_T;
	cpu->cpsr |= FLAG_I;
	cpu->next_instr_addr = cpu->r[15] = cpu->vec_base + 0x08;
	ARM_flushPipeline(cpu);
}

void ARM_prefAbort(ARM *cpu, char *why) {
	if (cpu->debug) fprintf(cpu->debug, "PREFETCH ABORT because %s\n", why);
	cpu->r14_abt = cpu->r[15];
	ARM_switchMode(cpu, MODE_ABORT);	// Switch into abort mode
	cpu->cpsr &= ~FLAG_T;
	cpu->cpsr |= FLAG_I;
	cpu->next_instr_addr = cpu->r[15] = cpu->vec_base + 0x0C;
	ARM_flushPipeline(cpu);
}

void ARM_dataAbort(ARM *cpu, char *why) {
	if (cpu->debug) fprintf(cpu->debug, "DATA ABORT because %s\n", why);
	cpu->r14_abt = cpu->r[15];
	ARM_switchMode(cpu, MODE_ABORT);	// Switch into abort mode
	cpu->cpsr &= ~FLAG_T;
	cpu->cpsr |= FLAG_I;
	cpu->next_instr_addr = cpu->r[15] = cpu->vec_base + 0x10;
	ARM_flushPipeline(cpu);
}

void ARM_IRQ(ARM *cpu) {
	cpu->r14_irq = cpu->r[15];
	ARM_switchMode(cpu, MODE_IRQ);	// Switch into IRQ mode
	cpu->cpsr &= ~FLAG_T;
	cpu->cpsr |= FLAG_I;
	cpu->next_instr_addr = cpu->r[15] = cpu->vec_base + 0x18;
	ARM_flushPipeline(cpu);
}

void ARM_FIQ(ARM *cpu) {
	cpu->r14_fiq = cpu->r[15];
	ARM_switchMode(cpu, MODE_FIQ);	// Switch into FIQ mode
	cpu->cpsr &= ~FLAG_T;
	cpu->cpsr |= FLAG_I;
	cpu->cpsr |= FLAG_F;
	cpu->next_instr_addr = cpu->r[15] = cpu->vec_base + 0x1C;
	ARM_flushPipeline(cpu);
}