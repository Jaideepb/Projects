/* Actual Stuff happens Here ...
   Declarations of all the functions are here 
*/
#include "project.h"

Globals globals;
GlobalFlags globFlags;
GlobalBuffs globBuffs=DEFAULT;
WaveringVars wavVals;
InstructionLine *insDecodeStructure;
InstructionLine *dataMemLoc;
FILE *ofp_trace,*ofp_assm;

static void SimulatorGlobalValInit()
{
	int i;
	globals.numOfTotalInstructions	= 0;
	globals.insNumbersFrmt		= NULL;
	globals.isBreakFlagSet		= FALSE;
	globals.insAddress		= START_ADDRESS;
	globals.pcAddress		= START_ADDRESS;
	globals.fetchesPerCycle		= 2;
	wavVals.decodeVal		= 0x0;
	wavVals.mask 			= 0x1;
	insDecodeStructure		= NULL;
	dataMemLoc			= NULL;
	for(i=0;i<MAXGPRS;i++)
	{
		globals.R[i].status	= READY;
		globals.R[i].counter	= 0;
	}
	
	globals.R[DUMMY_REGISTER].value = -999;
	globals.R[DUMMY_REGISTER].status= NOT_INVOLVED;
	globals.exitFlag = FALSE;
}

//Storing retrieved strings in number format
static void ConvertIntoNumber(char inputStrings[100][MAXLENGTH])
{
	int *outNumberFormat=globals.insNumbersFrmt;
	int i,tempNum=0;
	while(tempNum<=globals.numOfTotalInstructions)
	{
		for(i=0;i<32;i++)
		{
			outNumberFormat[tempNum]  = outNumberFormat[tempNum]<<1;
			outNumberFormat[tempNum] += inputStrings[tempNum][i]-48;	
		}
		tempNum++;
	}
}

static void MapInstructions()
{
	int insNum=0;
	while(insNum<=globals.numOfTotalInstructions)
	{
		
	    	insDecodeStructure[insNum].Address = globals.insAddress;
		insDecodeStructure[insNum].InsNumberFrmt   = globals.insNumbersFrmt[insNum];
		
		if(globals.isBreakFlagSet)
		{
			insDecodeStructure[insNum].Value   = globals.insNumbersFrmt[insNum];
		}
		else
		{
			MASKING(globals.insNumbersFrmt[insNum],32,31);
			insDecodeStructure[insNum].Category = wavVals.decodeVal;

			MASKING(globals.insNumbersFrmt[insNum],30,27);
			insDecodeStructure[insNum].Opcode = wavVals.decodeVal;
			
			switch(insDecodeStructure[insNum].Category)
			{
				case CAT1:  PopulateOperandsCat1(&insDecodeStructure[insNum]);
					break;
				case CAT2:  PopulateOperandsCat2(&insDecodeStructure[insNum]);
					break;
				default:
					/* -- To-do -- 
		        	       	 Error Handling 
        		        	*/
					break;
			}			

			if(insDecodeStructure[insNum].Category==CAT1 && insDecodeStructure[insNum].Opcode==BREAK)
			{
				globals.isBreakFlagSet=TRUE;
				globals.breakAddress=insDecodeStructure[insNum].Address;
				dataMemLoc = &insDecodeStructure[insNum+1];
			}

		}		
		insNum++;
		INC_GLOBAL_ADDRESS();
	}
	
}

void PopulateOperandsCat1(InstructionLine *InsWord)
{
	int kk;
	switch(InsWord->Opcode)
	{
		case J:		InsWord->Ops.rs = DUMMY_REGISTER;
				InsWord->Ops.rt = DUMMY_REGISTER;
				InsWord->Ops.rd = DUMMY_REGISTER;
				InsWord->Ops.constant = DUMMY_REGISTER;
	
				MASKING(InsWord->InsNumberFrmt,26,1);
				InsWord->Ops.offset = OFFSET_ADJ(wavVals.decodeVal);
			break;		
		case JR:	InsWord->Ops.rt = DUMMY_REGISTER;
				InsWord->Ops.rd = DUMMY_REGISTER;
				InsWord->Ops.constant = DUMMY_REGISTER;
	
				MASKING(InsWord->InsNumberFrmt,26,22);
				InsWord->Ops.rs = wavVals.decodeVal;
			break;
		case BEQ:	InsWord->Ops.rd = DUMMY_REGISTER;
				InsWord->Ops.constant = DUMMY_REGISTER;
	
				MASKING(InsWord->InsNumberFrmt,26,22);
				InsWord->Ops.rs = wavVals.decodeVal;
				MASKING(InsWord->InsNumberFrmt,21,17);
				InsWord->Ops.rt = wavVals.decodeVal;
				MASKING(InsWord->InsNumberFrmt,16,1);
				InsWord->Ops.offset = SIGN_EXTEND(OFFSET_ADJ(wavVals.decodeVal),18);
			break;
		case BLTZ:
		case BGTZ:	InsWord->Ops.rt = DUMMY_REGISTER;
				InsWord->Ops.rd = DUMMY_REGISTER;
				InsWord->Ops.constant = DUMMY_REGISTER;

				MASKING(InsWord->InsNumberFrmt,26,22);
				InsWord->Ops.rs = wavVals.decodeVal;
				MASKING(InsWord->InsNumberFrmt,16,1);
				InsWord->Ops.offset = SIGN_EXTEND(OFFSET_ADJ(wavVals.decodeVal),18);
			break;
		case BREAK:	
			break;
		case SW:
		case LW:	InsWord->Ops.rs = DUMMY_REGISTER;
				InsWord->Ops.rd = DUMMY_REGISTER;
				
				MASKING(InsWord->InsNumberFrmt,26,22);
				InsWord->Ops.constant = wavVals.decodeVal;
				MASKING(InsWord->InsNumberFrmt,21,17);
				InsWord->Ops.rt = wavVals.decodeVal;
				MASKING(InsWord->InsNumberFrmt,16,1);
				InsWord->Ops.offset = SIGN_EXTEND(wavVals.decodeVal,18); //OFFSET_ADJ not required
			break;
		case SLL:	
		case SRL:
		case SRA:	InsWord->Ops.rs = DUMMY_REGISTER;

				MASKING(InsWord->InsNumberFrmt,21,17);
                                InsWord->Ops.rt = wavVals.decodeVal;
                                MASKING(InsWord->InsNumberFrmt,16,12);
                                InsWord->Ops.rd = wavVals.decodeVal;
                                MASKING(InsWord->InsNumberFrmt,11,7);
                                InsWord->Ops.constant = wavVals.decodeVal;
			break;
		case NOP:
			break;
		default:
			/* -- To-do -- 
	               	 Error Handling 
                	*/
			break;
	}
}


