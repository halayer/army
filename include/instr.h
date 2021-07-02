#ifndef _ARM_INSTR_H_
#define _ARM_INSTR_H_

#include "ARM.h"

// Instructions
int ARMISA_AND(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_EOR(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_SUB(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_RSB(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_ADD(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_ADC(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_SBC(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_RSC(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_TST(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_TEQ(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_CMP(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_CMN(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_ORR(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_MOV(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_BIC(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_MVN(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_B(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_BL(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_MUL(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_MLA(ARM *cpu, ARMISA_InstrInfo *info);
int ARMISA_UMAAL(ARM *cpu, ARMISA_InstrInfo *info);

enum ARMISA_mnemonic {
	AND, EOR, SUB, RSB, ADD, ADC, SBC, RSC,	TST, TEQ,
	CMP, CMN, ORR, MOV, BIC, MVN, B,   BL,  MUL, MLA,
	UMAAL
};

cycleFunc instr_func_lookup[] = {
	ARMISA_AND, ARMISA_EOR, ARMISA_SUB, ARMISA_RSB, ARMISA_ADD,
	ARMISA_ADC, ARMISA_SBC, ARMISA_RSC, ARMISA_TST, ARMISA_TEQ,
	ARMISA_CMP, ARMISA_CMN, ARMISA_ORR, ARMISA_MOV, ARMISA_BIC,
	ARMISA_MVN, ARMISA_B,	ARMISA_BL,	ARMISA_MUL,	ARMISA_MLA,
	ARMISA_UMAAL
};

char *mnemonic_lookup[] = {
	"AND", "EOR", "SUB", "RSB", "ADD", "ADC", "SBC", "RSC", "TST", "TEQ",
	"CMP", "CMN", "ORR", "MOV", "BIC", "MVN", "B",	 "BL",	"MUL", "MLA",
	"UMAAL"
};

#endif