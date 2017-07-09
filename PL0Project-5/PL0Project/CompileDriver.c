/*Guillermo Alicea
** Homework 4 - Compile Driver
**07/23/16
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "VirtualMachine.h"
#include "LexicalAnalyzer.h"
#include "Parser.h"

int main(int argc, char **argv)
{
	int printLexemes = 0;
	int printMCode = 0;
	int printSymbolTable = 0;
	int i = 1;

	//if we have at least one directive, let's find out what they are
	if (argc > 1)
	{
		for (; i < argc; i++)
			//if it's -l, print lexemes, else if it's -a, print mcode, else we ignore 
			//it since it is not a recognized directive
			if (strcmp("-l", argv[i]) == 0)
				printLexemes = 1;
			else if (strcmp("-m", argv[i]) == 0)
				printMCode = 1;
			else if (strcmp("-s", argv[i]) == 0)
				printSymbolTable = 1;
			else
				printf("Directive %s not recognized: directive ignored\n", argv[i]);
	}
	
	//lexical analyzer (hw2), taking into account whether or not we should print
	if(!lexicalAnalyzer(printLexemes))
	{	
		//parser (hw3)
		parser(printMCode, printSymbolTable);
	
		//virtual machine (hw1)
		//won't check for errors directly, since we exit prematurely if an error
		//is found while parsing
		virtualMachine();
	}
	else
		printf("Lexical error found: Compilation stopped\n");

	return 0;
}