void PopulateOperandsCat2(InstructionLine *InsWord)
{
	switch(InsWord->Opcode)
	{
		case ADD:
		case SUB:
		case MUL:
		case AND:
		case OR :
		case XOR:
		case NOR:
		case SLT:	InsWord->Ops.constant = DUMMY_REGISTER;
				
				MASKING(InsWord->InsNumberFrmt,26,22);	
				InsWord->Ops.rs = wavVals.decodeVal;
				MASKING(InsWord->InsNumberFrmt,21,17);
			  	InsWord->Ops.rt = wavVals.decodeVal;
				MASKING(InsWord->InsNumberFrmt,16,12);
			  	InsWord->Ops.rd = wavVals.decodeVal;
			break;
		case ADDI:
		case ANDI:
		case ORI:
		case XORI:	InsWord->Ops.rd = DUMMY_REGISTER;
				
				MASKING(InsWord->InsNumberFrmt,26,22);
				InsWord->Ops.rs = wavVals.decodeVal;
				MASKING(InsWord->InsNumberFrmt,21,17);
				InsWord->Ops.rt = wavVals.decodeVal;
				MASKING(InsWord->InsNumberFrmt,16,1);
				InsWord->Ops.constant = wavVals.decodeVal;
			break;
		default:
			/* -- To-do -- 
			 Error Handling 
			*/
			break;

	}
}

void PrintOperandsCat1(InstructionLine *InsWord,char temp[])
{
	switch(InsWord->Opcode)
	{
		case J:		sprintf(temp,"%s #%d",opCodeName_C1[InsWord->Opcode],InsWord->Ops.offset);
			break;		
		case JR:	sprintf(temp,"%s R%d",opCodeName_C1[InsWord->Opcode],InsWord->Ops.rs);
			break;
		case BEQ:	sprintf(temp,"%s R%d, R%d, #%d",opCodeName_C1[InsWord->Opcode],InsWord->Ops.rs,InsWord->Ops.rt,InsWord->Ops.offset);
			break;
		case BLTZ:
		case BGTZ:	sprintf(temp,"%s R%d, #%d",opCodeName_C1[InsWord->Opcode],InsWord->Ops.rs,InsWord->Ops.offset);
			break;
		case BREAK:	sprintf(temp,"%s",opCodeName_C1[InsWord->Opcode]);
			break;
		case SW:
		case LW:	sprintf(temp,"%s R%d, %d(R%d)",opCodeName_C1[InsWord->Opcode],InsWord->Ops.rt,InsWord->Ops.offset,InsWord->Ops.constant);
			break;
		case SLL:	
		case SRL:
		case SRA:	sprintf(temp,"%s R%d, R%d, #%d",opCodeName_C1[InsWord->Opcode],InsWord->Ops.rd,InsWord->Ops.rt,InsWord->Ops.constant);
			break;
		case NOP:	sprintf(temp,"%s ",opCodeName_C1[InsWord->Opcode]);
			break;
		default:
			/* -- To-do -- 
	               	 Error Handling 
                	*/
			break;
	}

}

void PrintOperandsCat2(InstructionLine *InsWord,char temp[])
{
	switch(InsWord->Opcode)
	{
		case ADD:
		case SUB:
		case MUL:
		case AND:
		case OR :
		case XOR:
		case SLT:
		case NOR:	sprintf(temp,"%s R%d, R%d, R%d",opCodeName_C2[InsWord->Opcode],InsWord->Ops.rd,InsWord->Ops.rs,InsWord->Ops.rt);
			break;
		case ADDI:
		case ANDI:
		case ORI:
		case XORI:	sprintf(temp,"%s R%d, R%d, #%d",opCodeName_C2[InsWord->Opcode],InsWord->Ops.rt,InsWord->Ops.rs,InsWord->Ops.constant);
			break;
		default:
			/* -- To-do -- 
			 Error Handling 
			*/
			break;
	}
}

void PrintInstruction(int i,char temp[])
{
	
	switch(insDecodeStructure[i].Category)
	{
		case CAT1:  PrintOperandsCat1(&insDecodeStructure[i],temp);
			break;
		case CAT2:  PrintOperandsCat2(&insDecodeStructure[i],temp);
			break;
		default:
			/* -- To-do -- 
	       	       	 Error Handling 
                	*/
		break;
	}
}

void PrintAssembly(char inputStrings[100][MAXLENGTH])
{
 	int i;                       
	char temp[64];                                                                                                                	
 	for(i=0;i<=globals.numOfTotalInstructions;i++)
	{
			
 		sprintf(temp,"%s\t",inputStrings[i]);
		strcat(globals.buff,temp);
		memset(temp,0,sizeof(temp));

		sprintf(temp,"%d\t",insDecodeStructure[i].Address);
		strcat(globals.buff,temp);
		memset(temp,0,sizeof(temp));
		if(insDecodeStructure[i].Address<=globals.breakAddress)
		{
			PrintInstruction(i,temp);
		}
		else
		{
			sprintf(temp,"%d",insDecodeStructure[i].Value);
		}
		strcat(globals.buff,temp);
		memset(temp,0,sizeof(temp));
		fprintf(ofp_assm,"%s\n",globals.buff);
		memset(globals.buff,0,sizeof(globals.buff));
	}
}


void DebugPrint(char inputStrings[100][MAXLENGTH])
{
	int i;
	for(i=0;i<=globals.numOfTotalInstructions;i++)	
		printf("%s\t%d\t%d\t%d\n",inputStrings[i],insDecodeStructure[i].Address,insDecodeStructure[i].Category,insDecodeStructure[i].Opcode);
}
/*
void PrintInsLine(InstructionLine *IW,int Cycle)
{
	char temp[64];
	memset(globals.buff,0,sizeof(globals.buff));

	sprintf(temp,"--------------------\n");
	strcat(globals.buff,temp);
	memset(temp,0,sizeof(temp));

	sprintf(temp,"Cycle:%d\t%d\t",Cycle,IW->Address);
	strcat(globals.buff,temp);
	memset(temp,0,sizeof(temp));
	switch(IW->Category)	
		{
			case CAT1: PrintOperandsCat1(IW,temp);
				break;
			case CAT2: PrintOperandsCat2(IW,temp);
				break;
			default:
				break;			

		}

	strcat(globals.buff,temp);
	memset(temp,0,sizeof(temp));
	printf("%s\n",globals.buff);
	memset(globals.buff,0,sizeof(globals.buff));

}
*/


void PrintInsLine(InstructionLine *IW,int Cycle)
{
	char temp[64];
	memset(globals.buff,0,sizeof(globals.buff));

	sprintf(temp,"--------------------\n");
	strcat(globals.buff,temp);
	memset(temp,0,sizeof(temp));

	sprintf(temp,"Cycle:%d\t%d\t",Cycle,IW->Address);
	strcat(globals.buff,temp);
	memset(temp,0,sizeof(temp));
	switch(IW->Category)	
		{
			case CAT1: PrintOperandsCat1(IW,temp);
				break;
			case CAT2: PrintOperandsCat2(IW,temp);
				break;
			default:
				break;			

		}

	strcat(globals.buff,temp);
	memset(temp,0,sizeof(temp));
	fprintf(ofp_trace,"%s\n",globals.buff);
	memset(globals.buff,0,sizeof(globals.buff));

}


PrintGPRValues()
{
	char temp[64];
	int i,j;
	memset(globals.buff,0,sizeof(globals.buff));

	fprintf(ofp_trace,"Registers\n");
	
	for(i=0;i<MAXGPRS;i=i+8)
	{
		sprintf(temp,"R%02d:",i);
		strcat(globals.buff,temp);
		memset(temp,0,sizeof(temp));
		for(j=0;j<8;j++)
		{
			sprintf(temp,"\t%d",globals.R[i+j].value);
			strcat(globals.buff,temp);  	
		        memset(temp,0,sizeof(temp));
		}
		
		fprintf(ofp_trace,"%s\n",globals.buff);                	
		memset(globals.buff,0,sizeof(globals.buff));
	}
	sprintf(temp,"\n");
	strcat(globals.buff,temp); 
	fprintf(ofp_trace,"%s",globals.buff);  	
	memset(temp,0,sizeof(temp));
	memset(globals.buff,0,sizeof(globals.buff));
}

