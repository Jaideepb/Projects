#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define CAT1 1
#define CAT2 -1
#define MAXGPRS	  32
#define MAXLENGTH 40
#define MAXBUFLEN 256
#define SIZEOFINSTRUCTION 4
#define OpCodeMaxLen 8
#define START_ADDRESS 256
#define MAXNOOFINS 1024
#define INC_GLOBAL_ADDRESS() globals.insAddress+=SIZEOFINSTRUCTION
#define HASH_INDEX (globals.pcAddress-START_ADDRESS)/SIZEOFINSTRUCTION
#define INCREMENT_PC globals.pcAddress+=SIZEOFINSTRUCTION
#define DATA_INDEX(X) (X-globals.breakAddress)/SIZEOFINSTRUCTION - 1 //Zero-based index
#define DLY_SLT_ADDRESS globals.pcAddress+SIZEOFINSTRUCTION
#define JUMP_ADDRESS_TYP1 (IW->Ops.offset|(DLY_SLT_ADDRESS & 0xF0000000))  //Checking for 4-MSB bits
#define JUMP_ADDRESS_TYP2 IW->Ops.offset+DLY_SLT_ADDRESS  
#define TOTAL_DATA_INS ((START_ADDRESS + (globals.numOfTotalInstructions)*SIZEOFINSTRUCTION) - globals.breakAddress)/SIZEOFINSTRUCTION
#define LOGICAL_SHIFT(N) ((N<0)?(N^(1<<31)):N)
#define DUMMY_REGISTER 32

enum CAT_1
{
	J=0,
	JR,
	BEQ,
	BLTZ,
	BGTZ,
	BREAK,
	SW,
	LW,
	SLL,
	SRL,
	SRA,
	NOP,
	MAX_CATG1=12
};

enum CAT_2
{
	ADD=0,
	SUB,
	MUL,
	AND,
	OR,
	XOR,
	NOR,
	SLT,
	ADDI,
	ANDI,
	ORI,
	XORI,
	MAX_CATG2=12
};

/*enum FLAG
{
	FALSE,
	TRUE
}; 
*/
typedef struct{
	enum{
		NOT_INVOLVED,
		READY,
		READ,
		WRITE
	    }status;
	int counter;
	int value;
	int tempVal;
}Registers;

typedef struct{
	int numOfTotalInstructions;
	int *insNumbersFrmt;
	int isBreakFlagSet;
	int insAddress;
	int breakAddress;
	int pcAddress;
	Registers R[MAXGPRS+1]; //Move to init , +1 for default(non-visible) register no:32/ some stupid hack :)
	char buff[MAXBUFLEN];
	int Err;
	int memReg_LW;
	int memReg_SW;
	int fetchesPerCycle;
	int writeMemTemp;
	int writeALUTemp;
	int executedInstruction;
	int exitFlag;
}Globals;

typedef struct{
	int mask;
	int decodeVal;
	int idx;
}WaveringVars;

typedef struct{
	int rs;
	int rd;
	int rt;
	int offset;
	int constant;   //used as 'sa' or 'base' for CAT-1, 'immediate' for CAT-2 
}Operands;

typedef struct{
	char InsBinaryFrmt[MAXLENGTH];
	int InsNumberFrmt;
	int Address;
	int Category;
	int Opcode;
	char OpcodeName[8];
	int NoOfOperands;
	Operands Ops;
	int Value;
}InstructionLine;

#define MASK wavVals.mask
#define MASK_RESULT wavVals.decodeVal
#define MASK_IDX wavVals.idx
#define PREP_MASK(NUM_OF_BITS,LSB)		\
	 	MASK=0x1; 			\
		for(MASK_IDX=1;MASK_IDX<NUM_OF_BITS;MASK_IDX++)	\
			MASK=(MASK<<1)+1 ; 	\
		MASK=MASK<<(LSB-1);


/* for e.g MSB=32,LSB=31, in case of first two bits */
#define MASKING(X,MSB,LSB)   			\
		MASK_RESULT=0x0;			\
		PREP_MASK(MSB-LSB+1,LSB)	\
		MASK_RESULT=(X&MASK)>>(LSB-1);		

#define BREAK_INSTRUCTION  (CAT1<<2)|(BREAK)
#define OFFSET_ADJ(N)	(N<<2)
#define SIGN_EXTEND(N,MSB) ((N&(1<<(MSB-1)))>=1)?(N-(1<<MSB)):N

#define IS_INS_BRANCH(opCode) ((opCode >= J) && (opCode <= BGTZ))
#define SET_FETCH_FLAG(X) X=(globals.pcAddress==globals.breakAddress)?COMPLETE:NOTCOMPLETE

