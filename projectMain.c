/* 
   “On my honor, I have neither given nor received unauthorized aid on this assignment"

   Course       : Comp-Arc
   Project Name : MIPS simulator
*/

#include "projectMgmt.c"

int main(int argv,char **argc)
{
	char un[MAXNOOFINS][MAXLENGTH],username[MAXLENGTH];
	int i;
	FILE *fp;

	if(argv>=2)
	{
		fp=fopen(argc[1],"r");
		if (fp == NULL) {
 		 	fprintf(stderr, "Can't open input file in.list!\n");
  			exit(1);
			}
	}
	else
	{
		printf("Input File Missing :( \n");
		return -1;
	}
	
	SimulatorGlobalValInit();

	while(!feof(fp))
	{
		username[0]='\0';
		if(fscanf(fp,"%s",username)==1)
		{
			strcpy(un[globals.numOfTotalInstructions],username);
			globals.numOfTotalInstructions++;
		}
	}
	
	/*Dynamic Memory allocation*/	
	globals.insNumbersFrmt=(int *)calloc(globals.numOfTotalInstructions,sizeof(int));
	insDecodeStructure = (InstructionLine*)malloc( sizeof(InstructionLine)* globals.numOfTotalInstructions);
	globals.numOfTotalInstructions--;
		

	/*Management Code Begins */
	ConvertIntoNumber(un);  
	ofp_assm=fopen("disassembly.txt","w");
	ofp_trace=fopen("simulation.txt","w");
	MapInstructions();
	PrintAssembly(un);
	PipelineExecute();
	//ExecuteCycles();
clean:
	fclose(fp);	
	fclose(ofp_assm);
	fclose(ofp_trace);
	return 0;
}