/*

PrintGPRValues()
{
	char temp[64];
	int i,j;
	memset(globals.buff,0,sizeof(globals.buff));

	printf("Registers\n");
	
	for(i=0;i<MAXGPRS;i=i+8)
	{
		sprintf(temp,"R%02d:",i);
		strcat(globals.buff,temp);
		memset(temp,0,sizeof(temp));
		for(j=0;j<8;j++)
		{
			sprintf(temp,"\t%d",globals.R[i+j].value);
			strcat(globals.buff,temp);  	
		        memset(temp,0,sizeof(temp));
		}
		
		printf("%s\n",globals.buff);                	
		memset(globals.buff,0,sizeof(globals.buff));
	}
	sprintf(temp,"\n");
	strcat(globals.buff,temp); 
	printf("%s",globals.buff);  	
	memset(temp,0,sizeof(temp));
	memset(globals.buff,0,sizeof(globals.buff));
}

PrintDataValues()
{
	char temp[64];
	int i,j;
	memset(globals.buff,0,sizeof(globals.buff));

	printf("Data\n");
	
	for(i=0;i<TOTAL_DATA_INS;i=i+8)
	{
		sprintf(temp,"%d:",dataMemLoc[i].Address);
		strcat(globals.buff,temp);
		memset(temp,0,sizeof(temp));
		for(j=0;j<8;j++)
		{
			sprintf(temp,"\t%d",dataMemLoc[i+j].Value);
			strcat(globals.buff,temp);  	
		        memset(temp,0,sizeof(temp));
		}
		printf("%s\n",globals.buff);                	
		memset(globals.buff,0,sizeof(globals.buff));
	}
*	sprintf(temp,"\n");
	strcat(globals.buff,temp); 
	printf("%s",globals.buff);  	*
//	fprintf(ofp_trace,"%s",globals.buff);  	
	memset(temp,0,sizeof(temp));
	memset(globals.buff,0,sizeof(globals.buff));
}
*/

PrintDataValues()
{
	char temp[64];
	int i,j;
	memset(globals.buff,0,sizeof(globals.buff));

	fprintf(ofp_trace,"Data\n");
	
	for(i=0;i<TOTAL_DATA_INS;i=i+8)
	{
		sprintf(temp,"%d:",dataMemLoc[i].Address);
		strcat(globals.buff,temp);
		memset(temp,0,sizeof(temp));
		for(j=0;j<8;j++)
		{
			sprintf(temp,"\t%d",dataMemLoc[i+j].Value);
			strcat(globals.buff,temp);  	
		        memset(temp,0,sizeof(temp));
		}
		fprintf(ofp_trace,"%s\n",globals.buff);                	
		memset(globals.buff,0,sizeof(globals.buff));
	}
/*	sprintf(temp,"\n");
	strcat(globals.buff,temp); 
	printf("%s",globals.buff);  	*/
//	fprintf(ofp_trace,"%s",globals.buff);  	
	memset(temp,0,sizeof(temp));
	memset(globals.buff,0,sizeof(globals.buff));
}
			



void ExecuteInsCat_1(InstructionLine *IW,int stage)
{
	int dIx,temp=0;
	switch(IW->Opcode)
	{
		case J:		if(stage == FETCH)
				{
					globals.pcAddress = JUMP_ADDRESS_TYP1;		
				}
			break;		
		case JR:	if(stage == FETCH)
				{
					globals.pcAddress  = globals.R[IW->Ops.rs].value;	
				}
			break;
		case BEQ:	if(stage == FETCH)
				{
					if(globals.R[IW->Ops.rs].value==globals.R[IW->Ops.rt].value) 
						globals.pcAddress = JUMP_ADDRESS_TYP2;
					else
						INCREMENT_PC;
				}
			break;
		case BLTZ:	if(stage == FETCH)
				{
					if(globals.R[IW->Ops.rs].value<0)
						globals.pcAddress = JUMP_ADDRESS_TYP2;
					else
						INCREMENT_PC;
				}
			break;
		case BGTZ:	if(stage == FETCH)
				{
					if(globals.R[IW->Ops.rs].value>0)
						globals.pcAddress = JUMP_ADDRESS_TYP2;
					else
						INCREMENT_PC;
				}
			break;
		case BREAK:	
			break;
		case SW:	if(stage==ISSUE)
				{
					globals.R[IW->Ops.constant].status = READ;
					globals.R[IW->Ops.constant].counter++;
					globals.R[IW->Ops.rt].status	   = READ;
					globals.R[IW->Ops.rt].counter++;
				}	
				else if(stage==ALU)
				{
					globals.memReg_SW = globals.R[IW->Ops.constant].value+IW->Ops.offset;   // constant is used to store base value
				}
				else if(stage==MEM)
				{
					dIx = DATA_INDEX(globals.memReg_SW);
					dataMemLoc[dIx].Value = globals.R[IW->Ops.rt].value;

					if( (--globals.R[IW->Ops.constant].counter) == 0 )
						globals.R[IW->Ops.constant].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
					globals.memReg_SW = 0;
				}
			break;
		case LW:	if(stage == ISSUE)
				{
					globals.R[IW->Ops.constant].status = READ;
					globals.R[IW->Ops.constant].counter++;
					globals.R[IW->Ops.rt].status	   = WRITE;
					globals.R[IW->Ops.rt].counter++;
				}	
				else if(stage == ALU)
				{
					globals.memReg_LW = globals.R[IW->Ops.constant].value + IW->Ops.offset;   // constant is used to store base value
				}
				else if(stage == MEM)
				{
					dIx = DATA_INDEX(globals.memReg_LW);
					globals.R[IW->Ops.rt].tempVal = dataMemLoc[dIx].Value;
				}
				else if(stage == WB)
				{
					globals.R[IW->Ops.rt].value  = globals.R[IW->Ops.rt].tempVal;
					if( (--globals.R[IW->Ops.constant].counter) == 0 )
						globals.R[IW->Ops.constant].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status 	   = READY;
				//	globals.memReg_LW = 0;
				}
			break;
		case SLL:	if(stage == ISSUE)
				{
					globals.R[IW->Ops.rt].status = READ;
					globals.R[IW->Ops.rt].counter++;
					globals.R[IW->Ops.rd].status = WRITE;	
					globals.R[IW->Ops.rd].counter++;
				}	
				else if(stage == ALU)
				{
					globals.R[IW->Ops.rd].tempVal = LOGICAL_SHIFT(globals.R[IW->Ops.rt].value)<<IW->Ops.constant;
				}
				else if(stage == WB)
				{
					globals.R[IW->Ops.rd].value  = globals.R[IW->Ops.rd].tempVal;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
					if( (--globals.R[IW->Ops.rd].counter) == 0)
						globals.R[IW->Ops.rd].status = READY;
				}
			break;	
		case SRL:	if(stage==ISSUE)
				{
					globals.R[IW->Ops.rt].status = READ;
					globals.R[IW->Ops.rt].counter++;
					globals.R[IW->Ops.rd].status = WRITE;	
					globals.R[IW->Ops.rd].counter++;	
				}	
				else if(stage==ALU)
				{
					if(globals.R[IW->Ops.rt].value<0)
						temp=temp|(1<<(31-IW->Ops.constant));
					globals.R[IW->Ops.rd].tempVal = temp | (LOGICAL_SHIFT(globals.R[IW->Ops.rt].value)>>IW->Ops.constant);
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rd].value  = globals.R[IW->Ops.rd].tempVal;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
					if( (--globals.R[IW->Ops.rd].counter) == 0)	
						globals.R[IW->Ops.rd].status = READY;
				}
			break;
		case SRA:	if(stage==ISSUE)
				{	
					globals.R[IW->Ops.rt].status = READ;
					globals.R[IW->Ops.rt].counter++;
					globals.R[IW->Ops.rd].status = WRITE;
					globals.R[IW->Ops.rd].counter++;
				}	
				else if(stage==ALU)
				{
					globals.R[IW->Ops.rd].tempVal = globals.R[IW->Ops.rt].value>>IW->Ops.constant;   // To-do ... something not right here ... read the command again
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rd].value  = globals.R[IW->Ops.rd].tempVal;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
					if( (--globals.R[IW->Ops.rd].counter) == 0)
						globals.R[IW->Ops.rd].status = READY;
				}
			break;
		case NOP:	
			break;
		default:
			/* -- To-do -- 
	               	 Error Handling 
                	*/
			break;
	}
}

