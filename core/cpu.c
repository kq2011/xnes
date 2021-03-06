/*******************************************************
*	6502 CPU 
*******************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "def.h"
#include "memory.h"
#include "cpu.h"

/*********** macros *************/
//not implemented opcode
#define OP_FUTURE(code) {code,Future,1,OP_FUT}

/*********** declaration **********/
static unsigned char Cpu_GetFlags(void);

/*********************************
 status register
 7 6 5 4 3 2 1 0
 N V   B D I Z C
*********************************/
struct CPU{
	uint16_t PC;	//Program Counter
	char SP;		//Stack Pointer
	char A;			//Accumulator
	char X;			//Index Register X
	char Y;			//Index Register Y
	char C;			//Carry Flag
	char Z;			//Zero Flag
	char I;			//Interrupt Disable Flag
	char D;			//Decimal Mode Flag
	char B;			//Break Command Flag
	char U;			//Unused
	char V;			//Overflow Flag Flag
	char N;			//Negative Flag
	char Reserved[8];
};

static struct CPU global_cpu;

enum ADDRESS_MODE
{
	Future = 0,
	ZeroPage,
	ZeroPage_X,
	ZeroPage_Y,
	Absolute,
	Absolute_X,
	Absolute_Y,
	Indirect,
	Indirect_X,
	Indirect_Y,
	Implied,
	Accumulator,
	Immediate,
	Relative
};

static int OPBytes[] = {1,2,2,2,3,3,3,3,2,2,1,1,2,2};

struct OPCODE{
	unsigned char opcode;		//op code
	unsigned char mode;			//address mode
	unsigned char cycles;		//No. Cycles
	char name[5];
};


