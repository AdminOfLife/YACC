#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ElementArray.h"
#include "Error.h"
#include "ErrorLL.h"
#include "Type.h"
#include "Hash.h"
#include "PreDef.h"
#include "SymbolAll.h"

/* macro used in getTypeSym */
#define assertKindPtr(ptr) if (ptr == NULL) {\
    err(EXIT_FAILURE, "kindPtr is NULL");}

/* Creates const kind symbol using scope of global symbol table.
 *
 * Parameters:
 *              id: name of symbol
 *
 * Return: Newly created symbol
 */
Symbol *createConstSymbol(char *id) {
	return createSymbol(symbolTable, id, CONST_KIND, TYPEORIGINATOR_NO);
}


/* Creates func kind symbol using scope of global symbol table.
 *
 * Parameters:
 *              id: name of symbol
 *
 * Return: Newly created symbol
 */
Symbol *createFuncSymbol(char *id) {
	return createSymbol(symbolTable, id, FUNC_KIND, TYPEORIGINATOR_NO);
}


/* Creates Param kind symbol using scope of global symbol table.
 *
 * Parameters:
 *              id: name of symbol
 *
 * Return: Newly created symbol
 */
Symbol *createParamSymbol(char *id) {
	return createSymbol(symbolTable, id, PARAM_KIND, TYPEORIGINATOR_NO);
}


/* Creates proc kind symbol using scope of global symbol table.
 *
 * Parameters:
 *              id: name of symbol
 *
 * Return: Newly created symbol
 */
Symbol *createProcSymbol(char *id) {
	return createSymbol(symbolTable, id, PROC_KIND, TYPEORIGINATOR_NO);
}


/* Creates var kind symbol using scope of global symbol table.
 *
 * Parameters:
 *              id: name of symbol
 *
 * Return: Newly created symbol
 */
Symbol *createVarSymbol(char *id) {
	return createSymbol(symbolTable, id, VAR_KIND, TYPEORIGINATOR_NO);
}


/* Creates type kind symbol using scope of global symbol table.
 *
 * Parameters:
 *              id: name of symbol
 *		typeOriginator: flag for if type originator 
 *
 * Return: Newly created symbol
 */
Symbol *createTypeSymbol(char *id, int typeOriginator) {
	return createSymbol(symbolTable, id, TYPE_KIND, typeOriginator);
}

/* Returns pointer to base type symbol of the subrange symbol
 *
 * Parameters:
 * 		s: subrange symbol
 *
 * Return: a pointer to the type symbol
 */
Symbol * getSubrangeBaseTypeSym(Symbol *sr) {
	if (!sr) return NULL;
	if (getType(sr) != SUBRANGE_T) return NULL;
	return getTypePtr(sr)->Subrange->baseTypeSym;
}


/* Frees passed proxy symbol
 *
 * Parameters:
 * 		p: proxy symbol
 *
 * Return: void
 */
void freeProxySymbol(ProxySymbol *p) {
	free(p);
}

/*
 * Checks whehter the given CONST_KIND symbol is a result of 
 * constant calculation or not.
 *
 * Parameters:
 *		ProxySymbol *s: MUST BE a CONST_KIND symbol !!!
 *	
 * Returns:  1: It is a result of const calculation.
 *			0: It is not.
 */
int isConstResultSym(ProxySymbol *s){
	assertKindPtr(s->kindPtr.ConstKind);
	return s->kindPtr.ConstKind->constResultFlag;
}

/* 
 * set the constResult flag to 1 
 * 
 * Parameters:
 *		ProxySymbol *s: MUST BE a CONST_KIND symbol !!!
 */
void setConstResultFlag(ProxySymbol *s){
	assertKindPtr(s->kindPtr.ConstKind);
	s->kindPtr.ConstKind->constResultFlag = 1; 
}
/*
 * Returns a pointer to the Symbol of TYPE_KIND defining the type for the
 * given CONST_KIND, FUNC_KIND, PROC_KIND, or VAR_KIND symbol.
 * Parameters:
 * 	s: the symbol for which to return the type pointer
 * Returns:
 * 	NULL if symbol has no type, or s for Symbols of TYPE_KIND.
 */
Symbol *getTypeSym(Symbol *s) {
	if (!s) return NULL; 
	switch (s->kind) {
	case CONST_KIND:
		assertKindPtr(s->kindPtr.ConstKind);
		return s->kindPtr.ConstKind->typeSym;
	case PARAM_KIND:
		assertKindPtr(s->kindPtr.ParamKind);
		return s->kindPtr.ParamKind->typeSym;
	case PROC_KIND: 
		/* Procedures do not have associated type symbols */
		return NULL;
	case FUNC_KIND:
		assertKindPtr(s->kindPtr.FuncKind);
		return s->kindPtr.FuncKind->typeSym;
	case TYPE_KIND:
		assertKindPtr(s->kindPtr.TypeKind);
		return s;
	case VAR_KIND:
		assertKindPtr(s->kindPtr.VarKind);
		return s->kindPtr.VarKind->typeSym;
	default:
		err(EXIT_FAILURE, "Unknown kind set on given symbol");
		return NULL;
	}
}

/*
 * Returns 1 if the symbol (which is kind PARAM_KIND or VAR_KIND) has been
 * passed by reference (e.g., the byRef flag in its kindPtr is set)
 * Parameters
 *		s: the symbol for which we would are inquiring if it has been
 *		    passed by reference
 * Return: 1 if the conditions above are met, 0 if not	
 */
int isByReference(Symbol *s)
{
	if (!s) return 0;
	
	if ((s->kind == PARAM_KIND)) return s->kindPtr.ParamKind->byRef;
	if (s->kind == VAR_KIND) return s->kindPtr.VarKind->byRef;
	return 0;
}

/*
 * Adds the given symbol to the symbol table supplied.
 *
 * This is a wrapper around createHashElement that also sets the
 * symbols size, since when we add to the hash table we know all the elements
 * needed to set the size.
 *      
 * Parameters: 
 *              table: hash table for element to be created in
 *              symbol: pointer to symbol to add to hash table.
 *
 * Returns: 0 on success 
 *         1 current lex level and symbol's lex level differ
 *         2 element on hash table has symbol set to NULL
 *         3 element's symbol list head lex level same as symbol's
 *         4 symbol's lex level lower than element's symbol list head    
 *         5 could not create a name for null key
*/
int addToSymbolTable(struct hash *table, Symbol *symbol) {
	int retval = 0;

	retval = createHashElement(table, symbol->name, symbol);

	/* For symbol which are record types we do not call setSymbolSize()
	 * as at this point in the program we have lost the information we 
	 * need to accureately calculate size */

	setSymbolSize(symbol);

	return retval;
}