#define ISMEMORY(opCode)	( (opCode >= SW) && (opCode <= LW) ) 
#define INS_TYPE(idx)		( insDecodeStructure[idx].Category==CAT1 && ISMEMORY(insDecodeStructure[idx].Opcode) )
#define ISSUE_PER_CYCLE_COMPLETE ( nonMemInsIssued==TRUE && memInsIssued==TRUE )  // Both instruction types should be issued to complete issue cycle
#define PRE_ALU_BUFF_NOTFULL	( BUFFERSTATUS(globBuffs.preALU2Buff)!= BUFFER_FULL || BUFFERSTATUS(globBuffs.preALU1Buff) != BUFFER_FULL ) 
#define PRE_ISSUE_BUFF_FULL	( BUFFERSTATUS(globBuffs.preIssueBuff)== BUFFER_FULL )
#define NON_MEMORY_INS		FALSE
#define MEMORY_INS    		TRUE
#define SET			TRUE
#define IS_INS_STORE(idx)	( insDecodeStructure[idx].Opcode == SW )
#define FIFO			0
#define IS_INS_JUMP(idx)	( insDecodeStructure[idx].Opcode == J )

#define ExecuteBranchAndFindNextPC(X)	Crux(X,FETCH)
#define RegisterStatusUpdate(X)		Crux(X,ISSUE)
#define ExecuteInstruction(X)		Crux(X,ALU)
#define CalculateMemoryAddress(X)	Crux(X,ALU)
#define ExecuteMemoryOperation(X) 	Crux(X,MEM)
#define ExecuteWriteBack(X)		Crux(X,WB)
#define NO_ELEMENTS		-999
#define EMPTY			-999

static void SimulatorGlobalValInit();
static void ConvertIntoNumber(char inputStrings[100][MAXLENGTH]);
static void MapInstructions();
void DebugPrint(char inputStrings[100][MAXLENGTH]);
void PopulateOperandsCat1(InstructionLine *InsWord);
void PopulateOperandsCat2(InstructionLine *InsWord);

char opCodeName_C1[MAX_CATG1][OpCodeMaxLen]={
	"J",
	"JR",
	"BEQ",
	"BLTZ",
	"BGTZ",
	"BREAK",
	"SW",
	"LW",
	"SLL",
	"SRL",
	"SRA",
	"NOP"
};

char opCodeName_C2[MAX_CATG2][OpCodeMaxLen]={
	"ADD",
	"SUB",
	"MUL",
	"AND",
	"OR",
	"XOR",
	"NOR",
	"SLT",
	"ADDI",
	"ANDI",
	"ORI",
	"XORI",
};

typedef enum{
	FETCH,
	ISSUE,
	ALU,
	MEM,
	WB
}Stages;

typedef enum{
	NOTCOMPLETE,
	COMPLETE,
	STALL	
}UnitStatus;

enum BuffStatus
{
	BUFFER_EMPTY,
	BUFFER_FREE,
	BUFFER_FULL
};

typedef enum STATUS
{
	FALSE,
	TRUE
}STATUS;



typedef struct {
	int *indexes;
	int noOfElements;
	int maxAllowed;
	#define BUFFERSTATUS(X) ((X.maxAllowed-X.noOfElements)?( (X.noOfElements==0)?BUFFER_EMPTY:BUFFER_FREE ):BUFFER_FULL) 
}Buffer;


typedef struct {
	UnitStatus fetchUnitFlag;
	UnitStatus issueUnitFlag;
	UnitStatus aluUnitFlag;
	UnitStatus memUnitFlag;
	UnitStatus writeback1UnitFlag;
	UnitStatus writeback2UnitFlag;
	UnitStatus totalPipeline;
}GlobalFlags;

typedef struct {
	Buffer preIssueBuff;
	Buffer preALU1Buff;
	Buffer preALU2Buff;
	Buffer preMemBuff;
	Buffer postALU2Buff;	
	Buffer postMemBuff;
}GlobalBuffs;

typedef struct
{
unsigned int READ : 1;
unsigned int WRITE : 1;
}HazardInternal;

#define FETCH_STAGE 	globFlags.fetchUnitFlag
#define ISSUE_STAGE	(FETCH_STAGE==COMPLETE && BUFFERSTATUS(globBuffs.preIssueBuff)==BUFFER_EMPTY)
#define ALU_STAGE	(ISSUE_STAGE==COMPLETE && ( BUFFERSTATUS(globBuffs.preALU1Buff)==BUFFER_EMPTY && BUFFERSTATUS(globBuffs.preALU2Buff)==BUFFER_EMPTY ))
#define MEMORY_STAGE	(ALU_STAGE==COMPLETE && ( BUFFERSTATUS(globBuffs.preMemBuff)==BUFFER_EMPTY) )
#define WRITEBACK_STAGE1 (MEMORY_STAGE==COMPLETE && ( BUFFERSTATUS(globBuffs.postMemBuff)==BUFFER_EMPTY && BUFFERSTATUS(globBuffs.postALU2Buff)==BUFFER_EMPTY ))
#define WRITEBACK_STAGE2 (WRITEBACK_STAGE1==COMPLETE && globals.writeMemTemp == EMPTY && globals.writeALUTemp == EMPTY)

#define PIPELINE_STATUS (globFlags.fetchUnitFlag && globFlags.issueUnitFlag && globFlags.aluUnitFlag && globFlags.memUnitFlag && globFlags.writeback1UnitFlag && globFlags.writeback2UnitFlag)
#define DEFAULT  	{{NULL,0,4},{NULL,0,2},{NULL,0,2},{NULL,0,1},{NULL,0,1},{NULL,0,1}};