static const struct OPCODE __ops[256] = {
	{0x00,Implied,7,OP_BRK},
	{0x01,Indirect_X,6,OP_ORA},
	OP_FUTURE(0x02),
	OP_FUTURE(0x03),
	OP_FUTURE(0x04),
	{0x05,ZeroPage,3,OP_ORA},
	{0x06,ZeroPage,5,OP_ASL},
	OP_FUTURE(0x07),
	{0x08,Implied,3,OP_PHP},
	{0x09,Immediate,2,OP_ORA},
	{0x0a,Implied,2,OP_ASL},
	OP_FUTURE(0x0b),
	OP_FUTURE(0x0c),
	{0x0d,Absolute,4,OP_ORA},
	{0x0e,Absolute,6,OP_ASL},
	OP_FUTURE(0x0f),

	{0x10,Relative,2,OP_BPL},
	{0x11,Indirect_Y,5,OP_ORA},
	OP_FUTURE(0x12),
	OP_FUTURE(0x13),
	OP_FUTURE(0x14),
	{0x15,ZeroPage_X,4,OP_ORA},
	{0x16,ZeroPage_X,6,OP_ASL},
	OP_FUTURE(0x17),
	{0x18,Implied,2,OP_CLC},
	{0x19,Absolute_Y,4,OP_ORA},
	OP_FUTURE(0x1a),
	OP_FUTURE(0x1b),
	OP_FUTURE(0x1c),
	{0x1d,Absolute_X,4,OP_ORA},
	{0x1e,Absolute_X,7,OP_ASL},
	OP_FUTURE(0x1f),

	{0x20,Absolute,6,OP_JSR},
	{0x21,Indirect_X,6,OP_AND},
	OP_FUTURE(0x22),
	OP_FUTURE(0x23),
	{0x24,ZeroPage,3,OP_BIT},
	{0x25,ZeroPage,3,OP_AND},
	{0x26,ZeroPage,5,OP_ROL},
	OP_FUTURE(0x27),
	{0x28,Implied,4,OP_PLP},
	{0x29,Immediate,2,OP_AND},
	{0x2a,Implied,2,OP_ROL},	/*Accumulator?*/
	OP_FUTURE(0x2b),
	{0x2c,Absolute,4,OP_BIT},
	{0x2d,Absolute,2,OP_AND},
	{0x2e,Absolute,6,OP_ROL},
	OP_FUTURE(0x2f),

	{0x30,Relative,2,OP_BMI},
	{0x31,Indirect_Y,5,OP_AND},
	OP_FUTURE(0x32),
	OP_FUTURE(0x33),
	OP_FUTURE(0x34),
	{0x35,ZeroPage_X,4,OP_AND},
	{0x36,ZeroPage_X,6,OP_ROL},
	OP_FUTURE(0x37),
	{0x38,Implied,2,OP_SEC},
	{0x39,Absolute_Y,4,OP_AND},
	OP_FUTURE(0x3a),
	OP_FUTURE(0x3b),
	OP_FUTURE(0x3c),
	{0x3d,Absolute_X,4,OP_AND},
	{0x3e,Absolute_X,7,OP_ROL},
	OP_FUTURE(0x3f),

	{0x40,Implied,6,OP_RTI},
	{0x41,Indirect_X,6,OP_EOR},
	OP_FUTURE(0x42),
	OP_FUTURE(0x43),
	OP_FUTURE(0x44),
	{0x45,ZeroPage,3,OP_EOR},
	{0x46,ZeroPage,5,OP_LSR},
	OP_FUTURE(0x47),
	{0x48,Implied,3,OP_PHA},
	{0x49,Immediate,2,OP_EOR},
	{0x4a,Implied,2,OP_LSR},	/*Accumulator*/
	OP_FUTURE(0x4b),
	{0x4c,Absolute,3,OP_JMP},
	{0x4d,Absolute,4,OP_EOR},
	{0x4e,Absolute,6,OP_LSR},
	OP_FUTURE(0x4f),

	{0x50,Relative,2,OP_BVC},
	{0x51,Indirect_Y,5,OP_EOR},
	OP_FUTURE(0x52),
	OP_FUTURE(0x53),
	OP_FUTURE(0x54),
	{0x55,ZeroPage_X,4,OP_EOR},
	{0x56,ZeroPage_X,6,OP_LSR},
	OP_FUTURE(0x57),
	{0x58,Implied,2,OP_CLI},
	{0x59,Absolute_Y,4,OP_EOR},
	OP_FUTURE(0x5a),
	OP_FUTURE(0x5b),
	OP_FUTURE(0x5c),
	{0x5d,Absolute_X,4,OP_EOR},
	{0x5e,Absolute_X,7,OP_LSR},
	OP_FUTURE(0x5f),

	{0x60,Implied,6,OP_RTS},
	{0x61,Indirect_X,6,OP_ADC},
	OP_FUTURE(0x62),
	OP_FUTURE(0x63),
	OP_FUTURE(0x64),
	{0x65,ZeroPage,3,OP_ADC},
	{0x66,ZeroPage,5,OP_ROR},
	OP_FUTURE(0x67),
	{0x68,Implied,4,OP_PLA},
	{0x69,Immediate,2,OP_ADC},
	{0x6a,Implied,2,OP_ROR},		/*Accumulator*/
	OP_FUTURE(0x6b),
	{0x6c,Indirect,5,OP_JMP},
	{0x6d,Absolute,4,OP_ADC},
	{0x6e,Absolute,6,OP_ROR},
	OP_FUTURE(0x6f),

	{0x70,Relative,2,OP_BVS},
	{0x71,Indirect_Y,5,OP_ADC},
	OP_FUTURE(0x72),
	OP_FUTURE(0x73),
	OP_FUTURE(0x74),
	{0x75,ZeroPage_X,4,OP_ADC},
	{0x76,ZeroPage_X,6,OP_ROR},
	OP_FUTURE(0x77),
	{0x78,Implied,2,OP_SEI},
	{0x79,Absolute_Y,4,OP_ADC},
	OP_FUTURE(0x7a),
	OP_FUTURE(0x7b),
	OP_FUTURE(0x7c),
	{0x7d,Absolute_X,4,OP_ADC},
	{0x7e,Absolute_X,7,OP_ROR},
	OP_FUTURE(0x7f),

	OP_FUTURE(0x80),
	{0x81,Indirect_X,6,OP_STA},
	OP_FUTURE(0x82),
	OP_FUTURE(0x83),
	{0x84,ZeroPage,3,OP_STY},
	{0x85,ZeroPage,3,OP_STA},
	{0x86,ZeroPage,3,OP_STX},
	OP_FUTURE(0x87),
	{0x88,Implied,2,OP_DEY},
	OP_FUTURE(0x89),
	{0x8a,Implied,2,OP_TXA},
	OP_FUTURE(0x8b),
	{0x8c,Absolute,4,OP_STY},
	{0x8d,Absolute,4,OP_STA},
	{0x8e,Absolute,4,OP_STX},
	OP_FUTURE(0x8f),

	{0x90,Relative,2,OP_BCC},
	{0x91,Indirect_Y,6,OP_STA},
	OP_FUTURE(0x92),
	OP_FUTURE(0x93),
	{0x94,ZeroPage_X,4,OP_STY},
	{0x95,ZeroPage_X,4,OP_STA},
	{0x96,ZeroPage_Y,4,OP_STX},
	OP_FUTURE(0x97),
	{0x98,Implied,2,OP_TYA},
	{0x99,Absolute_Y,5,OP_STA},
	{0x9a,Implied,2,OP_TXS},
	OP_FUTURE(0x9b),
	OP_FUTURE(0x9c),
	{0x9d,Absolute_X,5,OP_STA},
	OP_FUTURE(0x9e),
	OP_FUTURE(0x9f),

	{0xa0,Immediate,2,OP_LDY},
	{0xa1,Indirect_X,6,OP_LDA},
	{0xa2,Immediate,2,OP_LDX},
	OP_FUTURE(0xa3),
	{0xa4,ZeroPage,2,OP_LDY},
	{0xa5,ZeroPage,2,OP_LDA},
	{0xa6,ZeroPage,2,OP_LDX},
	OP_FUTURE(0xa7),
	{0xa8,Implied,2,OP_TAY},
	{0xa9,Immediate,2,OP_LDA},
	{0xaa,Implied,2,OP_TAX},
	OP_FUTURE(0xab),
	{0xac,Absolute,4,OP_LDY},
	{0xad,Absolute,4,OP_LDA},
	{0xae,Absolute,4,OP_LDX},
	OP_FUTURE(0xaf),

	{0xb0,Relative,2,OP_BCS},
	{0xb1,Absolute_Y,5,OP_LDA},
	OP_FUTURE(0xb2),
	OP_FUTURE(0xb3),
	{0xb4,ZeroPage_X,4,OP_LDY},
	{0xb5,ZeroPage_X,4,OP_LDA},
	{0xb6,ZeroPage_Y,4,OP_LDX},
	OP_FUTURE(0xb7),
	{0xb8,Implied,2,OP_CLV},
	{0xb9,Absolute_Y,4,OP_LDA},
	{0xba,Implied,2,OP_TSX},
	OP_FUTURE(0xbb),
	{0xbc,Absolute_X,4,OP_LDY},
	{0xbd,Absolute_X,4,OP_LDA},
	{0xbe,Absolute_Y,4,OP_LDX},
	OP_FUTURE(0xbf),

	{0xc0,Immediate,2,OP_CPY},
	{0xc1,Indirect_X,6,OP_CMP},
	OP_FUTURE(0xc2),
	OP_FUTURE(0xc3),
	{0xc4,ZeroPage,3,OP_CPY},
	{0xc5,ZeroPage,3,OP_CMP},
	{0xc6,ZeroPage,5,OP_DEC},
	OP_FUTURE(0xc7),
	{0xc8,Implied,2,OP_INY},
	{0xc9,Immediate,2,OP_CMP},
	{0xca,Implied,2,OP_DEX},
	OP_FUTURE(0xcb),
	{0xcc,Absolute,4,OP_CPY},
	{0xcd,Absolute,4,OP_CMP},
	{0xce,Absolute,6,OP_DEC},
	OP_FUTURE(0xcf),

	{0xd0,Relative,2,OP_BNE},
	{0xd1,Indirect_Y,5,OP_CMP},
	OP_FUTURE(0xd2),
	OP_FUTURE(0xd3),
	OP_FUTURE(0xd4),
	{0xd5,ZeroPage_X,4,OP_CMP},
	{0xd6,ZeroPage_X,6,OP_DEC},
	OP_FUTURE(0xd7),
	{0xd8,Implied,2,OP_CLD},
	{0xd9,Absolute_Y,4,OP_CMP},
	OP_FUTURE(0xda),
	OP_FUTURE(0xdb),
	OP_FUTURE(0xdc),
	{0xdd,Absolute_X,4,OP_CMP},
	{0xde,Absolute_X,7,OP_DEC},
	OP_FUTURE(0xdf),

	{0xe0,Immediate,2,OP_CPX},
	{0xe1,Indirect_X,6,OP_SBC},
	OP_FUTURE(0xd2),
	OP_FUTURE(0xe3),
	{0xe4,ZeroPage,3,OP_CPX},
	{0xe5,ZeroPage,3,OP_SBC},
	{0xe6,ZeroPage,5,OP_INC},
	OP_FUTURE(0xe7),
	{0xe8,Implied,2,OP_INX},
	{0xe9,Immediate,2,OP_SBC},
	{0xea,Implied,2,OP_NOP},
	OP_FUTURE(0xeb),
	{0xec,Absolute,4,OP_CPX},
	{0xed,Absolute,4,OP_SBC},
	{0xee,Absolute,6,OP_INC},
	OP_FUTURE(0xef),

	{0xf0,Relative,2,OP_BEQ},
	{0xf1,Indirect_Y,5,OP_SBC},
	OP_FUTURE(0xf2),
	OP_FUTURE(0xf3),
	OP_FUTURE(0xf4),
	{0xf5,ZeroPage_X,4,OP_SBC},
	{0xf6,ZeroPage_X,6,OP_INC},
	OP_FUTURE(0xf7),
	{0xf8,Implied,2,OP_SED},
	{0xf9,Absolute_Y,4,OP_SBC},
	OP_FUTURE(0xfa),
	OP_FUTURE(0xfb),
	OP_FUTURE(0xfc),
	{0xfd,Absolute_X,4,OP_SBC},
	{0xfe,Absolute_X,7,OP_INC},
	OP_FUTURE(0xff)
};


