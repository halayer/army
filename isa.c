#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "isa.h"
#include "instr.h"

char *ARMISA_cond2string(enum ARMISA_Cond cond) {
	char *strings[] = {"EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC", "HI", "LS", "GE", "LT", "GT", "LE", "AL", "NV"};
	return strings[cond];
}

char *ARMISA_data_proc_opcode2mnemonic(enum ARMISA_DataProc_Opcode opcode) {
	char *strings[] = {"AND", "EOR", "SUB", "RSB", "ADD", "ADC", "SBC", "RSC", "TST", "TEQ", "CMP", "CMN", "ORR", "MOV", "BIC", "MVN"};
	return strings[opcode];
}

int ARMISA_mnemonic2data_proc_opcode(char *mnemonic) {
	char *strings[] = {"AND", "EOR", "SUB", "RSB", "ADD", "ADC", "SBC", "RSC", "TST", "TEQ", "CMP", "CMN", "ORR", "MOV", "BIC", "MVN"};
	
	for (int i = 0; i < 16; i++) {
		if (strcmp(strings[i], mnemonic))
			return i;
	}
	
	return -1;
}

cycleFunc ARMISA_getInstrFunc(int thumb, WORD instr) {
	ARMISA_InstrInfo *info = ARMISA_getInstrInfo(thumb, instr);
	
	return instr_func_lookup[info->lookup_index];
}

ARMISA_InstrInfo *ARMISA_getInstrInfo(int thumb, WORD instr) {
	ARMISA_Instr *instr_data = (ARMISA_Instr *)&instr;
	ARMISA_ImmOperand *imm_op = (ARMISA_ImmOperand *)&instr;
	ARMISA_RegOperand *reg_op = (ARMISA_RegOperand *)&instr;
	ARMISA_InstrInfo *ret = malloc(sizeof(ARMISA_InstrInfo));
	memset((void *)ret, 0, sizeof(ARMISA_InstrInfo));
	strcpy((char *)&ret->cond, ARMISA_cond2string(instr_data->data_proc.cond));
	
	if (!thumb) {
		// Identify instruction type
		if (instr_data->data_proc.c0 == 0) {	// Data processing instruction
			// Data processing instruction
			ret->lookup_index = instr_data->data_proc.op;
			ret->mnemonic = ARMISA_data_proc_opcode2mnemonic(instr_data->data_proc.op);
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
				ret->op2.type = OperandType_Immediate;				// An immediate value is
				ret->op2.shift_type = ShiftType_Rotate_Right;		// always rotated right by
				ret->op2.shift_src_type = ShiftSrcType_Immediate;	// an immediate value
				ret->op2.shift_src = imm_op->rotate;
				ret->op2.value = imm_op->val;
			}
		}
		
		if (instr_data->branch.c0 == 5) { // Branch
			ret->lookup_index = instr_data->branch.L ? BL : B;
			ret->offset = ((instr_data->branch.off ^ (1<<23)) - (1<<23)) << 2;
		}
		
		if (instr_data->mult.c0 == 9 && instr_data->mult.c1 == 0) { // Multiply
			ret->Rm = instr_data->mult.Rm;
			ret->Rs = instr_data->mult.Rs;
			ret->Rn = instr_data->mult.Rn;
			ret->Rd = instr_data->mult.Rd;
			ret->S = instr_data->mult.S;
			ret->lookup_index = (instr_data->mult.A) ? MLA : MUL;
		}
	}
	
	return ret;
}

char *ARMISA_disasm(int thumb, WORD instr) {
	ARMISA_InstrInfo *info = ARMISA_getInstrInfo(thumb, instr);
	char *ret = malloc(64); memset((void *)ret, 0, 64);
	char op2[16] = {0};
	char cond[3] = {0};
	char suffix[2] = {0};
	
	if (strcmp((char *)&info->cond, "AL") != 0)
		strcpy((char *)&cond, (char *)&info->cond);
	
	if (info->op2.type == OperandType_Register) {
		sprintf((char *)&op2, "r%d", info->op2.value);
	} else {
		sprintf((char *)&op2, "#%x", info->op2.value);
	}
	
	if (info->S) suffix[0] = 83;	// "S"
	
	if (info->lookup_index == MLA) {
		sprintf(ret, "MLA%s r%d, r%d, r%d, r%d", &cond, info->Rd, info->Rm, info->Rs, info->Rn);
		return ret;
	}
	
	switch (info->lookup_index) {
		case MOV: case MVN:
			sprintf(ret, "%s%s%s r%d, %s", info->mnemonic, &cond, suffix, info->Rd, &op2); break;
		case ORR: case EOR: case AND: case BIC: case ADD: case ADC:
		case SUB: case SBC: case RSB: case RSC:
			sprintf(ret, "%s%s%s r%d, r%d, %s", info->mnemonic, &cond, suffix, info->Rd, info->Rn, &op2);
			break;
		case TST: case TEQ: case CMP: case CMN:
			sprintf(ret, "%s%s%s r%d, %s", info->mnemonic, &cond, suffix, info->Rn, &op2); break;
		case B: case BL:
			sprintf(ret, "%s%s r15 + (%d)", info->mnemonic, &cond, info->offset); break;
		case MUL:
			sprintf(ret, "MUL%s r%d, r%d, r%d", &cond, info->Rd, info->Rm, info->Rs); break;
		case MLA:
			sprintf(ret, "MLA%s r%d, r%d, r%d, r%d", &cond, info->Rd, info->Rm, info->Rs, info->Rn);
			break;
		default: break;
	}
	
	/*if (!strcmp(info->mnemonic, "MOV") || !strcmp(info->mnemonic, "MVN")) {
		// POTENTIAL OPTIMIZATION: Merge MOV & MVN with TST & TEQ ..., and instead of
		// passing Rd, pass Rn
		sprintf(ret, "%s%s%s r%d, %s", info->mnemonic, &cond, suffix, info->Rd, &op2);
	} else if (!strcmp(info->mnemonic, "ORR") || !strcmp(info->mnemonic, "EOR") ||
		!strcmp(info->mnemonic, "AND") || !strcmp(info->mnemonic, "BIC") ||
		!strcmp(info->mnemonic, "ADD") || !strcmp(info->mnemonic, "ADC") ||
		!strcmp(info->mnemonic, "SUB") || !strcmp(info->mnemonic, "SBC") ||
		!strcmp(info->mnemonic, "RSB") || !strcmp(info->mnemonic, "RSC")) {
		sprintf(ret, "%s%s%s r%d, r%d, %s", info->mnemonic, &cond, suffix, info->Rd, info->Rn, &op2);
	} else if (!strcmp(info->mnemonic, "TST") || !strcmp(info->mnemonic, "TEQ") ||
		!strcmp(info->mnemonic, "CMP") || !strcmp(info->mnemonic, "CMN")) {
		sprintf(ret, "%s%s%s r%d, %s", info->mnemonic, &cond, suffix, info->Rn, &op2);
	} else if (!strcmp(info->mnemonic, "B") || !strcmp(info->mnemonic, "BL")) {
		sprintf(ret, "%s%s r15 + (%d)", info->mnemonic, &cond, info->offset);
	} else if (!strcmp(info->mnemonic, "MUL")) {
		sprintf(ret, "MUL%s r%d, r%d, r%d", &cond, info->Rd, info->Rm, info->Rs);
	} else if (!strcmp(info->mnemonic, "MLA")) {
		sprintf(ret, "MLA%s r%d, r%d, r%d, r%d", &cond, info->Rd, info->Rm, info->Rs, info->Rn);
	}*/
	
	return ret;
}