#ifndef _ARM_ISA_H_
#define _ARM_ISA_H_

#include "types.h"
#include "ARM.h"

#define LSL(a, b) (a << b)
#define LSR(a, b) (a >> b)
#define ASR(a, b) (WORD)((signed int)a >> b)
#define ROR(a, b) (a >> b) | (a << (32 - b))

enum ARMISA_Cond {
	EQ = 0,
	NE = 1,
	CS = 2,
	CC = 3,
	MI = 4,
	PL = 5,
	VS = 6,
	VC = 7,
	HI = 8,
	LS = 9,
	GE = 10,
	LT = 11,
	GT = 12,
	LE = 13,
	AL = 14,
	NV = 15
};
char *ARMISA_cond2string(enum ARMISA_Cond cond);

enum ARMISA_DataProc_Opcode {
	DP_AND = 0,
	DP_EOR = 1,
	DP_SUB = 2,
	DP_RSB = 3,
	DP_ADD = 4,
	DP_ADC = 5,
	DP_SBC = 6,
	DP_RSC = 7,
	DP_TST = 8,
	DP_TEQ = 9,
	DP_CMP = 10,
	DP_CMN = 11,
	DP_ORR = 12,
	DP_MOV = 13,
	DP_BIC = 14,
	DP_MVN = 15
};
char *ARMISA_data_proc_opcode2mnemonic(enum ARMISA_DataProc_Opcode opcode);

enum ARMISA_InstrType {
	InstrType_Data_Proc = 0,
	InstrType_PSR_Trans = 1,
	InstrType_Multiply = 2,
	InstrType_Data_Swap = 3,
	InstrType_Data_Trans = 4,
	InstrType_Undefined = 5,
	InstrType_Block_Data_Trans = 6,
	InstrType_Branch = 7,
	InstrType_Coproc_Data_Trans = 8,
	InstrType_Coproc_Data_Op = 9,
	InstrType_Coproc_Reg_Trans = 10,
	InstrType_SWI = 10
};

enum ARMISA_ShiftType {
	ShiftType_Logical_Left = 0,
	ShiftType_Logical_Right = 1,
	ShiftType_Arithmetic_Right = 2,
	ShiftType_Rotate_Right = 3
};

enum ARMISA_ShiftSrcType {
	ShiftSrcType_Rs = 0,
	ShiftSrcType_Immediate = 1
};

enum ARMISA_OperandType {
	OperandType_Register = 0,
	OperandType_Immediate = 1
};

typedef struct ARMISA_ImmShift {
	BYTE Rm:4;		// Should be accessed through ARMISA_RegOperand.Rm, just here for bit offset
	BYTE c0:1;		// Constant: Must be 0
	BYTE type:2;	// Shift type (0b00=Logical left; 0b01=Logical right;
					//			   0b10=Arithmetic right; 0b11=Rotate right)
	BYTE amount:5;	// Shift amount
} ARMISA_ImmShift;

typedef struct ARMISA_RegShift {
	BYTE Rm:4;
	BYTE c0:1;		// Constant: Must be 1
	BYTE type:2;	// Shift type (described above)
	BYTE c1:1;		// Constant: Must be 0
	BYTE Rs:4;		// Register containing shift amount in the bottom byte
} ARMISA_RegShift;

typedef union ARMISA_Shift {
	ARMISA_ImmShift	imm;	// Shift amount
	ARMISA_RegShift	reg;	// Shift register
	BYTE val;
} ARMISA_Shift;

typedef union ARMISA_RegOperand {
	WORD Rm:4;			// Operand register
	ARMISA_Shift shift;	// Shift applied to Rm
} ARMISA_RegOperand;

typedef struct ARMISA_ImmOperand {
	WORD val:8;		// Immediate value
	WORD rotate:4;	// Rotation applied to the value
} ARMISA_ImmOperand;

typedef union ARMISA_Operand {
	ARMISA_RegOperand reg;	// Operand is a register
	ARMISA_ImmOperand imm;	// Operand is an immediate value
} ARMISA_Operand;

typedef struct ARMISA_DataProc {
	WORD op2:12;	// 2nd operand
	WORD Rd:4;
	WORD Rn:4;
	WORD S:1;			// Set condition codes (0=No, 1=Yes)
	WORD op:4;
	WORD I:1;			// Immediate operand (0=Operand 2 is a register, 1=Operand 2 is an immediate value)
	WORD c0:2;			// Constant: Must be 0
	WORD cond:4;
} ARMISA_DataProc;

typedef struct ARMISA_Branch {
	WORD off:24;	// PC-relative offset
	WORD L:1;		// Link bit (0=Branch, 1=Branch with link)
	WORD c0:3;		// Constant: Must be 0b101
	WORD cond:4;
} ARMISA_Branch;

typedef struct ARMISA_Multiply {
	WORD Rm:4;
	WORD c0:4;
	WORD Rs:4;
	WORD Rn:4;
	WORD Rd:4;
	WORD S:1;
	WORD A:1;
	WORD c1:6;
	WORD cond:4;
} ARMISA_Multiply;

typedef union ARMISA_Instr {
	ARMISA_DataProc data_proc;
	ARMISA_Branch branch;
	ARMISA_Multiply mult;
} ARMISA_Instr;

// Instruction info for disassembling
typedef struct ARMISA_OperandInfo {
	enum ARMISA_OperandType	type;
	enum ARMISA_ShiftType shift_type;
	enum ARMISA_ShiftSrcType shift_src_type;
	BYTE shift_src;
	BYTE value;	// Unshifted value
} ARMISA_OperandInfo;

typedef struct ARMISA_InstrInfo {
	enum ARMISA_InstrType type;	
	char *mnemonic;
	char cond[3];
	int Rd;
	int Rm;
	int Rn;
	int Rs;
	int S;						// Set condition codes (see ARMISA_DataProc)
	int32_t offset;				// PC-relative offset (see ARMISA_Branch)
	ARMISA_OperandInfo op2;
	int lookup_index;
} ARMISA_InstrInfo;

typedef int (*cycleFunc)(ARM *cpu, ARMISA_InstrInfo *info);

cycleFunc ARMISA_getInstrFunc(int thumb, WORD instr);
ARMISA_InstrInfo *ARMISA_getInstrInfo(int thumb, WORD instr);
char *ARMISA_disasm(int thumb, WORD instr);

#endif