void ExecuteInsCat_2(InstructionLine *IW,int stage)
{
	switch(IW->Opcode)
	{
		case ADD: 	if(stage==ISSUE)
				{
					globals.R[IW->Ops.rs].status = READ;
					globals.R[IW->Ops.rs].counter++;
					globals.R[IW->Ops.rt].status = READ;
					globals.R[IW->Ops.rt].counter++;
					globals.R[IW->Ops.rd].status = WRITE;
					globals.R[IW->Ops.rd].counter++;
				}	
				else if(stage==ALU)
				{
					globals.R[IW->Ops.rd].tempVal = globals.R[IW->Ops.rs].value + globals.R[IW->Ops.rt].value;
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rd].value  = globals.R[IW->Ops.rd].tempVal;
					if( (--globals.R[IW->Ops.rd].counter) == 0)
						globals.R[IW->Ops.rd].status = READY;
					if( (--globals.R[IW->Ops.rs].counter) == 0)
						globals.R[IW->Ops.rs].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
				}
			break;
		case SUB:	if(stage==ISSUE)
				{
					globals.R[IW->Ops.rs].status = READ;
					globals.R[IW->Ops.rs].counter++;
					globals.R[IW->Ops.rt].status = READ;
					globals.R[IW->Ops.rt].counter++;
					globals.R[IW->Ops.rd].status = WRITE;
					globals.R[IW->Ops.rd].counter++;
				}	
				else if(stage==ALU)
				{
					globals.R[IW->Ops.rd].tempVal = globals.R[IW->Ops.rs].value - globals.R[IW->Ops.rt].value;
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rd].value  = globals.R[IW->Ops.rd].tempVal;
					if( (--globals.R[IW->Ops.rd].counter) == 0)
						globals.R[IW->Ops.rd].status = READY;
					if( (--globals.R[IW->Ops.rs].counter) == 0)
						globals.R[IW->Ops.rs].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
				}
			break;
		case MUL:	if(stage==ISSUE)
				{
					globals.R[IW->Ops.rs].status = READ;
					globals.R[IW->Ops.rs].counter++;
					globals.R[IW->Ops.rt].status = READ;
					globals.R[IW->Ops.rt].counter++;
					globals.R[IW->Ops.rd].status = WRITE;
					globals.R[IW->Ops.rd].counter++;
				}	
				else if(stage==ALU)
				{
					globals.R[IW->Ops.rd].tempVal = globals.R[IW->Ops.rs].value * globals.R[IW->Ops.rt].value;
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rd].value  = globals.R[IW->Ops.rd].tempVal;
					if( (--globals.R[IW->Ops.rd].counter) == 0)
						globals.R[IW->Ops.rd].status = READY;
					if( (--globals.R[IW->Ops.rs].counter) == 0)
						globals.R[IW->Ops.rs].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)	
						globals.R[IW->Ops.rt].status = READY;
				}
			break;	
		case AND:	if(stage==ISSUE)
				{
					globals.R[IW->Ops.rs].status = READ;
					globals.R[IW->Ops.rs].counter++;
					globals.R[IW->Ops.rt].status = READ;
					globals.R[IW->Ops.rt].counter++;
					globals.R[IW->Ops.rd].status = WRITE;
					globals.R[IW->Ops.rd].counter++;
				}	
				else if(stage==ALU)
				{
					globals.R[IW->Ops.rd].tempVal = globals.R[IW->Ops.rs].value & globals.R[IW->Ops.rt].value;
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rd].value  = globals.R[IW->Ops.rd].tempVal;
					if( (--globals.R[IW->Ops.rd].counter) == 0)
						globals.R[IW->Ops.rd].status = READY;
					if( (--globals.R[IW->Ops.rs].counter) == 0)
						globals.R[IW->Ops.rs].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
				}
			break;
		case OR :	if(stage==ISSUE)
				{
					globals.R[IW->Ops.rs].status = READ;
					globals.R[IW->Ops.rs].counter++;
					globals.R[IW->Ops.rt].status = READ;
					globals.R[IW->Ops.rt].counter++;
					globals.R[IW->Ops.rd].status = WRITE;
					globals.R[IW->Ops.rd].counter++;
				}	
				else if(stage==ALU)
				{
					globals.R[IW->Ops.rd].tempVal = globals.R[IW->Ops.rs].value | globals.R[IW->Ops.rt].value;
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rd].value  = globals.R[IW->Ops.rd].tempVal;
					if( (--globals.R[IW->Ops.rd].counter) == 0)
						globals.R[IW->Ops.rd].status = READY;
					if( (--globals.R[IW->Ops.rs].counter) == 0)
						globals.R[IW->Ops.rs].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
				}
			break;
		case XOR:	if(stage==ISSUE)
				{
					globals.R[IW->Ops.rs].status = READ;
					globals.R[IW->Ops.rs].counter++;
					globals.R[IW->Ops.rt].status = READ;
					globals.R[IW->Ops.rt].counter++;
					globals.R[IW->Ops.rd].status = WRITE;
					globals.R[IW->Ops.rd].counter++;
				}	
				else if(stage==ALU)
				{
					globals.R[IW->Ops.rd].tempVal = globals.R[IW->Ops.rs].value ^ globals.R[IW->Ops.rt].value;
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rd].value  = globals.R[IW->Ops.rd].tempVal;
					if( (--globals.R[IW->Ops.rd].counter) == 0)
						globals.R[IW->Ops.rd].status = READY;
					if( (--globals.R[IW->Ops.rs].counter) == 0)
						globals.R[IW->Ops.rs].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
				}
			break;
		case NOR:	if(stage==ISSUE)
				{
					globals.R[IW->Ops.rs].status = READ;
					globals.R[IW->Ops.rs].counter++;
					globals.R[IW->Ops.rt].status = READ;
					globals.R[IW->Ops.rt].counter++;
					globals.R[IW->Ops.rd].status = WRITE;
					globals.R[IW->Ops.rd].counter++;
				}	
				else if(stage==ALU)
				{
					globals.R[IW->Ops.rd].tempVal = globals.R[IW->Ops.rs].value | globals.R[IW->Ops.rt].value;
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rd].value  = globals.R[IW->Ops.rd].tempVal;
					if( (--globals.R[IW->Ops.rd].counter) == 0)
						globals.R[IW->Ops.rd].status = READY;
					if( (--globals.R[IW->Ops.rs].counter) == 0)
						globals.R[IW->Ops.rs].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;	
				}
			break;
		case SLT:	if(stage==ISSUE)
				{
					globals.R[IW->Ops.rs].status = READ;
					globals.R[IW->Ops.rs].counter++;
					globals.R[IW->Ops.rt].status = READ;
					globals.R[IW->Ops.rt].counter++;
					globals.R[IW->Ops.rd].status = WRITE;
					globals.R[IW->Ops.rd].counter++;
				}	
				else if(stage==ALU)
				{
					globals.R[IW->Ops.rd].tempVal = (globals.R[IW->Ops.rs].value<globals.R[IW->Ops.rt].value)?1:0;
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rd].value  = globals.R[IW->Ops.rd].tempVal;
					if( (--globals.R[IW->Ops.rd].counter) == 0)
						globals.R[IW->Ops.rd].status = READY;
					if( (--globals.R[IW->Ops.rs].counter) == 0)
						globals.R[IW->Ops.rs].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
				}
			break;
		case ADDI:	if(stage==ISSUE)
				{
					globals.R[IW->Ops.rs].status = READ;
					globals.R[IW->Ops.rs].counter++;
					globals.R[IW->Ops.rt].status = WRITE;
					globals.R[IW->Ops.rt].counter++;
				}	
				else if(stage==ALU)
				{
					globals.R[IW->Ops.rt].tempVal = globals.R[IW->Ops.rs].value + IW->Ops.constant;
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rt].value  = globals.R[IW->Ops.rt].tempVal;	
					if( (--globals.R[IW->Ops.rs].counter) == 0)
						globals.R[IW->Ops.rs].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
				}
			break;
		case ANDI:	if(stage==ISSUE)
				{
					globals.R[IW->Ops.rs].status = READ;
					globals.R[IW->Ops.rs].counter++;
					globals.R[IW->Ops.rt].status = WRITE;
					globals.R[IW->Ops.rt].counter++;
				}	
				else if(stage==ALU)
				{
					globals.R[IW->Ops.rt].tempVal = globals.R[IW->Ops.rs].value & IW->Ops.constant;
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rt].value  = globals.R[IW->Ops.rt].tempVal;
					if( (--globals.R[IW->Ops.rs].counter) == 0)
						globals.R[IW->Ops.rs].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
				}
			break;
		case ORI:	if(stage==ISSUE)
				{
					globals.R[IW->Ops.rs].status = READ;
					globals.R[IW->Ops.rs].counter++;
					globals.R[IW->Ops.rt].status = WRITE;
					globals.R[IW->Ops.rt].counter++;
				}	
				else if(stage==ALU)
				{
					globals.R[IW->Ops.rt].tempVal = globals.R[IW->Ops.rs].value | IW->Ops.constant;
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rt].value  = globals.R[IW->Ops.rt].tempVal;
					if( (--globals.R[IW->Ops.rs].counter) == 0)
						globals.R[IW->Ops.rs].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
				}
			break;
		case XORI:	if(stage==ISSUE)
				{
					globals.R[IW->Ops.rs].status = READ;
					globals.R[IW->Ops.rs].counter++;
					globals.R[IW->Ops.rt].status = WRITE;
					globals.R[IW->Ops.rt].counter++;
				}	
				else if(stage==ALU)
				{
					globals.R[IW->Ops.rt].tempVal = globals.R[IW->Ops.rs].value ^ IW->Ops.constant;
				}
				else if(stage==WB)
				{
					globals.R[IW->Ops.rt].value  = globals.R[IW->Ops.rt].tempVal;
					if( (--globals.R[IW->Ops.rs].counter) == 0)
						globals.R[IW->Ops.rs].status = READY;
					if( (--globals.R[IW->Ops.rt].counter) == 0)
						globals.R[IW->Ops.rt].status = READY;
				}
			break;
		default:
			/* -- To-do -- 
			 Error Handling 
			*/
			break;
	}
}

