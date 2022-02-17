#include "ARM.h"
#include "isa.h"

// Instructions
#define UPDATE_FLAGS \
    if (info->S) { ARM_setFlag(cpu, FLAG_N, res & 0x80000000); \
                   ARM_setFlag(cpu, FLAG_Z, res == 0); }
#define SHIFT switch (info->op2.shift_type) { \
        case ShiftType_Logical_Left: \
            Op2 = LSL(Op2, info->op2.shift_src); break; \
        case ShiftType_Logical_Right: \
            Op2 = LSR(Op2, info->op2.shift_src); break; \
        case ShiftType_Arithmetic_Right: \
            Op2 = ASR(Op2, info->op2.shift_src); break; \
        case ShiftType_Rotate_Right: \
            Op2 = ROR(Op2, info->op2.shift_src); break; \
    }
#define DP_LOGICAL(s) \
    WORD Op2 = info->op2.value; \
    if (info->op2.type == OperandType_Register) Op2 = cpu->r[Op2]; \
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
    uint64_t res = s; \
    UPDATE_FLAGS;
    //else { Op2 = ROR(info->op2.value, (info->op2.shift_src << 1)); };
#define DP_ARITHMETIC(s) \
    WORD Op2 = info->op2.value; \
    if (info->op2.type == OperandType_Register) Op2 = cpu->r[Op2]; \
    WORD Rd = info->Rd; \
    WORD Rn = info->Rn; \
    cpu->instr_cycles = 1; \
    if (info->op2.shift_src_type == ShiftSrcType_Rs) cpu->instr_cycles++; \
    if (Rd == 15) cpu->instr_cycles += 2; \
    if (info->op2.type == OperandType_Register) { Op2 = cpu->r[info->op2.value]; } \
    uint64_t res = s; \
    if (info->S) { ARM_setFlag(cpu, FLAG_C, (res > 0xFFFFFFFF)); \
                   ARM_setFlag(cpu, FLAG_V, ((int64_t)res) < 0); } \
    UPDATE_FLAGS;
    //else { Op2 = ROR(info->op2.value, (info->op2.shift_src << 1)); };
    
// Data processing
int ARMISA_MOV(ARM *cpu, ARMISA_InstrInfo *info) {
    if (info->Rd == 15 && info->op2.value == 14) {
        printf("%d\n", cpu->r[14]);
    }
    DP_LOGICAL(cpu->r[Rd] = Op2)
    if (info->Rd == 15) { ARM_flushPipeline(cpu); }
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
    cpu->instr_cycles = 3;
    
    cpu->r[15] += info->offset;
    
    ARM_flushPipeline(cpu);
}

int ARMISA_BL(ARM *cpu, ARMISA_InstrInfo *info) {
    cpu->instr_cycles = 3;
    
    cpu->r[14] = cpu->r[15] - ((cpu->cpsr & FLAG_T) ? 2 : 4);
    cpu->r[15] += info->offset;
    
    ARM_flushPipeline(cpu);
}

int ARMISA_BX(ARM *cpu, ARMISA_InstrInfo *info) {
    cpu->instr_cycles = 3;
    
    cpu->r[15] = cpu->r[info->Rn];
    ARM_setFlag(cpu, FLAG_T, cpu->r[15] & 1);
    
    ARM_flushPipeline(cpu);
}

int ARMISA_BLX_reg(ARM *cpu, ARMISA_InstrInfo *info) {
    cpu->instr_cycles = 3;
    
    cpu->r[14] = cpu->r[15] - ((cpu->cpsr & FLAG_T) ? 2 : 4);
    cpu->r[15] = cpu->r[info->Rn];
    ARM_setFlag(cpu, FLAG_T,  cpu->r[15] & 1);
    
    ARM_flushPipeline(cpu);
}

int ARMISA_BLX_imm(ARM *cpu, ARMISA_InstrInfo *info) {
    cpu->instr_cycles = 3;
    
    cpu->r[14] = cpu->r[15] - ((cpu->cpsr & FLAG_T) ? 2 : 4);
    cpu->r[15] += info->offset;
    ARM_setFlag(cpu, FLAG_T, ~(cpu->r[15] & 1));    // Switch Instruction Set (ARM => Thumb; Thumb => ARM)
    
    ARM_flushPipeline(cpu);
}

// Multiply
#define MUL_DET_CYCLES \
    if ((res >> 8 == 0xFFFFFF) || (res >> 8 == 0)) { m = 1; } \
    else if ((res >> 16 == 0xFFFF) || (res >> 16 == 0)) { m = 2; } \
    else if ((res >> 24 == 0xFF) || (res >> 24 == 0)) { m = 3; } \
    else { m = 4; }
#define MUL_R15_CHECK \
    if ((info->Rd == 15) || (info->Rm == 15) || (info->Rs == 15) || \
        (info->Rn == 15)) { \
        ARM_undefined(cpu, "r15 must not be used as a register"); \
        return -1; \
    }

int ARMISA_MUL(ARM *cpu, ARMISA_InstrInfo *info) {
    if (info->Rd == info->Rm) {
        ARM_undefined(cpu, "Rd == Rm");
        return -1;
    }
    
    if ((info->Rd == 15) || (info->Rm == 15) || (info->Rs == 15)) {
        ARM_undefined(cpu, "r15 must not be used as a register");
        return -1;
    }
    
    int m = 0;
    WORD res = cpu->r[info->Rd] = cpu->r[info->Rm] * cpu->r[info->Rs];
    
    MUL_DET_CYCLES
    UPDATE_FLAGS
    
    cpu->instr_cycles = 1 + m;
}

