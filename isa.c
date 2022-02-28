#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "isa.h"
#include "instr.h"

char *ARMISA_cond2string(enum ARMISA_Cond cond) {
    char *strings[] = {"EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC", "HI", "LS", "GE", "LT", "GT", "LE", "AL", "NV"};
    return strings[cond];
}

cycleFunc ARMISA_getInstrFunc(ARM *cpu, WORD instr) {
    ARMISA_InstrInfo *info = ARMISA_getInstrInfo(cpu, instr);
    if (!info) return NULL;

    return instr_func_lookup[info->lookup_index];
}

ARMISA_InstrInfo *ARMISA_getInstrInfo(ARM *cpu, WORD instr) {
    ARMISA_Instr *instr_data = (ARMISA_Instr *)&instr;
    ARMISA_ImmOperand *imm_op = (ARMISA_ImmOperand *)&instr;
    ARMISA_RegOperand *reg_op = (ARMISA_RegOperand *)&instr;
    ARMISA_InstrInfo *ret = malloc(sizeof(ARMISA_InstrInfo));
    memset((void *)ret, 0, sizeof(ARMISA_InstrInfo)); ret->instr = instr;
    strcpy((char *)&ret->cond, ARMISA_cond2string(instr_data->data_proc.cond));

    if (!(cpu->cpsr & FLAG_T)) {
        // Identify instruction type
        if (instr_data->mult.c0 == 9 && instr_data->mult.c1 == 0) { // Multiply
            ret->type = InstrType_Multiply;
            ret->Rm = instr_data->mult.Rm;
            ret->Rs = instr_data->mult.Rs;
            ret->Rn = instr_data->mult.Rn;
            ret->Rd = instr_data->mult.Rd;
            ret->S = instr_data->mult.S;
            
            if (!instr_data->mult.L) {
                ret->lookup_index = (instr_data->mult.A) ? MLA : MUL;
                return ret;
            }
            
            if (instr_data->mult.A) {
                ret->lookup_index = (instr_data->mult.U) ? SMLAL : UMLAL;
            } else {
                ret->lookup_index = (instr_data->mult.U) ? SMULL : UMULL;
            }
            
            return ret;
        }
        
        if (instr_data->data_proc.c0 == 0) {	// Data processing instruction
            // Data processing instruction
            ret->lookup_index = instr_data->data_proc.op;
            ret->type = InstrType_Data_Proc;
            ret->Rd = instr_data->data_proc.Rd;
            ret->Rn = instr_data->data_proc.Rn;
            ret->S = instr_data->data_proc.S;
            
            // Identify type of second operand
            if (!instr_data->data_proc.I) {	// Register
                ret->op2.type = OperandType_Register;
                ret->op2.shift_type = reg_op->shift.imm.type;
                ret->Rm = ret->op2.value = reg_op->Rm;
                
                // Identify shift type
                if (reg_op->shift.imm.c0 == 0) {	// Shifted by immediate value
                    ret->op2.shift_src_type = ShiftSrcType_Immediate;
                    ret->op2.shift_src = reg_op->shift.imm.amount;
                } else if (reg_op->shift.reg.c0 == 1 && reg_op->shift.reg.c1 == 0) {	// Shifted by register
                    ret->op2.shift_src_type = ShiftSrcType_Rs;
                    ret->Rs = ret->op2.shift_src = reg_op->shift.reg.Rs;
                }
            } else {	// Immediate value
                ret->op2.type = OperandType_Immediate;
                /*ret->op2.shift_type = ShiftType_Rotate_Right;
                ret->op2.shift_src_type = ShiftSrcType_Immediate;*/
                ret->op2.value = ROR(imm_op->val, (imm_op->rotate << 1));   // An immediate value is always rotated right by another immediate value
            }
            
            return ret;
        }
        
        if (instr_data->branch.c0 == 5) { // Branch
            if (instr_data->branch.cond == 15) {
                if (cpu->arch != ARCH_ARM9) goto unknown;
                
                ret->lookup_index = BLX_imm;
            } else if ((instr >> 8) & 0xFFFFF == 0x12FFF) {
                switch ((instr >> 4) & 15) {
                    case 1: ret->lookup_index = BX; break;
                    //case 2: ret->lookup_index = BJX; break;       To be implemented among Jazelle bytecode execution
                    case 3: ret->lookup_index = BLX_reg; break;
                }
            } else {
                ret->lookup_index = instr_data->branch.L ? BL : B;
            }
            
            ret->type = InstrType_Branch;
            ret->offset = ((instr_data->branch.off ^ (1<<23)) - (1<<23)) << 2; // Converting to signed integer, then multiplying by 4
            
            return ret;
        }
        
        if (instr_data->data_trns.c0 == 1) { // Single Data Transfer
            ret->type = InstrType_Data_Trans;
            ret->lookup_index = instr_data->data_trns.L ? LDR : STR;
            ret->Rd = instr_data->data_trns.Rd;
            ret->Rn = instr_data->data_trns.Rn;
            ret->W = instr_data->data_trns.W;
            ret->B = instr_data->data_trns.B;
            ret->U = instr_data->data_trns.U;
            ret->P = instr_data->data_trns.P;
            
            // Identify type of offset
            if (instr_data->data_trns.I) {	// Register
                ret->op2.type = OperandType_Register;
                ret->op2.shift_type = reg_op->shift.imm.type;
                ret->Rm = ret->op2.value = reg_op->Rm;
                
                ret->op2.shift_src_type = ShiftSrcType_Immediate;
                ret->op2.shift_type = (instr >> 5) & 0x3;
                ret->op2.shift_src = (instr >> 7) & 0x1F;
            } else {
                ret->op2.type = OperandType_Immediate;
                ret->op2.value = instr & 0xFFF;
            }
            
            return ret;
        }
        
        if (instr_data->swi.c0 == 15) {
            ret->type = InstrType_SWI;
            ret->lookup_index = SWI;
            ret->swi_num = instr_data->swi.ignored;
            
            return ret;
        }
    }

    unknown:
    free(ret);
    return NULL;
}

