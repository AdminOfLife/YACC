#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Definitions.h"
#include "ElementArray.h"
#include "Error.h"
#include "Globals.h"
#include "Hash.h"
#include "PreDef.h"
#include "Type.h"
#include "SymbolAll.h"
#include "Utils.h"
#include "ActionsAll.h"
#include "Emit.h"

#ifndef TESTBUILD
#include "parser.tab.h"	/* token definitions used in operator compat checks */
#endif
#ifdef TESTBUILD
#include "tokens.h"
#endif

/*For error reporting:*/
extern int yylineno;
extern int colno;
static char *errMsg;


/*
 * Encountered 'continue' statement. Verify we're in a while loop and then
 * continue to next iteration.
 */
void continueLoop(void) {
	if (whileLoopDepth <= 0) {
		errMsg = customErrorString("A continue statement is only "
		    "allowed inside a while loop.");
		recordError(errMsg, yylineno, colno, SEMANTIC);
	}

	emitGotoLoopTop();
}

/*
 * Encountered 'exit' statement. Verify we're in a while loop and then
 * exit innermost loop.
 */
void exitLoop(void) {
	if (whileLoopDepth <= 0) {
		errMsg = customErrorString("An exit statement is only "
		    "allowed inside a while loop.");
		recordError(errMsg, yylineno, colno, SEMANTIC);
	}

	emitGotoLoopEnd();
}

/*
 * Beginning of a While loop. Called from tokens.l
 */
void beginWhileLoop(void) {
	whileLoopDepth++;

	/* expr code will be emitted after this */
	emitBeginWhile();
}

/*
 * Check whether the expr given as the loop condition is true or false
 */
void whileLoopCondCheck(ProxySymbol *expr) {
	/* make sure we have boolean expression */
	if ((expr == NULL) || (getType(expr) != BOOLEAN_T)) {
		errMsg = customErrorString("Loop conditions must be ",
		    "boolean type.");
		recordError(errMsg, yylineno, colno, SEMANTIC);
	}

	emitWhileLoopCondCheck(expr);	
}


/*
 * Return to the top of the current loop
 */
void gotoLoopTop() {
	emitGotoLoopTop();
}


/*
 * End of while loop.
 */
void endWhileLoop(void) {
	whileLoopDepth--;

	emitEndWhile();
}


/*
 * Confirm that exp is of boolean type.
 */
void ifPart(ProxySymbol *expr) {
	/* make sure we have boolean expression */
	if ((expr == NULL) || (getType(expr) != BOOLEAN_T)) {
		errMsg = customErrorString("If statement must be given ",
		    "boolean type expression.");
		recordError(errMsg, yylineno, colno, SEMANTIC);
	}

	/* expression code already emitted */
	emitIfStat(expr);
}

/*
 * THEN part of IF-THEN
 */
void thenStatPart() {
	emitThenStat();
}

/*
 * THEN part of IF-THEN-ELSE
 */
void thenMatchedStatPart() {
	/* stat part already emitted */
	emitThenMatchedStat();
}

/*
 * ELSE part of IF-THEN-ELSE
 */
void elseStatPart() {
	/* stat part already emitted */
	emitElseStat();
}