int ARMISA_MLA(ARM *cpu, ARMISA_InstrInfo *info) {
    if (info->Rd == info->Rm) {
        ARM_undefined(cpu, "Rd == Rm");
        return -1;
    }
    MUL_R15_CHECK
    
    int m = 0;
    WORD res = cpu->r[info->Rd] = cpu->r[info->Rm] * cpu->r[info->Rs] + cpu->r[info->Rn];
    
    MUL_DET_CYCLES
    UPDATE_FLAGS
    
    cpu->instr_cycles = 2 + m;
}

int ARMISA_UMULL(ARM *cpu, ARMISA_InstrInfo *info) {
    MUL_R15_CHECK
    
    uint64_t res = cpu->r[info->Rm] * cpu->r[info->Rs];
    cpu->r[info->Rn] = res & 0xFFFFFFFF;
    cpu->r[info->Rd] = res >> 32;
    
    int m = 0;
    if (res >> 8 == 0) { m = 1; }
    else if (res >> 16 == 0) { m = 2; } \
    else if (res >> 24 == 0) { m = 3; } \
    else { m = 4; }
    cpu->instr_cycles = 2 + m;
}

int ARMISA_SMULL(ARM *cpu, ARMISA_InstrInfo *info) {
    MUL_R15_CHECK
    
    int64_t res = (int32_t)cpu->r[info->Rm] * (int32_t)cpu->r[info->Rs];
    cpu->r[info->Rn] = ((uint64_t)res) & 0xFFFFFFFF;
    cpu->r[info->Rd] = ((uint64_t)res) >> 32;
    
    int m = 0;
    MUL_DET_CYCLES
    cpu->instr_cycles = 2 + m;
}

int ARMISA_UMLAL(ARM *cpu, ARMISA_InstrInfo *info) {
    MUL_R15_CHECK
    
    uint64_t res = cpu->r[info->Rm] * cpu->r[info->Rs] + \
        (uint64_t)(cpu->r[info->Rn] | ((uint64_t)cpu->r[info->Rd] << 32));
    cpu->r[info->Rn] = res & 0xFFFFFFFF;
    cpu->r[info->Rd] = res >> 32;
    
    int m = 0;
    if (res >> 8 == 0) { m = 1; }
    else if (res >> 16 == 0) { m = 2; } \
    else if (res >> 24 == 0) { m = 3; } \
    else { m = 4; }
    cpu->instr_cycles = 3 + m;
}

int ARMISA_SMLAL(ARM *cpu, ARMISA_InstrInfo *info) {
    MUL_R15_CHECK
    
    int64_t res = (int32_t)cpu->r[info->Rm] * (int32_t)cpu->r[info->Rs] + \
        (int64_t)(cpu->r[info->Rn] | ((int64_t)cpu->r[info->Rd] << 32));
    cpu->r[info->Rn] = ((uint64_t)res) & 0xFFFFFFFF;
    cpu->r[info->Rd] = ((uint64_t)res) >> 32;
    
    int m = 0;
    MUL_DET_CYCLES
    cpu->instr_cycles = 3 + m;
}

int ARMISA_LDR(ARM *cpu, ARMISA_InstrInfo *info) {
    int Op2;
    
    if (info->op2.type == OperandType_Immediate) {
        Op2 = cpu->r[info->Rn] + info->op2.value;
        if (!info->U) { Op2 = Op2 - 2*info->op2.value; }
    } else {
        info->op2.value = cpu->r[info->Rn] + cpu->r[info->Rm];
        if (!info->U) { Op2 = Op2 - 2*cpu->r[info->Rm]; }
        
        if (!info->op2.shift_src == 0) { SHIFT }
    }
    
    if (info->P) {  // Pre-indexed
        if (info->B) { Bus_read(cpu->bus, Op2, 4, &cpu->r[info->Rd]); }
        else { Bus_read(cpu->bus, Op2, 1, &cpu->r[info->Rd]); }
        
        if (info->W) { cpu->r[info->Rn] = Op2; }
    } else {    // Post-indexed
        if (info->B) { Bus_read(cpu->bus, cpu->r[info->Rn], 4, &cpu->r[info->Rd]); }
        else { Bus_read(cpu->bus, cpu->r[info->Rn], 1, &cpu->r[info->Rd]); }
        cpu->r[info->Rn] = Op2;
    }
}
int ARMISA_STR(ARM *cpu, ARMISA_InstrInfo *info) {
    WORD Op2;

    if (info->op2.type == OperandType_Immediate) {
        Op2 = cpu->r[info->Rn] + info->op2.value;
        if (!info->U) { Op2 = Op2 - 2*info->op2.value; }
    } else {
        info->op2.value = cpu->r[info->Rn] + cpu->r[info->Rm];
        if (!info->U) { Op2 = Op2 - 2*cpu->r[info->Rm]; }
        
        if (!info->op2.shift_src == 0) { SHIFT }
    }

    if (info->P) {  // Pre-indexed
        if (!info->B) { Bus_write(cpu->bus, Op2, 4, &cpu->r[info->Rd]); }
        else { Bus_write(cpu->bus, Op2, 1, &cpu->r[info->Rd]); }
        
        if (info->W) { cpu->r[info->Rn] = Op2; }
    } else {    // Post-indexed
        if (!info->B) { Bus_write(cpu->bus, cpu->r[info->Rn], 4, &cpu->r[info->Rd]); }
        else { Bus_write(cpu->bus, cpu->r[info->Rn], 1, &cpu->r[info->Rd]); }
        
        cpu->r[info->Rn] = Op2;
    }
}