char *shift_type_lookup[4] = {"LSL", "LSR", "ASR", "ROR"};

char *_op2_to_string(ARMISA_InstrInfo *info) {
    char *shift_type = shift_type_lookup[info->op2.shift_type];
    char *op2 = malloc(16);
    memset(op2, 0, 16);

    if (info->type == InstrType_Data_Trans) goto data_trans;

    if (info->op2.type == OperandType_Register) {
        if (info->op2.shift_src_type == ShiftSrcType_Rs) {
            sprintf(op2, "r%d, %s r%d", info->op2.value, shift_type, info->op2.shift_src);
        } else if (info->op2.shift_src != 0) {
            sprintf(op2, "r%d, %s #%d", info->op2.value, shift_type, info->op2.shift_src);
        } else {
            sprintf(op2, "r%d", info->op2.value);
        }
    } else {
        sprintf(op2, "#0x%x", info->op2.value);
    }

    return op2;

    data_trans:
    if (info->op2.type == OperandType_Immediate && info->op2.value == 0) {
        sprintf(op2, "[r%d]", info->Rn); return op2;
    }

    if (info->P) { // Pre-indexed
        if (info->op2.type == OperandType_Immediate) {
            if (info->op2.value != 0) {
                sprintf(op2, "[r%d, #%d]%s", info->Rn, info->op2.value, (info->W) ? "!" : "");
            } else {
                sprintf(op2, "[r%d]", info->Rn);
            }
        } else if (info->op2.type == OperandType_Register) {
            if (info->op2.shift_src == 0) {
                sprintf(op2, "[r%d, %sr%d]", info->Rn, (info->U) ? "" : "-", info->Rm);
            } else {
                sprintf(op2, "[r%d, %sr%d, %s #%d]", info->Rn, (info->U) ? "" : "-", info->Rm, shift_type, info->op2.shift_src);
            }
        }
    } else { // Post-indexed
        if (info->op2.type == OperandType_Immediate) {
            sprintf(op2, "[r%d], #%d", info->Rn, info->op2.value);
        } else if (info->op2.type == OperandType_Register) {
            if (info->op2.shift_src == 0) {
                sprintf(op2, "[r%d], %sr%d", info->Rn, (info->U) ? "" : "-", info->Rm);
            } else {
                sprintf(op2, "[r%d], %sr%d, %s #%d", info->Rn, (info->U) ? "" : "-", info->Rm, shift_type, info->op2.shift_src);
            }
        }
    }

    return op2;
}

char *ARMISA_disasm(ARM *cpu, WORD instr, int pc_offset) {
    ARMISA_InstrInfo *info = ARMISA_getInstrInfo(cpu, instr);
    if (!info) return NULL;
    char *mnemonic = mnemonic_lookup[info->lookup_index];
    char *ret = malloc(64); memset((void *)ret, 0, 64);
    char *op2 = _op2_to_string(info);
    char cond[3] = "\0\0\0";
    char suffix[2] = "\0\0";

    if (strcmp((char *)&info->cond, "AL") != 0)
        strcpy((char *)&cond, (char *)&info->cond);

    if (info->S) suffix[0] = 83;	// "S"

    switch (info->lookup_index) {
        case MOV: case MVN:
            sprintf(ret, "%s%s%s r%d, %s", mnemonic, &cond, suffix, info->Rd, op2); break;
        case ORR: case EOR: case AND: case BIC: case ADD: case ADC:
        case SUB: case SBC: case RSB: case RSC:
            sprintf(ret, "%s%s%s r%d, r%d, %s", mnemonic, &cond, suffix, info->Rd, info->Rn, op2);
            break;
        case TST: case TEQ: case CMP: case CMN:
            sprintf(ret, "%s%s%s r%d, %s", mnemonic, &cond, suffix, info->Rn, &op2); break;
        case B: case BL: case BX:
            sprintf(ret, "%s%s 0x%x", mnemonic, &cond, cpu->r[15] + info->offset + pc_offset); break;
        case BLX_imm:
            sprintf(ret, "%s 0x%x", mnemonic, cpu->r[15] + info->offset + pc_offset); break;
        case MUL:
            sprintf(ret, "MUL%s%s r%d, r%d, r%d", &cond, suffix, info->Rd, info->Rm, info->Rs); break;
        case MLA:
            sprintf(ret, "MLA%s%s r%d, r%d, r%d, r%d", &cond, suffix, info->Rd, info->Rm, info->Rs, info->Rn);
            break;
        case UMULL: case SMULL: case UMLAL: case SMLAL:
            sprintf(ret, "%s%s%s r%d, r%d, r%d, r%d", mnemonic, &cond, suffix, info->Rn, info->Rd, info->Rm, info->Rs);
            break;
        case LDR: case STR:
            sprintf(ret, "%s%s%s%s r%d, %s", mnemonic, (info->B) ? "B" : "", (info->W && !info->P) ? "T" : "", &cond,
                info->Rd, op2); break;
        case SWI:
            sprintf(ret, "%s%s #0x%x", mnemonic, &cond, info->swi_num); break;
        default: break;
    }

    return ret;
}
