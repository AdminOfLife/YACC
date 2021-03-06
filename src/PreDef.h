#ifndef PREDEF_H
#define PREDEF_H


#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Globals.h"
#include "Definitions.h"


/* Macros and const string variable declarations. */

//pre-defined types:
#define BOOLEAN_KEY "Boolean"
#define CHAR_KEY "char"
#define INTEGER_KEY "integer"
#define REAL_KEY "real"
#define STRING_KEY "string"

//pre-defined constants:
#define TRUE_KEY "true"
#define FALSE_KEY "false"
#define MAXINT_KEY "maxint"
#define ASC_MAX_INT 2147483647

//pre-defined procedures:
#define WRITE "write"
#define WRITELN "writeln"
#define READ "read"
#define READLN "readln"

//pre-defined functions:
#define ODD "odd"
#define ABS "abs"
#define SQR "sqr"
#define SQRT "sqrt"
#define SIN "sin"
#define EXP "exp"
#define LN "ln"
//type coercion
#define TRUNC "trunc"
#define ROUND "round"
#define ORD "ord"
#define CHR "chr"
// ordinal operations
#define SUCC "succ"
#define PRED "pred"



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
Symbol *getPreDefString(struct preDefTypeSymbols*);
struct Symbol *createPreDefType(char *name, type_t type);
struct preDefTypeSymbols *initializePreDefTypes();
struct Symbol *createPreDefProc(char *name); 
struct Symbol *createPreDefFunc(char *name); 
int initializePreDefProc();
int initializePreDefFunc();
int initializePreDefConstants();
Symbol *createNewBoolConst(char*, int);
Symbol *createNewIntConst(char*, int);
#endif