void Cpu_SetZ(unsigned char b)
{
	global_cpu.Z = (b == 0 ? 1 : 0);
}

void Cpu_SetN(unsigned char b)
{
	global_cpu.N = ((b & 0x80) == 0 ? 0 : 1);
}

/**
 * Push 1 Byte Onto Stack
 */
void Cpu_PushB(unsigned char b)
{
	Mem_WriteB((0x100|global_cpu.SP),b);
	global_cpu.SP--;
}

/**
 * Push 2 Bytes Onto Stack
 */
void Cpu_PushW(uint16_t w)
{
	unsigned char hi = w >> 8;
	unsigned char lo = w & 0xff;
	Cpu_PushB(hi);
	Cpu_PushB(lo);
}

/**
 * DEBUG OPCODE
 * @param op [description]
 */
static void Cpu_DebugOP(struct OPCODE* op)
{
	printf("OPDBG:[0x%02X] mode[%d] BYTES[%d] %d %s\n",op->opcode,op->mode,OPBytes[op->mode],op->cycles,op->name);
}

/**
 * show all opcodes
 */
static void Cpu_ShowOps(void)
{
	int i;
	for(i=0;i<256;i++)
	{
		if(__ops[i].mode == Future)
			continue;
		printf("[0x%02X] %s mode=%d\n",__ops[i].opcode,__ops[i].name,__ops[i].mode);
	}
}

