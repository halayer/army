#include "ARM.h"
#include "isa.h"

// Instructions
#define ALU_UPDATE_FLAGS \
	if (info->S) { ARM_setFlag(cpu, FLAG_N, res & 0x80000000); \
				   ARM_setFlag(cpu, FLAG_Z, res == 0); }
#define SHIFT switch (info->op2.shift_type) { \
		case ShiftType_Logical_Left: \
			Op2 = LSL(info->op2.value, info->op2.shift_src); break; \
		case ShiftType_Logical_Right: \
			Op2 = LSR(info->op2.value, info->op2.shift_src); break; \
		case ShiftType_Arithmetic_Right: \
			Op2 = ASR(info->op2.value, info->op2.shift_src); break; \
		case ShiftType_Rotate_Right: \
			Op2 = ROR(info->op2.value, info->op2.shift_src); break; \
	}
#define DP_LOGICAL(s) \
	WORD Op2; \
	WORD Rd = info->Rd; \
	WORD Rn = info->Rn; \
	cpu->instr_cycles = 1; \
	if (info->op2.shift_src_type == ShiftSrcType_Rs) cpu->instr_cycles++; \
	if (Rd == 15) cpu->instr_cycles += 2; \
	if (info->op2.type == OperandType_Register) { \
		if (info->op2.shift_src_type == ShiftSrcType_Rs) \
			info->op2.shift_src = cpu->r[info->Rs]; \
		SHIFT \
	} \
	else { Op2 = ROR(info->op2.value, (info->op2.shift_src << 1)); }; \
	uint64_t res = s; \
	ALU_UPDATE_FLAGS;
#define DP_ARITHMETIC(s) \
	WORD Op2; \
	WORD Rd = info->Rd; \
	WORD Rn = info->Rn; \
	cpu->instr_cycles = 1; \
	if (info->op2.shift_src_type == ShiftSrcType_Rs) cpu->instr_cycles++; \
	if (Rd == 15) cpu->instr_cycles += 2; \
	if (info->op2.type == OperandType_Register) { Op2 = cpu->r[info->op2.value]; } \
	else { Op2 = ROR(info->op2.value, (info->op2.shift_src << 1)); }; \
	uint64_t res = s; \
	if (info->S) { ARM_setFlag(cpu, FLAG_C, (res > 0xFFFFFFFF)); \
				   ARM_setFlag(cpu, FLAG_V, ((int64_t)res) < 0); } \
	ALU_UPDATE_FLAGS;
	
// Data processing
int ARMISA_MOV(ARM *cpu, ARMISA_InstrInfo *info) {
	DP_LOGICAL(cpu->r[Rd] = Op2)
}

int ARMISA_MVN(ARM *cpu, ARMISA_InstrInfo *info) {
	DP_LOGICAL(cpu->r[Rd] = ~Op2)
}

int ARMISA_ORR(ARM *cpu, ARMISA_InstrInfo *info) {
	DP_LOGICAL(cpu->r[Rd] = cpu->r[Rn] | Op2)
}

int ARMISA_EOR(ARM *cpu, ARMISA_InstrInfo *info) {
	DP_LOGICAL(cpu->r[Rd] = cpu->r[Rn] ^ Op2)
}

int ARMISA_AND(ARM *cpu, ARMISA_InstrInfo *info) {
	DP_LOGICAL(cpu->r[Rd] = cpu->r[Rn] & Op2)
}

int ARMISA_BIC(ARM *cpu, ARMISA_InstrInfo *info) {
	DP_LOGICAL(cpu->r[Rd] = cpu->r[Rn] & (~Op2))
}

int ARMISA_TST(ARM *cpu, ARMISA_InstrInfo *info) {
	if (!info->S) return 0;
	DP_LOGICAL(cpu->r[Rn] & Op2)
}

int ARMISA_TEQ(ARM *cpu, ARMISA_InstrInfo *info) {
	if (!info->S) return 0;
	DP_LOGICAL(cpu->r[Rn] ^ Op2)
}

