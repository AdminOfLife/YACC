#ifndef PREDEF_H
#define PREDEF_H


#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Globals.h"
#include "Definitions.h"


/* Macros and const string variable declarations. */
#define BOOLEAN_KEY "boolean"
#define CHAR_KEY "char"
#define INTEGER_KEY "integer"
#define REAL_KEY "real"

/* Global variables. */
struct preDefTypeSymbols {
	Symbol *boolean;
	Symbol *chars;
	Symbol *integer;
	Symbol *real;	
};


/* Non-C99 compliant function prototypes. */


/* Function declarations. */
Symbol *getPreDefBool(struct preDefTypeSymbols*);
Symbol *getPreDefChar(struct preDefTypeSymbols*);
Symbol *getPreDefInt(struct preDefTypeSymbols*);
Symbol *getPreDefReal(struct preDefTypeSymbols*);
struct Symbol *createPreDef(char *name, type_t type);
struct preDefTypeSymbols *initializePredDefs();

#endif