/*
void ExecuteCycles()
{
	int idx,cycle=1;
	while(globals.pcAddress<=globals.breakAddress && globals.pcAddress>=START_ADDRESS)
	{
		idx=HASH_INDEX;
		PrintInsLine(&insDecodeStructure[idx],cycle);
		switch(insDecodeStructure[idx].Category)	
		{
			case CAT1: ExecuteInsCat_1(&insDecodeStructure[idx]);
				break;
			case CAT2: ExecuteInsCat_2(&insDecodeStructure[idx]);
				break;
			default:
				break;			
		}
		PrintGPRValues();
		PrintDataValues();
		cycle++;
	}
}
*/

void Crux(int idx,int stage)
{
	switch(insDecodeStructure[idx].Category)	
	{
		case CAT1: ExecuteInsCat_1(&insDecodeStructure[idx],stage);
			break;
		case CAT2: ExecuteInsCat_2(&insDecodeStructure[idx],stage);
			break;
		default:
			break;			
	}
}

void Enqueue(Buffer *buff,int value)
{
	buff->indexes[buff->noOfElements++]= value;
}

int Dequeue(Buffer *buff,int idx)
{
	int i,value;
	if(buff->noOfElements>0)
	{
		buff->noOfElements--;
		value = buff->indexes[idx];
		for (i=idx;i<buff->noOfElements;i++)
		{
			buff->indexes[i]=buff->indexes[i+1];
		}
		return value;
	}
	else
	{
		return NO_ELEMENTS;
	}
}


/*	---------------- HAZARD MANAGEMENT --------------	*/

STATUS WAW(Operands Ops)
{
	if( globals.R[Ops.rd].status == WRITE || ( globals.R[Ops.rd].status == NOT_INVOLVED && globals.R[Ops.rt].status == WRITE ) )
		return TRUE;
	else
		return FALSE;	
}

STATUS WAR(Operands Ops)
{
	if( globals.R[Ops.rd].status == READ || ( globals.R[Ops.rd].status == NOT_INVOLVED && globals.R[Ops.rt].status == READ ) )
		return TRUE;
	else
		return FALSE;
}

STATUS RAW(Operands Ops)
{
	if( globals.R[Ops.rs].status == WRITE || ( globals.R[Ops.rd].status != NOT_INVOLVED && globals.R[Ops.rt].status == WRITE) || globals.R[Ops.constant].status == WRITE )		
		return TRUE;
	else
		return FALSE;
}

