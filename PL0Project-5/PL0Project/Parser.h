/*
*Guillermo Alicea
*Homework 3 - PL/0 Parser (Bonus Assignment)
*07/27/16
*
*Added parameter and return functionality for procedures,
*symbol table now stores number of parameters for procedures (test with -s),
*and fixed write's ebnf. Please see input.txt for an example parameter passing
*recursive factorial program
*
*Added: global int stackSize, functions paramList and paramBlock,
*if-else if conditions in gen(), gen(INC) after call in factor(), and a few other
*miscellaneous changes
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//macros for our code safeguards and setting up for parsing
#define MAX_SYMBOL_TABLE_SIZE 100
#define MAX_CODE_LENGTH 500
#define MAXIDLENGTH 11
#define MAXDIGITS 5

//macros for our tokensyms
#define     IDENTSYM 2
#define    NUMBERSYM 3
#define      PLUSSYM 4
#define     MINUSSYM 5
#define      MULTSYM 6
#define     SLASHSYM 7
#define       ODDSYM 8
#define       EQLSYM 9
#define       NEQSYM 10
#define       LESSYM 11
#define       LEQSYM 12
#define       GTRSYM 13
#define       GEQSYM 14
#define   LPARENTSYM 15
#define   RPARENTSYM 16
#define     COMMASYM 17
#define SEMICOLONSYM 18
#define    PERIODSYM 19
#define    BECOMESYM 20
#define     BEGINSYM 21
#define       ENDSYM 22
#define        IFSYM 23
#define      THENSYM 24
#define     WHILESYM 25
#define        DOSYM 26
#define      CALLSYM 27
#define     CONSTSYM 28
#define       VARSYM 29
#define      PROCSYM 30
#define     WRITESYM 31
#define      READSYM 32
#define      ELSESYM 33

//macros for our instruction ops
#define          LIT 1
#define          OPR 2
#define          LOD 3
#define          STO 4
#define          CAL 5
#define          INC 6
#define          JMP 7
#define          JPC 8
#define          SIO 9
#define          EQL 8
#define          NEQ 9
#define          LSS 10
#define          LEQ 11
#define          GTR 12
#define          GEQ 13

//recommended struct
typedef struct symbol
{
	int kind;
	char name[12];
	int value;
	int level;
	int addr;
	int param;
} symbol;

typedef struct outputTable
{
	symbol outputSymbolTable[MAX_SYMBOL_TABLE_SIZE];
	int index;
} outputTable;

//houses each token's ident and tokensym (taken directly from the output of
//lexicalanalyzer)
typedef struct tokenStruct
{
	int tokenSym;
	char tokenIdent[12];
} tokenStruct;

//houses our instructions' details
typedef struct instruction
{
	int op;
	int l;
	int m;
} instruction;

//houses our upper and lower index, which will be used throughout the parsing process and
//scopeHandler, which is an array that we will use to house identifiers declared
//in procedures (which will be deleted once we exit those procedures)
typedef struct scopeHandle
{
	int index;
	int sx;
	char scopeHandler[MAX_SYMBOL_TABLE_SIZE][MAXIDLENGTH];
} scopeHandle;

//globals
int tokenCount; //total tokencount (used in gettoken as a safeguard)

int cx = 0; //code index

int semiColonSafeGuard = 0; //guard against ; at eof

int stackSize = 0;

tokenStruct token; //current token

scopeHandle handle; //struct used to house our scope handling array and lower limit
					//for each corresponding procedure

outputTable symbolTableOutput; //holds every symbol declared throughout the parsing process, in order


//function prototypes
void parser(int printMCode, int printSymbolTable);

void createTokenList(char *fileName);

void getToken(FILE *input);

void enter(int constEnt, int type, char *name, int l, int addr, int value, symbol symbolTable[], int param);

void enterOutput(int constEnt, int type, char *name, int l, int addr, int value, int numParams);

void delete(symbol symbolTable[], int level);

void error(int errorValue);

void gen(int op, int l, int m, instruction code[]);

void outputInstructions(instruction code[]);

void printInstructions(instruction code[]);

void printSymbolTableFoo();

int findOprM(int sym);

int find(char *ident, symbol symbolTable[], int level);

void program(FILE *input, symbol symbolTable[], instruction code[]);

void block(FILE *input, int level, symbol symbolTable[], instruction code[], int numParams);

void constDecl(FILE *input, int level, symbol symbolTable[], instruction code[]);

int varDecl(FILE *input, int level, symbol symbolTable[], instruction code[], int numParams);

void procDecl(FILE *input, int level, symbol symbolTable[], instruction code[]);

int paramBlock(FILE *input, int level, symbol symbolTable[], instruction code[]);

void paramList(FILE *input, int level, symbol symbolTable[], instruction code[], int index);

void statement(FILE *input, int level, symbol symbolTable[], instruction code[]);

void condition(FILE *input, int level, symbol symbolTable[], instruction code[]);

void expression(FILE *input, int level, symbol symbolTable[], instruction code[]);

void term(FILE *input, int level, symbol symbolTable[], instruction code[]);

void factor(FILE *input, int level, symbol symbolTable[], instruction code[]);

//here we will set up for the parsing and code generation portion of the compiler
void parser(int printMCode, int printSymbolTable)
{
	FILE *input = fopen("parserInput.txt", "r");

	//we will make this array twice the max requirement in order to facilitate nicer
	//hashes
  symbol symbolTable[MAX_SYMBOL_TABLE_SIZE*2];

	//our code array
  instruction code[MAX_CODE_LENGTH];

  int i = 0;

	fscanf(input, "%d", &tokenCount);

	//we will make use of this characteristic in our enter function
  for (i = 0; i < MAX_SYMBOL_TABLE_SIZE*2; i++)
		symbolTable[i].kind = -1;

	//parse/generate code
	program(input, symbolTable, code);

	//output our mcode (for virtual machine) and symbol table
	outputInstructions(code);

	if(printMCode)
		printInstructions(code);

	if (printSymbolTable)
		printSymbolTableFoo();

	printf("No errors, program is syntactically correct\n");

  fclose(input);
}

//simply get the next token's ident and tokensym in the file
void getToken(FILE *input)
{
	//this should probably be an error, but we will just return and output the
	//first error we encounter (this should only ever happen if an error exists in
	//the code, which will be raised when token is not changed)
	if (tokenCount == 0)
		return;

  char string[12];

  if(fscanf(input, "%s", string) == EOF)
		return;

	strcpy(token.tokenIdent, string);

  if(fscanf(input, "%d", &token.tokenSym) == EOF)
		return;

  tokenCount--;

}

//inserts the token into our hash table, performing linear probing to avoid collision
//(linear should suffice given our hash function is not terrible and our table has a nice size)
//and inserting based on whether we want to insert a const or a var/proc
void enter(int constEnt, int kind, char *name, int l, int addr, int value, symbol symbolTable[], int param)
{
	int hash = ((name[0] + name[strlen(name) - 1])*name[0]) % 200;

	if (symbolTable[hash].kind == -1)
		;
	else
		while (symbolTable[++hash].kind != -1)
			;

	//const insert
	if (constEnt)
	{
		strcpy(symbolTable[hash].name, name);

    symbolTable[hash].kind = kind;

		symbolTable[hash].level = l;

    symbolTable[hash].value = value;
	}
	//var/proc insert
	else
	{
		strcpy(symbolTable[hash].name, name);

		symbolTable[hash].kind = kind;

		symbolTable[hash].level = l;

		symbolTable[hash].addr = addr;

		symbolTable[hash].param = param;
	}
}

void enterOutput(int constEnt, int kind, char *name, int l, int addr, int value, int numParams)
{
	//const insert
	if (constEnt)
	{
		strcpy(symbolTableOutput.outputSymbolTable[symbolTableOutput.index].name, name);

    symbolTableOutput.outputSymbolTable[symbolTableOutput.index].kind = kind;

		symbolTableOutput.outputSymbolTable[symbolTableOutput.index].level = l;

    symbolTableOutput.outputSymbolTable[symbolTableOutput.index++].value = value;
	}
	//var/proc insert
	else
	{
		strcpy(symbolTableOutput.outputSymbolTable[symbolTableOutput.index].name, name);

		symbolTableOutput.outputSymbolTable[symbolTableOutput.index].kind = kind;

		symbolTableOutput.outputSymbolTable[symbolTableOutput.index].level = l;

		symbolTableOutput.outputSymbolTable[symbolTableOutput.index].addr = addr;

		symbolTableOutput.outputSymbolTable[symbolTableOutput.index++].param = numParams;
	}
}
//we will use this function to delete symbols that are declared within proc
void delete(symbol symbolTable[], int level)
{
	int i;

	int index;

	for (i = handle.index; i < handle.sx; i++)
	{
		index = find(handle.scopeHandler[i], symbolTable, level);

		strcpy(symbolTable[index].name, "");
		symbolTable[index].kind = -1;
		symbolTable[index].value = -1;
		symbolTable[index].level = -1;
		symbolTable[index].addr = -1;

		strcpy(handle.scopeHandler[i], "");
	}

	handle.sx = handle.index;
}

//output the error corresponding to errorValue, which varies based on where the
//error is encountered, and then exit since we do not want to continue the parsing/
//code generation process
void error(int errorValue)
{
	switch (errorValue)
	{
		case 1:

			printf("Error 1: No period at end of file\n");

			exit(1);

		case 2:

			printf("Error 2: Missing identifier after const\n");

			exit(1);

		case 3:

			printf("Error 3: Use = instead of :=\n");

			exit(1);

		case 4:

			printf("Error 4: Consts should be assigned using =\n");

			exit(1);

		case 5:

			printf("Error 5: = should be followed by number\n");

			exit(1);

		case 6:

			printf("Error 6: Declaration must end with ;\n");

			exit(1);

		case 7:

			printf("Error 7: Missing identifier after var\n");

			exit(1);

		case 8:

			printf("Error 8: Missing identifier after procedure\n");

			exit(1);

		case 9:

			printf("Error 9: Procedure declaration must end with ;\n");

			exit(1);

		case 10:

			printf("Error 10: No ; at end of block\n");

			exit(1);

		case 11:

			printf("Error 11: := missing in statement\n");

			exit(1);

		case 12:

			printf("Error 12: Missing identifier after call\n");

			exit(1);

		case 13:

			printf("Error 13: Begin must be closed with end (check your ;'s)\n");

			exit(1);

		case 14:

			printf("Error 14: If condition must be followed by then\n");

			exit(1);

		case 15:

			printf("Error 15: While condition must be followed by do\n");

			exit(1);

		case 16:

			printf("Error 16: Missing identifier after read\n");

			exit(1);

		case 17:

			printf("Error 17: Missing identifier after write\n");

			exit(1);

		case 18:

			printf("Error 18: Relational operator missing in conditional statement\n");

			exit(1);

		case 19:

			printf("Error 19: Left ( has not been closed\n");

			exit(1);

		case 20:

			printf("Error 20: Identifier, (, or number expected\n");

			exit(1);

		case 21:

			printf("Error 21: Attempting to use undeclared identifier\n");

			exit(1);

		case 22:

			printf("Error 22: := assignment to constant or procedure is not allowed\n");

			exit(1);

		case 23:

			printf("Error 23: Call to const or var is meaningless\n");

			exit(1);

		case 24:

			printf("Error 24: Read must be followed by a var\n");

			exit(1);

		//case 25 used to be missing ident after write, however that was a mistake

		case 26:

			printf("Error 26: Instruction count has exceeded maximum instructions permitted\n");

			exit(1);

		case 27:

			printf("Error 27: Program ends with ; and not end.\n");

			exit(1);

		case 28:

			printf("Error 28: Use := instead of =\n");

			exit(1);

		case 29:

			printf("Error 34: Consts, Vars, and Procs must be declared before statements\n");

			exit(1);

		case 30:

			printf("Error 36: Identifiers cannot be re declared in the same scope\n");

			exit(1);

		case 31:

			printf("Error 37: Procedure must have paramters\n");

			exit(1);

		case 32:

			printf("Error 38: Identifier expected in procedure's parameter list\n");

			exit(1);

		case 33:

			printf("Error 39: Missing parameter list at procedure call\n");

			exit(1);

		case 34:

			printf("Error 40: Bad calling format\n");

			exit(1);

		case 35:

			printf("Error 41: Missing identifier after call in expression\n");

			exit(1);

		case 36:

			printf("Error 42: Attempting to pass too many paramters in procedure call\n");

			exit(1);

		case 37:

			printf("Error 43: Not enough parameters passed in procedure call\n");

			exit(1);

	}
}

//insert an instruction ,and its values, into our code then increment
//our code index
void gen(int op, int l, int m, instruction code[])
{
	if (cx >= MAX_CODE_LENGTH)
		error(26); //instruction count has exceed maximum instructions permitted

	code[cx].op = op;

	code[cx].l = l;

	code[cx].m = m;

	//we want to change our stackSize according to the instructions we generate.
	//the pdf said to set our stackSize to 0 after call, sto, and sio but the only
	//case we should do this is when we are returning, i think (most likely
	//this could be based on a different implementation, but in this case we
	//will only set stackSize to 0 when we return from a procedure)
	if (op == LIT || op == LOD || op == SIO+1)
        stackSize++;
	else if (op == JPC || op == STO || op == SIO)
        stackSize--;
	else if (op == INC)
        stackSize += m;
	else if (op == OPR)
	{
        if (m == 0)
            stackSize = 0;
        else if (m != 6 && m != 1)
            stackSize--;
    }

	cx++;
}

//find a particular symbol in our symbolTable based on its ident
//determing the symbol that has the correct value for level that we are looking
//for makes this function look a lot nastier than it really is
int find(char *ident, symbol symbolTable[], int level)
{
	int i, j;
	int hash = ((ident[0] + ident[strlen(ident) - 1])*ident[0]) % 200;

	//our symbol is not in the symbolTable since our first hash is empty
	if (symbolTable[hash].kind == -1)
		return -1;
	//our hash contains a symbol with our ident
	else if (strcmp(symbolTable[hash].name, ident) == 0)
	{
		//now we need to find the symbol with the right level
		//which will be the one with the highest level value
		//unfortunately, higher levels will be inserted after those with lower levels
		//so we must check directly
		for (i = level; i >= 0; i--)
		{
			for (j = hash; j < hash + MAX_SYMBOL_TABLE_SIZE*2; j++)
			{
				if (strcmp(symbolTable[j%(MAX_SYMBOL_TABLE_SIZE*2)].name, ident) != 0)
						break;
				else if (strcmp(symbolTable[j%(MAX_SYMBOL_TABLE_SIZE*2)].name, ident) == 0
						&& symbolTable[j%(MAX_SYMBOL_TABLE_SIZE*2)].level == i)
						break;
			}
			if (strcmp(symbolTable[j%(MAX_SYMBOL_TABLE_SIZE*2)].name, ident) == 0
				&& symbolTable[j%(MAX_SYMBOL_TABLE_SIZE*2)].level == i)
				break;
		}

        return j;
  }
	//hash conflict -- loop until we find our ident or we get to an empty slot (missing ident)
	else
		while (symbolTable[++hash].kind != -1 && strcmp(symbolTable[hash].name, ident) != 0)
			;
	//same process as earlier
	if(symbolTable[hash].kind != -1 && strcmp(symbolTable[hash].name, ident) == 0)
    {
			for (i = level; i >= 0; i--)
			{
				for (j = hash; j < hash + MAX_SYMBOL_TABLE_SIZE*2; j++)
				{
					if (strcmp(symbolTable[j%(MAX_SYMBOL_TABLE_SIZE*2)].name, ident) != 0)
							break;
					else if (strcmp(symbolTable[j%(MAX_SYMBOL_TABLE_SIZE*2)].name, ident) == 0
							&& symbolTable[j%(MAX_SYMBOL_TABLE_SIZE*2)].level == i)
							break;
				}
				if (strcmp(symbolTable[j%(MAX_SYMBOL_TABLE_SIZE*2)].name, ident) == 0
					&& symbolTable[j%(MAX_SYMBOL_TABLE_SIZE*2)].level == i)
					break;
			}

        return j;
    }
		//ident not found since we checked all our options
    else
			return -1;
}

//begin parsing process according to EBNF of pl/0
//EBNF of program
void program(FILE *input, symbol symbolTable[], instruction code[])
{
	getToken(input);

	handle.index = handle.sx = 0;

	block(input, 0, symbolTable, code, 0);

	if (token.tokenSym != PERIODSYM)
	error(1); //no period at end of file

	//generate our sio halt at the end of our code
	//this could also be opr 0 0 (return from main) but i chose to go with this
	//because hw1 test cases used it
	gen(SIO+2, 0, 3, code);
}

//EBNF of block
void block(FILE *input, int level, symbol symbolTable[], instruction code[], int numParams)
{
	int ctemp = cx, varCount = 0;

	//generate our initial jump to main's begin (change m later)
	gen(JMP, 0, 0, code);

	if (token.tokenSym == CONSTSYM)
		constDecl(input, level, symbolTable, code);

	if (token.tokenSym == VARSYM)
		varCount = varDecl(input, level, symbolTable, code, numParams);

	if (token.tokenSym == PROCSYM)
		procDecl(input, level, symbolTable, code);

	//updating our first JMP's m since we are now in program's statement
	code[ctemp].m = cx;

	//inc 4 (fv, sl, dl, ra) + however many variables we declared + however many
	//parameters our procedure has (main has 0 by default)
	gen(INC, 0, 4 + varCount + numParams, code);

	statement(input, level, symbolTable, code);

	//this condition doesn't really matter since we would be at the end of the program
	//when we reach this point with level == 0, but i would rather have it here to
	//emphasize the scope of variables declared before procedures -- global
	if (level != 0)
		delete(symbolTable, level);

}

//EBNF of const
void constDecl(FILE *input, int level, symbol symbolTable[], instruction code[])
{
	char ident[12];
	int number;
	int i;

	//enter all the consts into our symbolTable
	while (1)
	{
		getToken(input);

		if (token.tokenSym != IDENTSYM)
			error(2); //missing identifier after const

		for (i = handle.index; i < handle.sx; i++)
			if (strcmp(handle.scopeHandler[i], token.tokenIdent) == 0)
				error(30);

		strcpy(ident, token.tokenIdent);

		//add to our list of symbols to delete when we're done with this procedure
		//and increment sx
		strcpy(handle.scopeHandler[handle.sx++], token.tokenIdent);

		getToken(input);

		if (token.tokenSym != EQLSYM)
		{
			if (token.tokenSym == BECOMESYM)
				error(3); //use = instead of :=
			else
				error(4); //const should be assigned using =
		}

		getToken(input);

		if (token.tokenSym != NUMBERSYM)
			error(5); //= should be followed by number

		number = atoi(token.tokenIdent);

		enter(1, CONSTSYM, ident, level, 0, number, symbolTable, 0);

		enterOutput(1, CONSTSYM, ident, level, 0, number, 0);

		getToken(input);

		if (token.tokenSym != COMMASYM)
			break;
	}

	if (token.tokenSym != SEMICOLONSYM)
	error(6); //declaration must end with ;

	getToken(input);
}

//EBNF of var
int varDecl(FILE *input, int level, symbol symbolTable[], instruction code[], int numParams)
{
	int varCount = 0;
	int i;

	//enter all the vars into our symbolTable
	while (1)
	{
		getToken(input);

		if (token.tokenSym != IDENTSYM)
			error(7); //missing identifier after var

		for (i = handle.index; i < handle.sx; i++)
			if (strcmp(handle.scopeHandler[i], token.tokenIdent) == 0)
				error(30);

		enter(0, VARSYM, token.tokenIdent, level, 4 + numParams + varCount++, 0, symbolTable, 0);

		enterOutput(0, VARSYM, token.tokenIdent, level, 4 + (varCount - 1) + numParams, 0, 0);

		//add to our list of symbols to delete when we're done with this procedure
		//and increment sx
		strcpy(handle.scopeHandler[handle.sx++], token.tokenIdent);

		getToken(input);

		if (token.tokenSym != COMMASYM)
			break;
	}

	if (token.tokenSym != SEMICOLONSYM)
	error(6); //declaration must end with ;

	getToken(input);

	//used for our gen(INC) a little later in block()
	return varCount;
}

//EBNF of procedure
void procDecl(FILE *input, int level, symbol symbolTable[], instruction code[])
{
	int indexTemp;
	int i;
	int numParams = 0;

	//enter all our procedures into the symbol table (after dealing with each
	//of their blocks)
	while (token.tokenSym == PROCSYM)
	{
		getToken(input);

		if (token.tokenSym != IDENTSYM)
			error(8); //missing identifier after procedure

		for (i = handle.index; i < handle.sx; i++)
			if (strcmp(handle.scopeHandler[i], token.tokenIdent) == 0)
				error(30);

		enter(0, PROCSYM, token.tokenIdent, level, cx, 0, symbolTable, 0);

		strcpy(handle.scopeHandler[handle.sx++], token.tokenIdent);

		getToken(input);

		//we don't want to delete our global variables, or in the case of nested procedures,
		//the variables of the "parent" procedure; in order to do this, we need to hold
		//the value of our index before we make calls to nested procedures
		indexTemp = handle.index;

		//now set the lower limit equal to the upper limit (by the time we delete,
		//sx will be higher than handle.index by the number of entries made to symbol table)
		handle.index = handle.sx;

		numParams = paramBlock(input, level, symbolTable, code);

		symbolTable[(i = find(handle.scopeHandler[handle.index - 1], symbolTable, level))].param = numParams;

		enterOutput(0, PROCSYM, symbolTable[i].name, level, cx, 0, numParams);

		if (token.tokenSym != SEMICOLONSYM)
			error(9); //procedure declaration must end with ;

		enter(0, VARSYM, "return", level + 1, 0, 0, symbolTable, 0);

		getToken(input);

		//handle the block of each procedure with a level of level+1
		block(input, level + 1, symbolTable, code, numParams);

		//update our index to what it was before we went into our nested procedures
		handle.index = indexTemp;

		//RTN
		gen(OPR, 0, 0, code);

		if (token.tokenSym != SEMICOLONSYM)
			error(10); //no ; at end of block

		getToken(input);
	}
}

int paramBlock(FILE *input, int level, symbol symbolTable[], instruction code[])
{
	int numParams = 0;

	if (token.tokenSym != LPARENTSYM)
		error(31); //procedure must have parameters

	getToken(input);

	if (token.tokenSym == IDENTSYM)
	{
		//we insert it with level + 1 since we are calling this from the prior block
		enter(0, VARSYM, token.tokenIdent, level + 1, 4 + numParams++, 0, symbolTable, 0);

		//the scope that we are handling is the scope of the parameters of the procedures,
		//because a) we don't want to be able to use those variables outside of that function and b)
		//we don't want a procedure's parameter to cause an identifier conflict with a varibale in another
		//procedure -- foo1(x) and foo2(x) in the same level should be legal
		strcpy(handle.scopeHandler[handle.sx++], token.tokenIdent);

		getToken(input);

		while (token.tokenSym == COMMASYM)
		{
			getToken(input);

			if (token.tokenSym != IDENTSYM)
				error(32); //identifier expected in parameter list

			enter(0, VARSYM, token.tokenIdent, level + 1, 4 + numParams++, 0, symbolTable, 0);

			strcpy(handle.scopeHandler[handle.sx++], token.tokenIdent);

			getToken(input);
		}
	}

	if (token.tokenSym != RPARENTSYM)
		error(33); //bad procedure declaration

	getToken(input);

	return numParams;
}

void paramList(FILE *input, int level, symbol symbolTable[], instruction code[], int index)
{
	int i = 0; //check we have the right number of params

	if (token.tokenSym != LPARENTSYM)
		error(33); //missing parameter list at call

	getToken(input);

	//the bonus pdf does not specify whether no parameters were allowed,
	//however i figured they are since there's no reason why they shouldn't be allowed
	if (token.tokenSym == RPARENTSYM)
	{
		if (i != symbolTable[index].param)
			error(37); //not passing enough parameters in procedure call
		else
		{
			getToken(input);
			return;
		}
	}

	expression(input, level, symbolTable, code);

	i++;

	while (token.tokenSym == COMMASYM)
	{
		getToken(input);

		expression(input, level, symbolTable, code);

		i++;
	}

	if (i < symbolTable[index].param)
		error(37); //not passing enough parameters
	else if (i > symbolTable[index].param)
		error(36); //passing too many parameters

	//gen
	//4 signifies the minimum members of the AR, while we have to decrement 1 to stackSize
	//to account for our STO decrement of stackSize. We must then increment stackSize right after
	//because we do not want these particular STO's to affect our stackSize
	for (; i > 0; i--)
		gen(STO, 0, 4 + (stackSize - 1), code);

	if (token.tokenSym != RPARENTSYM)
		error(34); //bad calling formatting

	getToken(input);
}

//EBNF of statement
void statement(FILE *input, int level, symbol symbolTable[], instruction code[])
{
	int i, ctemp1, ctemp2;

	if (token.tokenSym == IDENTSYM)
	{
		i = find(token.tokenIdent, symbolTable, level);

		if (i == -1)
			error(21); //undeclared identifier

		if (symbolTable[i].kind != VARSYM)
			error(22); //assignment to constant or procedure is not allowed

		getToken(input);

		if (token.tokenSym != BECOMESYM)
		{
			if (token.tokenSym == EQLSYM)
				error(28); //use := instead of =
			else
				error(11); //:= missing in statement
		}
		getToken(input);

		//we are assigning something to our var -- let's handle the expression
		expression(input, level, symbolTable, code);

		//expression() has generated code for the assignment, now let's STO it
		gen(STO, level - symbolTable[i].level, symbolTable[i].addr, code);
	}
	else if (token.tokenSym == CALLSYM)
	{
		getToken(input);

		if (token.tokenSym != IDENTSYM)
			error(12); //missing identifier after call

		i = find(token.tokenIdent, symbolTable, level);

		if (i == -1)
			error(21); //undeclared identifier

		if (symbolTable[i].kind != PROCSYM)
			error(23); //call to const or var is meaningless

		getToken(input);

		paramList(input, level, symbolTable, code, i);

		//gen a CAL
		gen(CAL, level - symbolTable[i].level, symbolTable[i].addr, code);
	}
	else if (token.tokenSym == BEGINSYM)
	{
		getToken(input);

		//handle begin's statement
		statement(input, level, symbolTable, code);

		//if there is a ; then we have more statements
		//I realize now that a ; at the end of a begin block (and before the end)
		//is optional as we would then get an end, call statement(), and return with no changes
		while (token.tokenSym == SEMICOLONSYM)
		{
			getToken(input);

			statement(input, level, symbolTable, code);

			if (semiColonSafeGuard++ == 1000)
				error(27); //program ends with ; and not "end."
		}

		semiColonSafeGuard = 0;

		if (token.tokenSym != ENDSYM)
		{
			if (token.tokenSym == VARSYM || token.tokenSym == CONSTSYM || token.tokenSym == PROCSYM)
				error(29); //consts, vars, and procs must be declared before statements
			else
				error(13); //begin must be closed with end
		}

		getToken(input);
	}
	else if (token.tokenSym == IFSYM)
	{
		getToken(input);

		//handle if's condition
		condition(input, level, symbolTable, code);

		if (token.tokenSym != THENSYM)
			error(14); //if condition must be followed by then

		getToken(input);

		//store index of our JPC
		ctemp1 = cx;

		gen(JPC, 0, 0, code);

		statement(input, level, symbolTable, code);

		//store index of our JMP
		ctemp2 = cx;

		gen(JMP, 0, 0, code);

		//update JPC (we have dealt with the statement)
		code[ctemp1].m = cx;

		//JMP will be updated correctly, whether we have an else or not
		if (token.tokenSym == ELSESYM)
		{
			getToken(input);

			statement(input, level, symbolTable, code);
		}

		//update JMP (we have dealt with our else, or not if there wasn't one)
		code[ctemp2].m = cx;
	}
	else if (token.tokenSym == WHILESYM)
	{
		//store index for our JMP later on
		ctemp1 = cx;

		getToken(input);

		//handle while's condition
		condition(input, level, symbolTable, code);

		//store index of JPC
		ctemp2 = cx;

		gen(JPC, 0, 0, code);

		if (token.tokenSym != DOSYM)
			error(15); //while condition must be followed by do

		getToken(input);

		//handle while's statement
		statement(input, level, symbolTable, code);

		gen(JMP, 0, ctemp1, code);

		//update our JPC
		code[ctemp2].m = cx;
	}
	else if (token.tokenSym == READSYM)
	{
		getToken(input);

		if (token.tokenSym != IDENTSYM)
			error(16); //missing identifier after read

		i = find(token.tokenIdent, symbolTable, level);

		if (i == -1)
			error(21); //undeclared identifier

		if (symbolTable[i].kind != VARSYM)
			error(24); //read must be followed by a var

		//10 0 2
		gen(SIO+1, 0, 2, code);

		//STO into var
		gen(STO, level - symbolTable[i].level, symbolTable[i].addr, code);

		getToken(input);
	}
	else if (token.tokenSym == WRITESYM)
	{
		//apparently i assumed the EBNF of write was the same as read, and so this
		//block was almost identical to that of READSYM (handling only identifiers).
		//after realizing my mistake, i changed it to handle expressions, which turns out
		//to be a lot nicer
		getToken(input);

		expression(input, level, symbolTable, code);

		gen(SIO, 0, 1, code);
	}
}

//EBNF of condition
void condition(FILE *input, int level, symbol symbolTable[], instruction code[])
{
	int i;

	if (token.tokenSym == ODDSYM)
	{
		getToken(input);

		//handle expression
		expression(input, level, symbolTable, code);

		//2 0 6
		gen(OPR, 0, 6, code);
	}
	else
	{
		//handle expression
		expression(input, level, symbolTable, code);

		//check if token is a relational operator
		if (token.tokenSym != EQLSYM &&
			token.tokenSym != NEQSYM &&
			token.tokenSym != LESSYM &&
			token.tokenSym != LEQSYM &&
			token.tokenSym != GTRSYM &&
			token.tokenSym != GEQSYM )
			error(18); //relational operator missing in conditional statement

		i = findOprM(token.tokenSym);

		getToken(input);

		expression(input, level, symbolTable, code);

		//2 0 %d, depending on what our relation is
		gen(OPR, 0, i, code);
	}
}

//EBNF of expression
void expression(FILE *input, int level, symbol symbolTable[], instruction code[])
{
	int addop;

	if (token.tokenSym == PLUSSYM || token.tokenSym == MINUSSYM)
	{
		addop = token.tokenSym;

		getToken(input);

		//handle term
		term(input, level, symbolTable, code);

		//NEG
		if (addop == MINUSSYM)
			gen(OPR, 0, 1, code);
	}
	else
		term(input, level, symbolTable, code);

	//keep dealing with + or - until they stop
	while (token.tokenSym == PLUSSYM || token.tokenSym == MINUSSYM)
	{
		addop = token.tokenSym;

		getToken(input);

		//handle term
		term(input, level, symbolTable, code);

		//2 0 2
		if (addop == PLUSSYM)
			gen(OPR, 0, 2, code);
		//2 0 3
		else
			gen(OPR, 0, 3, code);
	}
}

//EBNF of term
void term(FILE *input, int level, symbol symbolTable[], instruction code[])
{
	int mulop;

	//handle factor
	factor(input, level, symbolTable, code);

	//keep dealing with * or / until they stop
	while (token.tokenSym == MULTSYM || token.tokenSym == SLASHSYM)
	{
		mulop = token.tokenSym;

		getToken(input);

		//handle factor
		factor(input, level, symbolTable, code);

		//2 0 4
		if (mulop == MULTSYM)
			gen(OPR, 0, 4, code);
		//2 0 5
		else
			gen(OPR, 0, 5, code);
	}
}

//EBNF of factor and the base of our recursion
void factor(FILE *input, int level, symbol symbolTable[], instruction code[])
{
	int i;

	if (token.tokenSym == IDENTSYM)
	{
		i = find(token.tokenIdent, symbolTable, level);

		if (i == -1)
			error(21); //undeclared identifier

		//LOD our var
		if (symbolTable[i].kind == VARSYM)
			gen(LOD, level - symbolTable[i].level, symbolTable[i].addr, code);
		//we have a const, use LIT
		else if (symbolTable[i].kind == CONSTSYM)
			gen(LIT, 0, symbolTable[i].value, code);
		else
			error(22); //cannot use a procedure in assignment

		getToken(input);
	}
	else if (token.tokenSym == NUMBERSYM)
	{
		//number is stored as a string - we need it as an int
		i = atoi(token.tokenIdent);

		//number - definitely use LIT
		gen(LIT, 0, i, code);

		getToken(input);
	}
	else if (token.tokenSym == LPARENTSYM)
	{
		getToken(input);

		//handle ( <expression> )
		expression(input, level, symbolTable, code);

		if (token.tokenSym != RPARENTSYM)
			error(19); //left ( has not been closed

		getToken(input);
	}
	else if (token.tokenSym == CALLSYM)
	{
		getToken(input);

		if (token.tokenSym != IDENTSYM)
			error(35); //missing identifier after call in expression

		i = find(token.tokenIdent, symbolTable, level);

		if (i == -1)
			error(21); //undeclared identifier

		if (symbolTable[i].kind != PROCSYM)
			error(23); //call to const or var is meaningless

		getToken(input);

		paramList(input, level, symbolTable, code, i);

		//gen a CAL
		gen(CAL, level - symbolTable[i].level, symbolTable[i].addr, code);

		//gen an INC
		gen(INC, 0, 1, code);
	}
	else
		error(20); //identifier, (, or number expected
}

//very simple - go through out code[] one by one and output the contents
//for our virtual machine
void outputInstructions(instruction code[])
{
	int i;
	FILE *ptr = fopen("mcode.txt", "w");

	for(i = 0; i < cx; i++)
		fprintf(ptr, "%d %d %d\n", code[i].op, code[i].l, code[i].m);

	fclose(ptr);
}

void printInstructions(instruction code[])
{
	int i;

	for(i = 0; i < cx; i++)
		printf("%d %d %d\n", code[i].op, code[i].l, code[i].m);

	printf("\n");
}

//this will output symbolTable to the screen
//it is very simple but looks nasty because of formatting (which may have a more
//concise solution).
//symbol table output into symboltable.txt is not required in the hw4 assignment, so i decided to
//add it as a directive instead. It could easily be printed to symboltable.txt using the same template
void printSymbolTableFoo()
{
	int i, j;
	int space = 0; //how many spaces we need depends on how big our output is

	printf("ident         type   value  level  address  params\n");

	for (i = 0; i < symbolTableOutput.index; i++)
	{
		if (symbolTableOutput.outputSymbolTable[i].kind != -1)
		{
			if (symbolTableOutput.outputSymbolTable[i].kind == CONSTSYM)
			{
				printf("%s", symbolTableOutput.outputSymbolTable[i].name);

				space = MAXIDLENGTH - strlen(symbolTableOutput.outputSymbolTable[i].name);

				for (j = 0; j < space; j++)
					printf(" ");

				printf("   const  %d", symbolTableOutput.outputSymbolTable[i].value);

				space = MAXDIGITS - symbolTableOutput.outputSymbolTable[i].value/10;

				for (j = 0; j < space; j++)
					printf(" ");

				printf("   %d      -         -\n", symbolTableOutput.outputSymbolTable[i].level);
			}
			else if (symbolTableOutput.outputSymbolTable[i].kind == VARSYM)
			{
				printf("%s", symbolTableOutput.outputSymbolTable[i].name);

				space = MAXIDLENGTH - strlen(symbolTableOutput.outputSymbolTable[i].name);

				for (j = 0; j < space; j++)
					printf(" ");

				printf("   var    -        %d      %d         -\n", symbolTableOutput.outputSymbolTable[i].level, symbolTableOutput.outputSymbolTable[i].addr);
			}
			else
			{
				printf("%s", symbolTableOutput.outputSymbolTable[i].name);

				space = MAXIDLENGTH - strlen(symbolTableOutput.outputSymbolTable[i].name);

				for (j = 0; j < space; j++)
					printf(" ");

				printf("   proc   -        %d      %d         %d\n", symbolTableOutput.outputSymbolTable[i].level, symbolTableOutput.outputSymbolTable[i].addr,
																	symbolTableOutput.outputSymbolTable[i].addr);
			}
		}
	}

	printf("\n");
}

//find the m that corresponds to our relational operator
int findOprM(int sym)
{
	switch (sym)
	{
		case EQLSYM:
			return EQL;
		case NEQSYM:
			return NEQ;
		case LESSYM:
			return LSS;
		case LEQSYM:
			return LEQ;
		case GTRSYM:
			return GTR;
		case GEQSYM:
			return GEQ;
	}
	return 0;
}