/**
 * SEI Set interrupt disable status
 */
void Cpu_Handle_OP_SEI(uint16_t address)
{
	UNUSED(address);
	global_cpu.I = 1;
}

/**
 * CLD Clear decimal mode
 */
void Cpu_Handle_OP_CLD(uint16_t address)
{
	UNUSED(address);
	global_cpu.D = 0;
}

/**
 * LDX Load index X with memory
 */
void Cpu_Handle_OP_LDX(uint16_t address)
{
	global_cpu.X = Mem_ReadB(address);
	Cpu_SetZ(global_cpu.X);
	Cpu_SetN(global_cpu.X);
}

/**
 * LDY Load index Y with memory
 */
void Cpu_Handle_OP_LDY(uint16_t address)
{
	global_cpu.Y = Mem_ReadB(address);
	Cpu_SetZ(global_cpu.Y);
	Cpu_SetN(global_cpu.Y);
}

/**
 * TXS Transfer index X to stack pointer
 */
void Cpu_Handle_OP_TXS(uint16_t address)
{
	global_cpu.SP = global_cpu.X;
}

/**
 * LDA Load accumulator with memory
 */
void Cpu_Handle_OP_LDA(uint16_t address)
{
	global_cpu.A = Mem_ReadB(address);
	printf("LDA address=0x%x,a=%d\n",address,global_cpu.A);
	Cpu_SetZ(global_cpu.A);
	Cpu_SetN(global_cpu.A);
}

/**
 * BPL Branch on result plus
 */
void Cpu_Handle_OP_BPL(uint16_t address)
{
	if(global_cpu.N == 0)
	{
		global_cpu.PC = address;
		//add cyles..
		//TODO...
	}
}

/**
 * ORA "OR" memory with accumulator
 */
void Cpu_Handle_OP_ORA(uint16_t address)
{
	global_cpu.A = global_cpu.A | Mem_ReadB(address);
	Cpu_SetZ(global_cpu.A);
	Cpu_SetN(global_cpu.A);
}

