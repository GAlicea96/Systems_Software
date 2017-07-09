/*Guillermo Alicea
**Homework 1 - PM/0 Virtual Machine
**05/30/16
*/

#include <stdio.h>
#include <stdlib.h>

//macros for input restrictions
#define MAX_STACK_HEIGHT 2000
#define MAX_CODE_LENGTH 500
#define MAX_LEXI_LEVEL 3

//our instructions based on our ISA
const char *ISA[12] =
{
	"ignore", "LIT", "OPR", "LOD",
	"STO", "CAL", "INC", "JMP",
	"JPC", "SIO", "SIO", "SIO"
};

//house our instruction (identical to the struct used in parser.h)
typedef struct Instruction
{
	int op;
	int l;
	int m;
} Instruction;

//essentially an array of instructions, except of course we also store the size
//of our array
typedef struct InstructionList
{
	Instruction *instructionArray;
	int size;
} InstructionList;

//struct that houses the details of our stack trace
typedef struct StackTrace
{
	int pc;
	int bp;
	int sp;
	int halt; //halt when we find 11 0 3
	int depth; //how far down are we? when we rtn from 0 we need to stop
	int stack[MAX_STACK_HEIGHT];
	int bars[MAX_STACK_HEIGHT]; //bars for our activation records
} StackTrace;

//function prototypes
void virtualMachine();

InstructionList readInput(char *fileName, InstructionList list);

void outputAssemblyLanguage(FILE *ptr, InstructionList list);

int base(int l, int base, StackTrace stackTrace);

StackTrace Opr(Instruction instruction, StackTrace stackTrace);

StackTrace executeInstruction(Instruction instruction, StackTrace stackTrace);

void printStackTrace(FILE *ptr, StackTrace stackTrace, Instruction instruction);

//run our virtual machine
void virtualMachine()
{
	FILE *ptr = fopen("stacktrace.txt", "w");

	//our list is complete after this returns - we are now ready to proceed
	InstructionList list = readInput("mcode.txt", list);

	//instructions to be printed
	Instruction printInstruction;

	//self explanatory
	StackTrace stackTrace;

	int i = 0;

	//output first part of our overall output (easy)
	outputAssemblyLanguage(ptr, list);

	//initialization
	printInstruction.op = -1; //to skip first if in while

	stackTrace.pc = 0;

	stackTrace.bp = 1;

	stackTrace.sp = 0;

	stackTrace.halt = 0;

	stackTrace.depth = 0;

	for(i = 0; i < MAX_STACK_HEIGHT; i++)
	{
		stackTrace.stack[i] = 0;
		stackTrace.bars[i] = 0;
	}

	fprintf(ptr, "                          pc   bp    sp    stack\n");

	fprintf(ptr, "Initial Values             0    1     0\n");

	//main loop that will terminate if we return from main or if we find 11 0 3
	while (1)
	{
		if (printInstruction.op != -1)
			printStackTrace(ptr, stackTrace, printInstruction);

		//get our instruction
		printInstruction = list.instructionArray[stackTrace.pc];

		fprintf(ptr, "%3d   ", stackTrace.pc);

		//execute instruction
		stackTrace = executeInstruction(list.instructionArray[stackTrace.pc],
																			stackTrace);

		//return from main - print our final line and break
		if ((stackTrace.depth == -1 &&
				(printInstruction.op == 2 && printInstruction.m == 0)))
		{
			printStackTrace(ptr, stackTrace, printInstruction);

			break;
		}
		//same as before except we break because we found 11 0 3
		if (stackTrace.halt == 1)
		{
			fprintf(ptr, "SIO    %d   %4d\n", printInstruction.l, printInstruction.m);

			fprintf(ptr, "Successfully Halted.");

			break;
		}
	}

	//clean up
	free(list.instructionArray);

	fclose(ptr);
}

