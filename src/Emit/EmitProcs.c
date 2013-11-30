/*
 * Module implementing functions generating asc code for 
 * procedures and functions.
 */

#include "EmitProcs.h"


/*
 * Emit code to push procedure/function call onto the the stack.
 *
 * Parameters: 	param: parameter symbol
 *				paramNum: the parameter's position number in the 
 *					parameter list.
 * 	
 * Returns: void
 */
void emitProcOrFuncDecl(Symbol *symbol, struct ElementArray *ea) {
 	CHECK_CAN_EMIT(symbol);

 	emitComment("");
 	emitComment("Procedure/Function start: %s", symbol->name);

 	/* Emit procedure label */
 	emitStmt(STMT_LEN, symbol->name); 
 	
 	/* Foreach parameter, grab local variable */
	for (int i = 0; i < ea->nElements; i++) {
		emitProcParam(getElementAt(ea, i), i);
	}
}


/*
 * Emit code to push local procedure/function parameters onto the
 * the stack.
 *
 * Parameters: 	param: parameter symbol
 *				paramNum: the parameter's position number in the 
 *					parameter list.
 * 	
 * Returns: void
 */
void emitProcParam(Symbol *param, int paramNum) {
	CHECK_CAN_EMIT(param);	

	/* Get real offset. i.e. Take the parameter number
	and account for the PC and display reg in offset.
	We use 3 because first parameter is at index 0. */
	int offset = (paramNum + 3) * -1;

	/* Push local varaible onto the stack */
	emitPushParmVarValue(param, offset, param->lvl);
}


/*
 * Emit code to end a procedure. Adjusts for any local variables
 * and returns.
 *
 * Parameters: void.
 * 	
 * Returns: void
 */
void emitEndProc() {
	/* we don't have a symbol pointer, so just pass in non-null */
	CHECK_CAN_EMIT(1);	

	/* Get the lexical so we can idenity the display register */
	int lexLevel = getCurrentLexLevel(symbolTable);

	/* Determine how many levels on the stack we need to adjust by */
	int adjustCount = adjustCounter[lexLevel] * -1;

	/* Adjust and return */
	emitStmt(STMT_LEN, "ADJUST %d", adjustCount);
	emitStmt(STMT_LEN, "RET %d", lexLevel);

	resetAdjustCounter();

	emitComment("Procedure/Function end.");
	emitComment("");
	emitComment("");
}