STATUS Hazard_Check(int idx,HazardInternal *inbuff,int warCheckFlag)
{	
	Operands regs = insDecodeStructure[idx].Ops;
	STATUS 	wawHazard = FALSE,
	       	warHazard = FALSE,
	       	rawHazard = FALSE,
	       	wawHazard_I = FALSE,
		warHazard_I = FALSE,
		rawHazard_I = FALSE,
		Hazard_Exists = FALSE;

	wawHazard = WAW(regs);

	if(warCheckFlag)
		warHazard = WAR(regs);

	rawHazard = RAW(regs);

	if(!(wawHazard || warHazard || rawHazard)) // Internal Hazard Check
	{
		if( inbuff[regs.rd].WRITE == SET || ( globals.R[regs.rd].status == NOT_INVOLVED && inbuff[regs.rt].WRITE == SET ) )
			wawHazard_I=TRUE; // WAW-Internal
		if( inbuff[regs.rd].READ == SET || ( globals.R[regs.rd].status == NOT_INVOLVED && inbuff[regs.rt].READ == SET ) )
			warHazard_I=TRUE; // WAR-Internal
		if( inbuff[regs.rs].WRITE == SET || ( globals.R[regs.rd].status != NOT_INVOLVED && inbuff[regs.rt].WRITE == SET ) || inbuff[regs.constant].WRITE == SET )
			rawHazard_I=TRUE; // RAW-Internal
	}
	Hazard_Exists = wawHazard || warHazard || rawHazard || wawHazard_I || warHazard_I || rawHazard_I ;

	if(Hazard_Exists)	//Fill Internal hazard detection buffer
	{
		if ( globals.R[regs.rd].status != NOT_INVOLVED )
		{
			inbuff[regs.rd].WRITE = SET;
			if(globals.R[regs.rt].status != NOT_INVOLVED)
				inbuff[regs.rt].READ = SET;
			if(globals.R[regs.rs].status != NOT_INVOLVED)
				inbuff[regs.rs].READ = SET;
			if(globals.R[regs.constant].status != NOT_INVOLVED)
				inbuff[regs.constant].READ = SET;
		}
		else if ( globals.R[regs.rd].status == NOT_INVOLVED )
		{
			inbuff[regs.rt].WRITE = SET;
			if(globals.R[regs.rs].status != NOT_INVOLVED )
				inbuff[regs.rs].READ = SET;
			if(globals.R[regs.constant].status != NOT_INVOLVED )
				inbuff[regs.constant].READ = SET;
		}
	}
	return (Hazard_Exists);
}

STATUS BranchHazard_Detect(int idx)
{
	int i,idx2;
	Operands regs = insDecodeStructure[idx].Ops;
	Operands buffRegs;
	STATUS wawHazard,warHazard,rawHazard,Hazard_Exists;
	wawHazard = WAW(regs);
//	warHazard = WAR(regs);
	rawHazard = RAW(regs);
	Hazard_Exists = wawHazard || warHazard || rawHazard;

	// Nasty_Trick: this is not written by me :P

	if(!Hazard_Exists)
	{
		for(i=0; i<globBuffs.preIssueBuff.noOfElements ; i++)
		{
			idx2 = globBuffs.preIssueBuff.indexes[i];
			buffRegs = insDecodeStructure[idx2].Ops;
			if(globals.R[buffRegs.rd].status != NOT_INVOLVED)
			{
				if(regs.rt == buffRegs.rd || regs.rs == buffRegs.rd)
				{
					Hazard_Exists = TRUE;
					break;
				}
			}
			else if(globals.R[buffRegs.rt].status != NOT_INVOLVED )
			{
				if(regs.rt == buffRegs.rt || regs.rs == buffRegs.rt)
				{
					Hazard_Exists = TRUE;
					break;
				}
			}
		}
	}
	return Hazard_Exists;
}


/* 	------------------ FETCH ----------------------	     */

void FetchUnit(int numElements)
{
	int fetchIns = 0;
	int fpc= globals.fetchesPerCycle;
	STATUS detectionResult = FALSE;

	while(fpc && globFlags.fetchUnitFlag != COMPLETE)
	{
		fetchIns = HASH_INDEX;
		if( globFlags.fetchUnitFlag == STALL )
		{ 
			detectionResult = BranchHazard_Detect(fetchIns);    	// Check for registers ready
			if(!detectionResult)					//If No Hazard 
			{
				globals.executedInstruction=fetchIns;
				ExecuteBranchAndFindNextPC(fetchIns);		//If registers are ready(i.e., no hazards present) execute branch and the jump address
				SET_FETCH_FLAG(globFlags.fetchUnitFlag);
			}
			break;
		}
		else if ( numElements >= 4 )  				//Pre-Issue buffer full
		{
			break;	
		}
		else
		{
			if( insDecodeStructure[fetchIns].Category==CAT1 && IS_INS_BRANCH(insDecodeStructure[fetchIns].Opcode) )
			{
				if(IS_INS_JUMP(fetchIns))
				{
					globals.executedInstruction=fetchIns;
					ExecuteBranchAndFindNextPC(fetchIns);
					SET_FETCH_FLAG(globFlags.fetchUnitFlag);
				}
				else
				{
					detectionResult = BranchHazard_Detect(fetchIns);    	// Check for registers
					if(!detectionResult)					//If No Hazard 
					{	
						globals.executedInstruction=fetchIns;
						ExecuteBranchAndFindNextPC(fetchIns);		//If registers are ready(i.e., no hazards present) execute branch and the jump address
						SET_FETCH_FLAG(globFlags.fetchUnitFlag);
					}
					else
					{
						globFlags.fetchUnitFlag = STALL;				//if registers are not ready stall the fetch unit 
					}
				}
				break;						// No instruction fetch after branch... so break out of loop
			}   
			else
			{ 
					Enqueue(&globBuffs.preIssueBuff,fetchIns);            
					numElements++;
					INCREMENT_PC;
					SET_FETCH_FLAG(globFlags.fetchUnitFlag);
			}
		}
		fpc--;
	}
}



/*	----------------- ISSUE ------------------		*/

// Atmost 2 instructions out-of-order per cycle -done
// Max 1-NonMemory and 1-Memory at a time -done
// No Structural hazards i.e., Free PreALU buffers -done
// No WAW hazards with active instructions(instructions issued)
// If 2 instructions are issued in a cycle make sure not to have WAW or WAR hazards between them
// No WAR hazard with earlier not-issued instructions
// For MEM instructions all source registers are ready before issuing
// Load instructions must wait before all previous stores are issued
// Stores must be issued in-order

void IssueUnit()
{
	int nonMemInsIssued	= FALSE;
	int memInsIssued	= FALSE;
	int result		= FALSE;
	int issueIns		= FALSE;
	int storeWaiting	= FALSE;
	int i			= FALSE;
	int idx			= FALSE;
	int instructionType	= FALSE;
	int noOfElements	= globBuffs.preIssueBuff.noOfElements;
	HazardInternal Internal[33] = {[0 ... 32]={FALSE,FALSE}};
	
	
	for(i=0; (i<noOfElements && (!ISSUE_PER_CYCLE_COMPLETE) && PRE_ALU_BUFF_NOTFULL); i++) //loop till: elements in preissuebuff over (or) 2-issues over (or) preALUbuff is full
	{
		idx = (globBuffs.preIssueBuff.noOfElements-noOfElements) + i;
		instructionType = INS_TYPE(globBuffs.preIssueBuff.indexes[idx]);
		switch(instructionType)
		{
			case NON_MEMORY_INS :	if(nonMemInsIssued == FALSE && BUFFERSTATUS(globBuffs.preALU2Buff) != BUFFER_FULL)
						{
							result = Hazard_Check(globBuffs.preIssueBuff.indexes[idx],Internal,memInsIssued);
							if(result == FALSE) // No Hazard 
							{
								issueIns = Dequeue(&globBuffs.preIssueBuff,idx);
								if(issueIns != NO_ELEMENTS)
								{
									Enqueue(&globBuffs.preALU2Buff,issueIns);
									RegisterStatusUpdate(globBuffs.preALU2Buff.indexes[0]);
									nonMemInsIssued = TRUE;
								}
							}
						}
					break;

			case MEMORY_INS    :	if(memInsIssued == FALSE && storeWaiting == FALSE && BUFFERSTATUS(globBuffs.preALU1Buff) != BUFFER_FULL)
						{
							//Registers_Ready(globBuffs.preIssueBuff.indexes[i]);
							result = Hazard_Check(globBuffs.preIssueBuff.indexes[idx],Internal,nonMemInsIssued);
							if(result == FALSE) // No Hazard
							{
								issueIns = Dequeue(&globBuffs.preIssueBuff,idx);
								if(issueIns != NO_ELEMENTS)
								{
									Enqueue(&globBuffs.preALU1Buff,issueIns);
									RegisterStatusUpdate(globBuffs.preALU1Buff.indexes[0]);
									memInsIssued = TRUE;	
								}
							}
							else if( IS_INS_STORE(globBuffs.preIssueBuff.indexes[idx]) )
							{
								storeWaiting = TRUE;
							}
						}
					break;
			default:
					break;
		}
	}	
}