//read "mcode.txt" prepared for us by parser.h
InstructionList readInput(char *fileName, InstructionList list)
{
	FILE *ptr = fopen(fileName, "r");

	int numInstructions = 0, holder = 0, i = 0;

	//go through and find out how many instr. we have so we can malloc
	//and set up a nice for loop
	while (fscanf(ptr, "%d", &holder) == 1)
			numInstructions++;

	//go back to start
	rewind(ptr);

	list.instructionArray = malloc(sizeof(Instruction)*(numInstructions/3));

	//numInstructions is maybe badly named - we have numInstructions/3 instructions
	//we are use mod in the loop to get the components of each (numInstructions/3)
	for (i = 0; i < numInstructions; i++)
	{
		fscanf(ptr, "%d", &holder);

		if (i % 3 == 0)
			list.instructionArray[i/3].op = holder;
		else if (i % 3 == 1)
			list.instructionArray[i/3].l = holder;
		else
		{
			list.instructionArray[i/3].m = holder;
			list.size++;
		}
	}

	fclose(ptr);

	return list;
}

//output our assembly language (first part of overall output)
//simple and self explanatory
void outputAssemblyLanguage(FILE *ptr, InstructionList list)
{
	int i = 0;

	fprintf(ptr, "Line    OP    L    M\n");

	for (i = 0; i < list.size; i++)
	{
		fprintf(ptr, "%3d    ", i);

		fprintf(ptr, "%s    ", ISA[list.instructionArray[i].op]);

		fprintf(ptr, "%d    ", list.instructionArray[i].l);

		fprintf(ptr, "%d\n", list.instructionArray[i].m);
	}

	fprintf(ptr, "\n");
}

//function to find our base
//provided by whoever originally wrote it
int base(int l, int base, StackTrace stackTrace)
{
	int b1;

	b1 = base;

	while (l > 0)
	{
		b1 = stackTrace.stack[b1 + 1];

		l--;
	}

	return b1;
}

//we have this in the case that we have an OPR instruction
//which would mean we have to find which instruction to execute based on m
//corresponds very closely to the ISA provided in hw1.pdf
StackTrace Opr(Instruction instruction, StackTrace stackTrace)
{
	switch (instruction.m)
	{
		//RTN
		case 0 :

			stackTrace.sp = stackTrace.bp - 1;

			stackTrace.pc = stackTrace.stack[stackTrace.sp + 4] - 1;

			stackTrace.bars[stackTrace.bp] = 0;

			stackTrace.bp = stackTrace.stack[stackTrace.sp + 3];

			stackTrace.depth--;

			if (stackTrace.depth == -1)
				stackTrace.pc = -1;

			break;
		//NEG
		case 1 :

			stackTrace.stack[stackTrace.sp] = -stackTrace.stack[stackTrace.sp];

			break;
		//ADD
		case 2 :

			stackTrace.sp--;

			stackTrace.stack[stackTrace.sp] = stackTrace.stack[stackTrace.sp] +
																			stackTrace.stack[stackTrace.sp + 1];

			break;
		//SUB
		case 3 :

			stackTrace.sp--;

			stackTrace.stack[stackTrace.sp] = stackTrace.stack[stackTrace.sp] -
																			stackTrace.stack[stackTrace.sp + 1];

		 break;
		//MUL
		case 4 :

			stackTrace.sp--;

			stackTrace.stack[stackTrace.sp] = stackTrace.stack[stackTrace.sp] *
																			stackTrace.stack[stackTrace.sp + 1];

			break;
		//DIV
		case 5 :

			stackTrace.sp--;

			stackTrace.stack[stackTrace.sp] = stackTrace.stack[stackTrace.sp] /
																			stackTrace.stack[stackTrace.sp + 1];

			break;
		//ODD
		case 6 :

			stackTrace.stack[stackTrace.sp] = stackTrace.stack[stackTrace.sp % 2];

			break;
		//MOD
		case 7 :

			stackTrace.sp--;

			stackTrace.stack[stackTrace.sp] = stackTrace.stack[stackTrace.sp] %
																			stackTrace.stack[stackTrace.sp + 1];

			break;
		//EQL
		case 8 :

			stackTrace.sp--;

			stackTrace.stack[stackTrace.sp] = (stackTrace.stack[stackTrace.sp]
																			== stackTrace.stack[stackTrace.sp + 1]);

			break;
		//NEQ
		case 9 :

			stackTrace.sp--;

			stackTrace.stack[stackTrace.sp] = (stackTrace.stack[stackTrace.sp]
																			!= stackTrace.stack[stackTrace.sp + 1]);

			break;
		//LSS
		case 10 :

			stackTrace.sp--;

			stackTrace.stack[stackTrace.sp] = (stackTrace.stack[stackTrace.sp]
																			 < stackTrace.stack[stackTrace.sp + 1]);

			break;
		//LEQ
		case 11 :

			stackTrace.sp--;

			stackTrace.stack[stackTrace.sp] = (stackTrace.stack[stackTrace.sp]
																			<= stackTrace.stack[stackTrace.sp + 1]);

			break;
		//GTR
		case 12 :

			stackTrace.sp--;

			stackTrace.stack[stackTrace.sp] = (stackTrace.stack[stackTrace.sp]
																			 > stackTrace.stack[stackTrace.sp + 1]);

			break;
		//GEQ
		case 13 :

			stackTrace.sp--;

			stackTrace.stack[stackTrace.sp] = (stackTrace.stack[stackTrace.sp]
																			>= stackTrace.stack[stackTrace.sp + 1]);

			break;
	}

	return stackTrace;
}

