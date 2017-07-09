/*Guillermo Alicea
**Homework 2 - Lexical Analyzer
**06/19/16
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXIDLENGTH 11
#define MAXDIGITS 5

//array of strings that we will use when checking for reserved strings
const char *reservedAndSpecial[34] =
{
	"\0", "null", "ident", " num", "+", "-", "*", "/", "odd", "=", "<>", "<", "<=",
	">", ">=", "(", ")", ",", ";", ".", ":=", "begin", "end", "if", "then",
	"while", "do", "call","const", "var", "procedure", "write", "read",
	"else"
};

//global char used for going through the input
char inputChar;

int lexError = 0;

//self explanatory: node in a linked list that holds the lex and id of each token
typedef struct node
{
	int lex;
	struct node *next;
	char id[MAXIDLENGTH];
} node;

//function prototypes
int lexicalAnalyzer(int printLexemes);

node *newNode(int newLex, char *id);

void outputCleanInput(FILE *input);

int reservedWordCheck(char *string);

int specialSymbolCheck(char *string);

char *emptyString(char *string, int length);

node *firstIsLetter(FILE *cleanInput, node *temp, char *tempString);

node *firstIsDigit(FILE *cleanInput, node *temp, char *tempString);

node *firstIsSpecialSymbol(FILE *cleanInput, node *temp, char *tempString);

node *outputParserInput(node *root, int count);

node *outputLexemeTable(FILE *cleanInput, node *root);

node *printLexemeList(node *root);

node *destroyList(node *root);

//run lexical analyzer (print lexemes once analysis is done if we were given the
//proper directive)
int lexicalAnalyzer(int printLexemes)
{
	FILE *input; //used to read commented input
	
	//check to make sure input.txt exists
	if ((input = fopen("input.txt", "r")) == NULL)
	{
		printf("Error 35: input.txt not found, Compilation process has stopped.\n"); //input.txt not found
		
		exit(1)
;	}
	FILE *cleanInput; //used to output clean input

	outputCleanInput(input);

	cleanInput = fopen("cleaninput.txt", "r");

	//call our main lexeme-table-building function
	node *root = NULL;
	root = outputLexemeTable(cleanInput, root);

	//print lexemes
	if (printLexemes)
	{
		root = printLexemeList(root);

		printf("\n");
	}

	//free our linked list
	root = destroyList(root);

	fclose(input);

	fclose(cleanInput);
	return lexError;
}

//simple node creation function
node *newNode(int newLex, char *id)
{
	node *root = malloc(sizeof(node));

	root->lex =  newLex;

	root->next = NULL;

	strcpy(root->id, id);

	return root;
}

//clean input.txt of any comments and output it into cleaninput.txt
void outputCleanInput(FILE *input)
{
	FILE *output = fopen("cleaninput.txt", "w");

	char holder1, holder2;

	//holder1 will hold the initial '/' that begins a comment
	//if we get '*' in holder 2 then we are definitely in a comment
	//ignore everything up until the next '*', then break out of the comment
	//if we find a '/'
	while (fscanf(input, "%c", &holder1) == 1)
	{
		if (holder1 == '/')
		{
			fscanf(input, "%c", &holder2);

			if (holder2 == '*')
			{
				while (1)
				{
					fscanf(input, "%c", &holder1);

					if (holder1 == '*')
					{
						while (1)
						{
							fscanf(input, "%c", &holder2);

							if (holder2 == '/')
								break;
						}

						break;
					}
				}
			}
			//special case that holder2 is actually a number
			else if(isdigit(holder2))
				fprintf(output, "%c%d", holder1, (holder2 - '0'));
			else
				fprintf(output, "%c%c", holder1, holder2);

			continue;
		}

		fprintf(output, "%c", holder1);
	}

	fclose(output);
}

//check if string is a reserved word
int reservedWordCheck(char *string)
{
	int i;

	for (i = 0; i < 34; i++)
	{
		if (strcmp(string, reservedAndSpecial[i]) == 0)
			return i;
	}

	return -1;
}

//check if string is a special symbol
int specialSymbolCheck(char *string)
{
	int i;

	for (i = 3; i < 21; i++)
	{
		if (strcmp(string, reservedAndSpecial[i]) == 0)
			return i;
	}

	return -1;
}

//empty a string manually
//necessary in order to ensure that tempString is completely empty, asctime
//our analysis will be incorrect if it is not when it is supposed to be
char *emptyString(char *string, int length)
{
	int i = 0;

	for (i = 0; i < length; i++)
		string[i] = '\0';

	return string;
}

//case that inputchar is a letter
node *firstIsLetter(FILE *cleanInput, node *temp, char *tempString)
{
	int counter = 1, holder;

	strcpy(tempString, emptyString(tempString, 100));

	tempString[0] = inputChar;

	//create tempString according to what we get as input
	while (1)
	{
		if (fscanf(cleanInput, "%c", &inputChar) == EOF)
			inputChar = '\n';

		//we have an identifier
		if (isalpha(inputChar) || isdigit(inputChar))
		{
			tempString[counter] = inputChar;

			counter++;

			if(counter > MAXIDLENGTH)
				break;
		}
		else
			break;
	}

	if (counter > MAXIDLENGTH)
	{
		while (isdigit(inputChar) || isalpha(inputChar))
			if(fscanf(cleanInput, "%c", &inputChar) != 1)
			{
				inputChar = '\n';

				break;
			}
			else
				counter++;

			printf("Error 29: Name is too long (%d characters, max is 11)\n", (counter - 1));

			lexError = 1;

		return temp;
	}
	//we have a reserved word
	if ((holder = reservedWordCheck(tempString)) != -1)
	{
		temp->next = newNode(holder, tempString);

		temp = temp->next;
	}
	//identifier that is acceptable and can be added to our list
	else
	{
		temp->next = newNode(2, tempString);

		temp = temp->next;
	}

	return temp;
}

//case that inputchar is a digit
node *firstIsDigit(FILE *cleanInput, node *temp, char *tempString)
{
	int counter = 1;

	strcpy(tempString, emptyString(tempString, 100));

	tempString[0] = inputChar;

	while (1)
	{
		if (fscanf(cleanInput, "%c", &inputChar) == EOF)
			inputChar = '\n';

		if (isdigit(inputChar))
		{
			tempString[counter] = inputChar;

			counter++;

			if(counter > MAXDIGITS)
				break;
		}
		else
		{
			//identifier that starts with number (i.e. 5name)
			if (isalpha(inputChar))
			{
				printf("Error 30: Name does not start with letter\n");

				lexError = 1;

				while (isalpha(inputChar) || isdigit(inputChar))
					if(fscanf(cleanInput, "%c", &inputChar) == EOF)
					{
						inputChar = '\n';

						break;
					}

				return temp;
			}

			break;
		}
	}
	//number is too big
	if (counter > MAXDIGITS)
	{
		while (isdigit(inputChar))
			if(fscanf(cleanInput, "%c", &inputChar) == EOF)
			{
				inputChar = '\n';

				break;
			}
			else
				counter++;

		printf("Error 31: Number is too long (%d digits, max is 5)\n", (counter - 1));

		lexError = 1;

		return temp;
	}

	//if we got this far then we have an acceptable number - let's add it to our list
	temp->next = newNode(3, tempString);

	temp = temp->next;

	return temp;
}

//case that input char is a special symbol or ":"
node *firstIsSpecialSymbol(FILE *cleanInput, node *temp, char *tempString)
{
	int holder;

	strcpy(tempString, emptyString(tempString, 100));

	tempString[0] = inputChar;

	//here, we either have an error or ":="
	if (inputChar == ':')
	{
		if (fscanf(cleanInput, "%c", &inputChar) == EOF)
			inputChar = '\n';

		tempString[1] = inputChar;

		if((holder = specialSymbolCheck(tempString)) != -1)
		{
			temp->next = newNode(holder, tempString);

			temp = temp->next;
		}
		else
		{
			printf("Error 32: : is an invalid symbol\n");

			lexError = 1;
		}
		return temp;
	}
	//we need this condition in order to account for <=, <>, and >=
	else if (inputChar == '<' || inputChar == '>')
	{
		if (fscanf(cleanInput, "%c", &inputChar) == EOF)
			inputChar = '\n';

		if (inputChar == '>' || inputChar == '=')
			tempString[1] = inputChar;

		holder = specialSymbolCheck(tempString);

		temp->next = newNode(holder, tempString);

		temp = temp->next;

		return temp;
	}

	//if we got here then we have a special symbol that doesn't meet our special
	//conditions - let's add it
	holder = specialSymbolCheck(tempString);

	temp->next = newNode(holder, tempString);

	temp = temp->next;

	return temp;
}

//main loop that will take input from cleaninput.txt and create our linked
//list by calling the functions corresponding to whatever is stored into inputchar
node *outputLexemeTable(FILE *cleanInput, node *root)
{
    FILE *output1 = fopen("lexemetable.txt", "w");

		FILE *output2 = fopen("lexemelist.txt", "w");

		char tempString[100];

		int holder, width, i, count = 0;

		root = newNode(-1, "\0");

		node *temp = root;

		fscanf(cleanInput, "%c", &inputChar);

		while (1)
    {
			//if inputChar is a letter, let's treat is as a possible ident or reserved word
			if(isalpha(inputChar))
				temp = firstIsLetter(cleanInput, temp, tempString);

			//else if it's a number, let's treat it like one
			else if (isdigit(inputChar))
				temp = firstIsDigit(cleanInput, temp, tempString);

			//ok, let's check if it's a special symbol then
			if (((holder = specialSymbolCheck(&inputChar)) != -1) || inputChar == ':')
				temp = firstIsSpecialSymbol(cleanInput, temp, tempString);

			//because of how our specialSymbolCheck is set up to check for <>, <=, and >=
			//we need to make sure that we're not overlooking something (this could happen if our input code
			//is not spaced very well)
			if (isalpha(inputChar) || isdigit(inputChar))
					continue;
			//nothing overlooked so far, let's check if it's a symbol that we can ignore
			else if(inputChar == ' ' || inputChar == '\t' || inputChar == '\n')
				;
			//no? ok, if it's a special symbol, let's continue and reiterate like normally, else
			//it's an invalid symbol, since we've already checked for everything else
			else
			{
				if (specialSymbolCheck(&inputChar) != -1)
						;
				else
				{
					printf("Error 33: %c is an invalid symbol\n", inputChar);

					lexError = 1;
				}
			}

			if (fscanf(cleanInput, "%c", &inputChar) == EOF)
				break;
    }

    temp = root->next;

		fprintf(output1, "lexeme        token type\n");

		//output lex table and lex list, and increment count so we have an idea
		//of how many tokens we have (used in parser as safeguard)
		while (temp)
    {
			count++;

			width = MAXIDLENGTH - strlen(temp->id);

			fprintf(output1, "%s   ", temp->id);

			for(i = 0; i < width; i++)
				fprintf(output1, " ");

			fprintf(output1, "%d\n", temp->lex);

			if (temp->lex == 2)
				fprintf(output2, "2 %s ", temp->id);
			else if (temp->lex == 3)
				fprintf(output2, "3 %s ", temp->id);
			else
				fprintf(output2, "%d ", temp->lex);

			temp = temp->next;
    }

	root = outputParserInput(root, count);

	fclose(output1);

	fclose(output2);

	return root;
}

//this is the output that we will use in our parser as input -
//it pretty much outputs each token's id and tokensym (each tokensym's value
//can be found in parser.h)
node *outputParserInput(node *root, int count)
{
	FILE *ptr = fopen("parserInput.txt", "w");

	node *temp = root->next;

	fprintf(ptr, "%d\n", count);

	while(temp)
	{
		fprintf(ptr, "%s   ", temp->id);

		fprintf(ptr, "%d%c", temp->lex, ((temp->next != NULL) ? '\n' : ' '));

		temp = temp->next;
	}

	fclose(ptr);

	return root;
}

//this is our function to print our lexemelist to the screen if we get the proper
//directive
node *printLexemeList(node *root)
{
	node *temp = root->next;

	while (temp)
	{
		if (temp->lex == 2)
			printf("2 %s ", temp->id);
		else if (temp->lex == 3)
			printf("3 %s ", temp->id);
		else
			printf("%d ", temp->lex);

		temp = temp->next;
	}

	printf("\n");
	
	return root;
}

//standard recursive function to free a linkedlist
node *destroyList(node *root)
{
	if (root == NULL)
		return NULL;

	destroyList(root->next);

	free(root);

	return NULL;
}