/**
 * BNE Branch on result not zero
 */
void Cpu_Handle_OP_BNE(uint16_t address)
{
	if(global_cpu.Z == 0)
	{
		global_cpu.PC = address;
		//TODO...
		//add cyles
	}
}

/**
 * JSR Jump to new location saving return address
 */
void Cpu_Handle_OP_JSR(uint16_t address)
{
	Cpu_PushW(global_cpu.PC - 1);
	global_cpu.PC = address;
}

/**
 * PHP Push processor status on stack
 */
void Cpu_Handle_OP_PHP(uint16_t address)
{
	UNUSED(address);
	Cpu_PushB(Cpu_GetFlags() | 0x10);
}

/**
 * BRK Force Break
 */
void Cpu_Handle_OP_BRK(uint16_t address)
{
	Cpu_PushW(global_cpu.PC);
	Cpu_Handle_OP_PHP(address);
	Cpu_Handle_OP_SEI(address);
	global_cpu.PC = Mem_ReadW(0xFFFE);
}

/**
 * INC Increment memory by one
 */
void Cpu_Handle_OP_INC(uint16_t address)
{
	unsigned char b = Mem_ReadB(address) + 1;
	Mem_WriteB(address,b);
	Cpu_SetZ(b);
	Cpu_SetN(b);
}

/**
 * STA Store accumulator in memory
 */
void Cpu_Handle_OP_STA(uint16_t address)
{
	Mem_WriteB(address,global_cpu.A);
}

/**
 * STX Store index X in memory
 */
void Cpu_Handle_OP_STX(uint16_t address)
{
	Mem_WriteB(address,global_cpu.X);
}

/**
 * DEX Decrement index X by one
 */
void Cpu_Handle_OP_DEX(uint16_t address)
{
	global_cpu.X--;
	Cpu_SetZ(global_cpu.X);
	Cpu_SetN(global_cpu.X);
}

/**
 * DEX Decrement index Y by one
 */
void Cpu_Handle_OP_DEY(uint16_t address)
{
	global_cpu.Y--;
	Cpu_SetZ(global_cpu.Y);
	Cpu_SetN(global_cpu.Y);
}

/**
 * Dispatch op code
 * @param name    [OPNAME]
 * @param address [address]
 */
void Cpu_Handle_OP(const char* name,uint16_t address)
{
	if(strcmp(name,OP_SEI) == 0)
	{
		Cpu_Handle_OP_SEI(address);
	}
	else if(strcmp(name,OP_CLD) == 0)
	{
		Cpu_Handle_OP_CLD(address);
	}
	else if(strcmp(name,OP_LDA) == 0)
	{
		Cpu_Handle_OP_LDA(address);
	}
	else if(strcmp(name,OP_LDX) == 0)
	{
		Cpu_Handle_OP_LDX(address);
	}
	else if(strcmp(name,OP_LDY) == 0)
	{
		Cpu_Handle_OP_LDY(address);
	}
	else if(strcmp(name,OP_TXS) == 0)
	{
		Cpu_Handle_OP_TXS(address);
	}
	else if(strcmp(name,OP_BPL) == 0)
	{
		Cpu_Handle_OP_BPL(address);
	}
	else if(strcmp(name,OP_ORA) == 0)
	{
		Cpu_Handle_OP_ORA(address);
	}
	else if(strcmp(name,OP_BNE) == 0)
	{
		Cpu_Handle_OP_BNE(address);
	}
	else if(strcmp(name,OP_JSR) == 0)
	{
		Cpu_Handle_OP_JSR(address);
	}
	else if(strcmp(name,OP_BRK) == 0)
	{
		Cpu_Handle_OP_BRK(address);
	}
	else if(strcmp(name,OP_INC) == 0)
	{
		Cpu_Handle_OP_INC(address);
	}
	else if(strcmp(name,OP_STA) == 0)
	{
		Cpu_Handle_OP_STA(address);
	}
	else if(strcmp(name,OP_STX) == 0)
	{
		Cpu_Handle_OP_STX(address);
	}
	else if(strcmp(name,OP_DEX) == 0)
	{
		Cpu_Handle_OP_DEX(address);
	}
	else if(strcmp(name,OP_DEY) == 0)
	{
		Cpu_Handle_OP_DEY(address);
	}
	else
	{
		printf("UnHandled OP %s\n",name);
	}
}