/*	--------------------- ALU ------------------		*/

void ALU1()
{
	int issueIns	= FALSE;
	issueIns=Dequeue(&globBuffs.preALU1Buff,FIFO);
	if(issueIns != NO_ELEMENTS)
	{
		CalculateMemoryAddress(issueIns);
		Enqueue(&globBuffs.preMemBuff,issueIns);	
	}
}

void ALU2()
{
	int issueIns	= FALSE;
	issueIns=Dequeue(&globBuffs.preALU2Buff,FIFO);
	if(issueIns != NO_ELEMENTS)
	{
		ExecuteInstruction(issueIns);
		Enqueue(&globBuffs.postALU2Buff,issueIns);	
	}
}

//PreALUBuff's follow FIFO
//ALU1 can fetch one instruction each cycle from the Pre-ALU1 queue, removes it from the Pre-ALU1 buffer and computes it.
//ALU2 can fetch one instruction each cycle from the Pre-ALU2 queue, removes it from the Pre-ALU2 buffer and compute it.

void ALUUnit()
{
	ALU1();
	ALU2();
}


/*	--------------------- Memory --------------------	*/

void MemoryUnit()
{
	int issueIns	= FALSE;
	issueIns = Dequeue(&globBuffs.preMemBuff,FIFO);
	if(issueIns != NO_ELEMENTS)
	{
		ExecuteMemoryOperation(issueIns);
		if(!IS_INS_STORE(issueIns))
			Enqueue(&globBuffs.postMemBuff,issueIns);
	}
}


/*	--------------------- WB ------------------------	*/

void WriteMemoryBuff()
{
	int issueIns	= FALSE;
	issueIns = Dequeue(&globBuffs.postMemBuff,FIFO);
	if(issueIns != NO_ELEMENTS)
		globals.writeMemTemp = issueIns;
	else
		globals.writeMemTemp = EMPTY;
}

void WriteALUBuff()
{
	int issueIns	= FALSE;
	issueIns = Dequeue(&globBuffs.postALU2Buff,FIFO);
	if(issueIns != NO_ELEMENTS)
		globals.writeALUTemp = issueIns;
	else
		globals.writeALUTemp = EMPTY;
}

void WriteToRegisters()
{
	if(globals.writeMemTemp != EMPTY)
		ExecuteWriteBack(globals.writeMemTemp);
	if(globals.writeALUTemp != EMPTY)
		ExecuteWriteBack(globals.writeALUTemp);
}

void WriteBackUnit(int I)
{	
	if(I==1)
	{
		WriteMemoryBuff();
		WriteALUBuff();
	}
	else if(I==2)
	{
		WriteToRegisters();
	}
}


/* ---------------- INIT & MAIN_MGMT -------------------	*/

void InitBuffers()
{
	globBuffs.preIssueBuff.indexes	= (int *)malloc(sizeof(int)*globBuffs.preIssueBuff.maxAllowed);
	globBuffs.preALU1Buff.indexes	= (int *)malloc(sizeof(int)*globBuffs.preALU1Buff.maxAllowed);
	globBuffs.preALU2Buff.indexes	= (int *)malloc(sizeof(int)*globBuffs.preALU2Buff.maxAllowed);
	globBuffs.preMemBuff.indexes	= (int *)malloc(sizeof(int)*globBuffs.preMemBuff.maxAllowed);
	globBuffs.postALU2Buff.indexes	= (int *)malloc(sizeof(int)*globBuffs.postALU2Buff.maxAllowed);
	globBuffs.postMemBuff.indexes	= (int *)malloc(sizeof(int)*globBuffs.postMemBuff.maxAllowed);
}

void SetFlags()
{
	globFlags.issueUnitFlag = ISSUE_STAGE;
	globFlags.aluUnitFlag = ALU_STAGE;
	globFlags.memUnitFlag = MEMORY_STAGE;
	globFlags.writeback1UnitFlag = WRITEBACK_STAGE1;
	globFlags.writeback2UnitFlag = WRITEBACK_STAGE2;
	globFlags.totalPipeline = PIPELINE_STATUS;
}

/*

void PrintCycles(int cycle)
{
	char temp[64];
	int i;
	for(i=1;i<=20;i++)
		printf("-");
	printf("\n");
	printf("Cycle:%d\n\n",cycle);
	memset(temp,0,sizeof(temp));
	printf("IF Unit:\n");
	if(globFlags.fetchUnitFlag==STALL)
	{
		PrintInstruction(HASH_INDEX,temp);
		printf("\tWaiting Instruction: [%s]\n",temp);
	}
	else
	{
		printf("\tWaiting Instruction:\n");
	}
	if(globals.executedInstruction!=EMPTY)
	{	
		memset(temp,0,sizeof(temp));
		PrintInstruction(globals.executedInstruction,temp);
		printf("\tExecuted Instruction: [%s]\n",temp);
	}
	else
		printf("\tExecuted Instruction:\n");

// PRE-ISSUE Buffer 
	memset(temp,0,sizeof(temp));
	printf("Pre-Issue Queue:\n");
	for(i=0;i<globBuffs.preIssueBuff.noOfElements;i++)
	{
		PrintInstruction(globBuffs.preIssueBuff.indexes[i],temp);
		printf("\tEntry %d: [%s]\n",i,temp);
	}
	for(;i<globBuffs.preIssueBuff.maxAllowed;i++)
		printf("\tEntry %d:\n",i);


// PRE-ALU1 Buffer
	memset(temp,0,sizeof(temp));
	printf("Pre-ALU1 Queue:\n");
 	for(i=0;i<globBuffs.preALU1Buff.noOfElements;i++) 
	{
		PrintInstruction(globBuffs.preALU1Buff.indexes[i],temp);
 		printf("\tEntry %d: [%s]\n",i,temp);       
	}
	for(;i<globBuffs.preALU1Buff.maxAllowed;i++)
        	printf("\tEntry %d:\n",i);


// PRE-MEM Buffer
	memset(temp,0,sizeof(temp));
	for(i=0;i<globBuffs.preMemBuff.noOfElements;i++) 
	{
		PrintInstruction(globBuffs.preMemBuff.indexes[0],temp);
		printf("Pre-MEM Queue: [%s]\n",temp);	
	}
	for(;i<globBuffs.preMemBuff.maxAllowed;i++)
        	printf("Pre-MEM Queue:\n");


// POST-MEM Buffer
	memset(temp,0,sizeof(temp));
	for(i=0;i<globBuffs.postMemBuff.noOfElements;i++) 
	{
		PrintInstruction(globBuffs.postMemBuff.indexes[0],temp);
		printf("Post-MEM Queue: [%s]\n",temp);
	}
	for(;i<globBuffs.postMemBuff.maxAllowed;i++)
        	printf("Post-MEM Queue:\n");


// PRE-ALU2 Buffer
	memset(temp,0,sizeof(temp));
	printf("Pre-ALU2 Queue:\n");
 	for(i=0;i<globBuffs.preALU2Buff.noOfElements;i++)                               	
	{
		PrintInstruction(globBuffs.preALU2Buff.indexes[i],temp);
 		printf("\tEntry %d: [%s]\n",i,temp);       
	}
	for(;i<globBuffs.preALU2Buff.maxAllowed;i++)
        	printf("\tEntry %d:\n",i);


// POST-ALU2 Buffer
	memset(temp,0,sizeof(temp));
	for(i=0;i<globBuffs.postALU2Buff.noOfElements;i++)                               	
	{
		PrintInstruction(globBuffs.postALU2Buff.indexes[0],temp);
		printf("Post-ALU2 Queue: [%s]\n\n",temp);
	}
	for(;i<globBuffs.postALU2Buff.maxAllowed;i++)
        	printf("Post-ALU2 Queue:\n\n");



// REGISTER AND DATA Values
	PrintGPRValues();
	PrintDataValues();
}
*/