int ARMISA_ADD(ARM *cpu, ARMISA_InstrInfo *info) {
	DP_ARITHMETIC(cpu->r[Rd] = cpu->r[Rn] + Op2)
}

int ARMISA_ADC(ARM *cpu, ARMISA_InstrInfo *info) {
	DP_ARITHMETIC(cpu->r[Rd] = cpu->r[Rn] + Op2 + ((cpu->cpsr >> 29) & 1))
}

int ARMISA_SUB(ARM *cpu, ARMISA_InstrInfo *info) {
	DP_ARITHMETIC(cpu->r[Rd] = cpu->r[Rn] - Op2)
}

int ARMISA_SBC(ARM *cpu, ARMISA_InstrInfo *info) {
	DP_ARITHMETIC(cpu->r[Rd] = cpu->r[Rn] - Op2 - ((cpu->cpsr >> 29) & 1))
}

int ARMISA_RSB(ARM *cpu, ARMISA_InstrInfo *info) {
	DP_ARITHMETIC(cpu->r[Rd] = Op2 - cpu->r[Rn])
}

int ARMISA_RSC(ARM *cpu, ARMISA_InstrInfo *info) {
	DP_ARITHMETIC(cpu->r[Rd] = Op2 - cpu->r[Rn] - ((cpu->cpsr >> 29) & 1))
}

int ARMISA_CMP(ARM *cpu, ARMISA_InstrInfo *info) {
	if (!info->S) return 0;
	DP_ARITHMETIC(cpu->r[Rd] - Op2)
}

int ARMISA_CMN(ARM *cpu, ARMISA_InstrInfo *info) {
	if (!info->S) return 0;
	DP_ARITHMETIC(cpu->r[Rd] + Op2)
}

// Branch
int ARMISA_B(ARM *cpu, ARMISA_InstrInfo *info) {
	cpu->instr_cycles = 1;
	
	cpu->r[15] += info->offset;
	
	ARM_flushPipeline(cpu);
}

int ARMISA_BL(ARM *cpu, ARMISA_InstrInfo *info) {
	cpu->instr_cycles = 1;
	
	cpu->r[14] = cpu->r[15] - ((cpu->cpsr & FLAG_T) ? 2 : 4);
	cpu->r[15] += info->offset;
	
	ARM_flushPipeline(cpu);
}

// Multiply
int ARMISA_MUL(ARM *cpu, ARMISA_InstrInfo *info) {
	int m = 0;
	WORD res = cpu->r[info->Rd] = cpu->r[info->Rm] * cpu->r[info->Rs];
	
	if (info->S) {
		ARM_setFlag(cpu, FLAG_Z, res == 0);
		ARM_setFlag(cpu, FLAG_N, res & 0x80000000);
	}
	
	if ((res >> 8 == 0xFFFFFF) || (res >> 8 == 0)) { m = 1; }
	else if ((res >> 16 == 0xFFFF) || (res >> 16 == 0)) { m = 2; }
	else if ((res >> 24 == 0xFF) || (res >> 24 == 0)) { m = 3; }
	else { m = 4; }
	
	cpu->instr_cycles = 1 + m;
}

int ARMISA_MLA(ARM *cpu, ARMISA_InstrInfo *info) {
	int m = 0;
	WORD res = cpu->r[info->Rd] = cpu->r[info->Rm] * cpu->r[info->Rs] + cpu->r[info->Rn];
	
	if (info->S) {
		ARM_setFlag(cpu, FLAG_Z, res == 0);
		ARM_setFlag(cpu, FLAG_N, res & 0x80000000);
	}
	
	if ((res >> 8 == 0xFFFFFF) || (res >> 8 == 0)) { m = 1; }
	else if ((res >> 16 == 0xFFFF) || (res >> 16 == 0)) { m = 2; }
	else if ((res >> 24 == 0xFF) || (res >> 24 == 0)) { m = 3; }
	else { m = 4; }
	
	cpu->instr_cycles = 2 + m;
}

int ARMISA_UMAAL(ARM *cpu, ARMISA_InstrInfo *info) {
	cpu->instr_cycles = 1;
}