void Cpu_Handle_IntNMI(void)
{
	Cpu_PushW(global_cpu.PC);
	Cpu_Handle_OP_PHP(0);
	global_cpu.PC = Mem_ReadW(0xFFFA);
	global_cpu.I = 1;
	//TODO.. 
	//cycles.
}

void Cpu_Handle_IntIRQ(void)
{
	Cpu_PushW(global_cpu.PC);
	Cpu_Handle_OP_PHP(0);
	global_cpu.PC = Mem_ReadW(0xFFFE);
	global_cpu.I = 1;
	//TODO.. 
	//cycles.
}

static void Cpu_SetFlags(unsigned char flags)
{
	global_cpu.C = (flags >> 0) & 1;
	global_cpu.Z = (flags >> 1) & 1;
	global_cpu.I = (flags >> 2) & 1;
	global_cpu.D = (flags >> 3) & 1;
	global_cpu.B = (flags >> 4) & 1;
	global_cpu.U = (flags >> 5) & 1;
	global_cpu.V = (flags >> 6) & 1;
	global_cpu.N = (flags >> 7) & 1;
}

static unsigned char Cpu_GetFlags(void)
{
	unsigned char flags = 0;
	flags |= global_cpu.C << 0;
	flags |= global_cpu.Z << 1;
	flags |= global_cpu.I << 2;
	flags |= global_cpu.D << 3;
	flags |= global_cpu.B << 4;
	flags |= global_cpu.U << 5;
	flags |= global_cpu.V << 6;
	flags |= global_cpu.N << 7;
	return flags;
}

void Cpu_Init(void)
{
	memset(&global_cpu,0,sizeof(global_cpu));
	global_cpu.PC = Mem_ReadW(0xFFFC);
	global_cpu.SP = 0xFD;
	Cpu_SetFlags(0x24);
}


int Cpu_Step(void)
{
	uint16_t address;
	unsigned char opcode = Mem_ReadB(global_cpu.PC);
	struct OPCODE* OP = &__ops[opcode];
	Cpu_DebugOP(OP);
	static int interrupt = 0;
	
	
	if(OP->mode == ZeroPage)
	{
		address = Mem_ReadB(global_cpu.PC + 1);
	}
	else if(OP->mode == ZeroPage_X)
	{
		address = Mem_ReadB(global_cpu.PC + 1) + global_cpu.X;
	}
	else if(OP->mode == ZeroPage_Y)
	{
		address = Mem_ReadB(global_cpu.PC + 1) + global_cpu.Y;
	}
	else if(OP->mode == Absolute)
	{
		address = Mem_ReadW(global_cpu.PC + 1);
	}
	else if(OP->mode == Absolute_X)
	{
		address = Mem_ReadB(global_cpu.PC + 1); + global_cpu.X;
		//calc page diff
	}
	else if(OP->mode == Absolute_Y)
	{
		address = Mem_ReadB(global_cpu.PC + 1); + global_cpu.Y;
		//calc page diff
	}
	else if(OP->mode == Indirect)
	{
		printf("OPMODE[%d] Indirect unsigned\n",OP->mode);
		//cpu read bug
	}
	else if(OP->mode == Indirect_X)
	{
		printf("OPMODE[%d] Indirect unsigned\n",OP->mode);
		//cpu read bug
	}
	else if(OP->mode == Indirect_Y)
	{
		printf("OPMODE[%d] Indirect unsigned\n",OP->mode);
		//cpu read bug
	}
	else if(OP->mode == Implied)
	{
		address = 0;
	}
	else if(OP->mode == Accumulator)
	{
		address = 0;
	}
	else if(OP->mode == Immediate)
	{
		address = global_cpu.PC + 1;
	}
	else if(OP->mode == Relative)
	{
		unsigned char offset = Mem_ReadB(global_cpu.PC + 1);
		if(offset < 0x80)
			address = global_cpu.PC + 2 + offset;
		else
			address = global_cpu.PC + 2 + offset - 0x100;
	}

	global_cpu.PC += OPBytes[OP->mode];
	Cpu_Handle_OP(OP->name,address);
	

	printf("[%d]DEBUG PC=0x%04x\n",interrupt, global_cpu.PC);

	if((interrupt++)%17 == 0)
	{
		//ppu.
		//PPU status register
		Mem_WriteB(0x2002,0x80);
		//Cpu_Handle_IntNMI();
	}
	else
	{
		Mem_WriteB(0x2002,0x0);
	}

	return OP->cycles;
}