void PrintCycles(int cycle)
{
	char temp[64];
	int i;
	for(i=1;i<=20;i++)
		fprintf(ofp_trace,"-");
	fprintf(ofp_trace,"\n");
	fprintf(ofp_trace,"Cycle:%d\n\n",cycle);
	memset(temp,0,sizeof(temp));
	fprintf(ofp_trace,"IF Unit:\n");
	if(globFlags.fetchUnitFlag==STALL)
	{
		PrintInstruction(HASH_INDEX,temp);
		fprintf(ofp_trace,"\tWaiting Instruction: [%s]\n",temp);
	}
	else
	{
		fprintf(ofp_trace,"\tWaiting Instruction:\n");
	}
	if(globals.executedInstruction!=EMPTY)
	{	
		memset(temp,0,sizeof(temp));
		PrintInstruction(globals.executedInstruction,temp);
		fprintf(ofp_trace,"\tExecuted Instruction: [%s]\n",temp);
	}
	else
		fprintf(ofp_trace,"\tExecuted Instruction:\n");

// PRE-ISSUE Buffer 
	memset(temp,0,sizeof(temp));
	fprintf(ofp_trace,"Pre-Issue Queue:\n");
	for(i=0;i<globBuffs.preIssueBuff.noOfElements;i++)
	{
		PrintInstruction(globBuffs.preIssueBuff.indexes[i],temp);
		fprintf(ofp_trace,"\tEntry %d: [%s]\n",i,temp);
	}
	for(;i<globBuffs.preIssueBuff.maxAllowed;i++)
		fprintf(ofp_trace,"\tEntry %d:\n",i);


// PRE-ALU1 Buffer
	memset(temp,0,sizeof(temp));
	fprintf(ofp_trace,"Pre-ALU1 Queue:\n");
 	for(i=0;i<globBuffs.preALU1Buff.noOfElements;i++) 
	{
		PrintInstruction(globBuffs.preALU1Buff.indexes[i],temp);
 		fprintf(ofp_trace,"\tEntry %d: [%s]\n",i,temp);       
	}
	for(;i<globBuffs.preALU1Buff.maxAllowed;i++)
        	fprintf(ofp_trace,"\tEntry %d:\n",i);


// PRE-MEM Buffer
	memset(temp,0,sizeof(temp));
	for(i=0;i<globBuffs.preMemBuff.noOfElements;i++) 
	{
		PrintInstruction(globBuffs.preMemBuff.indexes[0],temp);
		fprintf(ofp_trace,"Pre-MEM Queue: [%s]\n",temp);	
	}
	for(;i<globBuffs.preMemBuff.maxAllowed;i++)
        	fprintf(ofp_trace,"Pre-MEM Queue:\n");


// POST-MEM Buffer
	memset(temp,0,sizeof(temp));
	for(i=0;i<globBuffs.postMemBuff.noOfElements;i++) 
	{
		PrintInstruction(globBuffs.postMemBuff.indexes[0],temp);
		fprintf(ofp_trace,"Post-MEM Queue: [%s]\n",temp);
	}
	for(;i<globBuffs.postMemBuff.maxAllowed;i++)
        	fprintf(ofp_trace,"Post-MEM Queue:\n");


// PRE-ALU2 Buffer
	memset(temp,0,sizeof(temp));
	fprintf(ofp_trace,"Pre-ALU2 Queue:\n");
 	for(i=0;i<globBuffs.preALU2Buff.noOfElements;i++)                               	
	{
		PrintInstruction(globBuffs.preALU2Buff.indexes[i],temp);
 		fprintf(ofp_trace,"\tEntry %d: [%s]\n",i,temp);       
	}
	for(;i<globBuffs.preALU2Buff.maxAllowed;i++)
        	fprintf(ofp_trace,"\tEntry %d:\n",i);


// POST-ALU2 Buffer
	memset(temp,0,sizeof(temp));
	for(i=0;i<globBuffs.postALU2Buff.noOfElements;i++)                               	
	{
		PrintInstruction(globBuffs.postALU2Buff.indexes[0],temp);
		fprintf(ofp_trace,"Post-ALU2 Queue: [%s]\n\n",temp);
	}
	for(;i<globBuffs.postALU2Buff.maxAllowed;i++)
        	fprintf(ofp_trace,"Post-ALU2 Queue:\n\n");



// REGISTER AND DATA Values
	PrintGPRValues();
	PrintDataValues();
}


void PipelineExecute()
{
	int exFlag = FALSE;
	int cycles=0;
	int preIssueBuffsize;
	InitBuffers();
	while(globFlags.totalPipeline != COMPLETE)
	{
		cycles++;
		
		preIssueBuffsize = globBuffs.preIssueBuff.noOfElements; 
		globals.executedInstruction=EMPTY;
	
		//Write-Back
		if(globFlags.writeback1UnitFlag != COMPLETE)
			WriteBackUnit(1); 

		//Memory
		if(globFlags.memUnitFlag != COMPLETE)
			MemoryUnit();  	

		//ALU
		if(globFlags.aluUnitFlag != COMPLETE)
			ALUUnit();

		//Issue
		if(globFlags.issueUnitFlag != COMPLETE)
			IssueUnit();

		//Fetch
		if(globFlags.fetchUnitFlag != COMPLETE)
			FetchUnit(preIssueBuffsize);
		else if(globFlags.fetchUnitFlag == COMPLETE && globals.exitFlag== TRUE)
			break;
		else if(globFlags.fetchUnitFlag == COMPLETE && globals.exitFlag != TRUE)
		{
			globals.exitFlag=TRUE;
			globals.executedInstruction=HASH_INDEX;
		}
		else
			globals.executedInstruction=HASH_INDEX;

		if(globFlags.writeback2UnitFlag != COMPLETE)
			WriteBackUnit(2);
		
		SetFlags();
		PrintCycles(cycles);

	}
}