//determine which instruction to execute based on our op
//corresponds very closely to the ISA provided in hw1.pdf
StackTrace executeInstruction(Instruction instruction, StackTrace stackTrace)
{
	switch (instruction.op)
	{
		//LIT
		case 1 :

			stackTrace.sp++;

			stackTrace.stack[stackTrace.sp] = instruction.m;

			break;
		//OPR
		case 2 :

			stackTrace = Opr(instruction, stackTrace);

			break;
		//LOD
		case 3 :

			stackTrace.sp++;

			stackTrace.stack[stackTrace.sp] =

					stackTrace.stack[base(instruction.l,
							stackTrace.bp, stackTrace) + instruction.m];

			break;
		//STO
		case 4 :

			stackTrace.stack[base(instruction.l,
					stackTrace.bp, stackTrace) + instruction.m] =
					stackTrace.stack[stackTrace.sp];

			stackTrace.sp--;

			break;
		//CAL
		case 5 :

			stackTrace.stack[stackTrace.sp + 1] = 0;

			stackTrace.stack[stackTrace.sp + 2] = base(instruction.l,
																					stackTrace.bp, stackTrace);

			stackTrace.stack[stackTrace.sp + 3] = stackTrace.bp;

			stackTrace.stack[stackTrace.sp + 4] = stackTrace.pc + 1;

			stackTrace.bp = stackTrace.sp + 1;

			stackTrace.pc = instruction.m;

			stackTrace.bars[stackTrace.bp] = 1;

			stackTrace.depth++;

			return stackTrace;
	  //INC
	  case 6 :

			stackTrace.sp = stackTrace.sp + instruction.m;

			break;
		//JMP
		case 7 :

			stackTrace.pc = instruction.m;

			return stackTrace;
		//JPC
		case 8 :

			if (stackTrace.stack[stackTrace.sp] == 0)
			{
					stackTrace.pc = instruction.m;

				 stackTrace.sp--;

					return stackTrace;
			}

			stackTrace.sp--;

			break;
		//WRITE
		case 9 :

			printf("%d\n", stackTrace.stack[stackTrace.sp]);

			stackTrace.sp--;

			break;
		//READ
		case 10 :

			stackTrace.sp++;

			scanf("%d", &stackTrace.stack[stackTrace.sp]);

			break;
		//HALT
		case 11 :

			stackTrace.halt = 1;

			break;
	}

	stackTrace.pc++;

	return stackTrace;
}

//output the current contents of our stackTrace
void printStackTrace(FILE *ptr, StackTrace stackTrace, Instruction instruction)
{
	int i;

	fprintf(ptr, "%s %4d %6d %6d%5d%6d    ", ISA[instruction.op], instruction.l,
					instruction.m, stackTrace.pc, stackTrace.bp, stackTrace.sp);

	for(i = 1; i <= stackTrace.sp; i++)
	{
		//bars for AR's
		if(stackTrace.bars[i] == 1)
			fprintf(ptr, "| ");

		fprintf(ptr, "%d ", stackTrace.stack[i]);
	}

	fprintf(ptr, "\n");
}
