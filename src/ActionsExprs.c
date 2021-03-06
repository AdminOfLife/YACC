#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Definitions.h"
#include "ElementArray.h"
#include "Error.h"
#include "Globals.h"
#include "Hash.h"
#include "Emit.h"
#include "PreDef.h"
#include "Type.h"
#include "SymbolAll.h"
#include "Utils.h"
#include "ActionsAll.h"
#include "Kind.h"
#include "Tree.h"


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
 * Action which performs semantic analyis and code generation with the
 * aim of assigned the symbol x the value of the proxysymbol y.
 * Parameters
 *		x : the symbol which is assigned a value
 *		y : the proxysymbol of the value to assign to x
 */
void assignOp(Symbol *x, ProxySymbol *y) {
	if (!(x) || !(y)) return;

	if ( x->kind == CONST_KIND ) {
		errMsg = customErrorString("The identifier %s is a constant "
		    " and cannot be re-assigned a value.", x->name);
		recordError(errMsg, yylineno, colno, SEMANTIC);
	} 

	if (isAssignmentCompat(x, y)) {
		emitAssignmentOp(x, y);
	}

}

/*
 * 
 */
Symbol * variableAssignmentLookup(char *id)
{
	Symbol *s = NULL;
	s = getGlobalSymbol(symbolTable, id);
	if (!s) {
		notDefinedError(id);
		return NULL;
	}
	emitComment("Pushing address of %s in preparation for assignment.",
	    s->name);
	emitPushVarAddress(s);
	return s;
}

/*
 *
 *
 */
Symbol *recordFieldAssignmentLookup(struct treeNode *node, char *id)
{
	struct treeNode *newNode;
	Symbol *s = NULL, *p;
	struct Record *r = NULL;


	p = node->symbol;
	if ((!p) || (!id)) return NULL;
	

	s = getTypeSym(p);
	if (getType(s) != RECORD_T) {
		errMsg = customErrorString("Cannot get field %s from %s. "
		    "Identifier %s is not a record.", id, p->name, p->name);
		recordError(errMsg, yylineno, colno, SEMANTIC);
		return NULL;
	}
	
	r = s->kindPtr.TypeKind->typePtr.Record;

	s = getGlobalSymbol(r->hash, id);
	if (!s) {
		errMsg = customErrorString("Field %s does not exist in %s.",
		    id, p->name);
		recordError(errMsg, yylineno, colno, SEMANTIC);
		return NULL;
	}

	emitComment("Pushing address of %s in preparation for assignment.",
	    s->name);
	newNode = createRecordNode(s, node);
	postOrderWalk(newNode);
	return newProxySymFromSym(s);
}

/*
 * Calls the function to change the address resultant from the indexing
 * operations into a value (in asc).
 * Parameters
 *		ps : the type resultant from the indexing operation, it
 *		    is simply passed up the chain.
 */
Symbol *pushArrayIndexValue(struct treeNode *node)
{
	postOrderWalk(node);
	return node->symbol;
}

struct treeNode *hashLookupToProxy(char *id) {
	Symbol *s = NULL;
	s = getGlobalSymbol(symbolTable, id);
	if (!s) {
		notDefinedError(id);
		return createLeafNode(NULL);
	}

	if (s->kind == TYPE_KIND && !(s->isAddress)) {
		errMsg = customErrorString("%s is a type and cannot be used "
		    "in expressions\n", s->name);
		recordError(errMsg, yylineno, colno, SEMANTIC);
		return createLeafNode(NULL);
	}
	return createLeafNode(newProxySymFromSym(s));
}

/*
 * Creates a new proxy symbol dependent on the field in the record type p
 * which was accessed.
 * Parameterscall
 *		p : the record to which the field id belongs
 *		id : the id of the field that is to be accessed
 */
struct treeNode *recordAccessToProxy(struct treeNode *node, char *id) {
	Symbol *s = NULL, *p = node->symbol;
	struct Record *r = NULL;

	if ((!p) || (!id)) { 
		node->symbol = NULL;
		return node;	
	}

	s = getTypeSym(p);
	if (getType(s) != RECORD_T) {
		errMsg = customErrorString("Cannot get field %s from %s. "
		    "Identifier %s is not a record.", id, p->name, p->name);
		recordError(errMsg, yylineno, colno, SEMANTIC);
		node->symbol = NULL;
		return node;	
	}
	
	r = s->kindPtr.TypeKind->typePtr.Record;

	s = getGlobalSymbol(r->hash, id);
	if (!s) {
		errMsg = customErrorString("Field %s does not exist in %s.",
		    id, p->name);
		recordError(errMsg, yylineno, colno, SEMANTIC);
		node->symbol = NULL;
		return node;	
	}

	s->isAddress = 1;
	return createRecordNode(s, node);
}


/*
 * Access an array given the list of indexes.
 *
 * Return a ProxySymbol of kind VAR_KIND of the expected type.
 */
struct treeNode *arrayIndexAccess(struct treeNode *arrayBase, 
    struct treeNode * exprNodes) {

	struct treeNode *exprNode, *arrayNode, *tmpNode, *newArrayNode;
	ProxySymbol *accessResultSym = NULL;

	if ( !(arrayBase->symbol) || !(exprNodes->symbol) ) {
		arrayBase->symbol = NULL;
		return arrayBase;	
	}

	exprNode = exprNodes;
	arrayNode = arrayBase;

	while (exprNode != NULL) {
		accessResultSym = isValidArrayAccess(arrayNode->symbol,
		    exprNode->symbol);
		if (!accessResultSym) {
			/* Memory leak - should free tree nodes */
			arrayBase->symbol = NULL;
			return arrayBase;
		}
		tmpNode = exprNode->parent;
		newArrayNode = createTreeNode(accessResultSym, ARRAY_INDEX,
		    arrayNode, exprNode);
		arrayNode = newArrayNode;
		exprNode = tmpNode; 
	}

	/* Return the root node of the tree we just constructed such that
	 * it can be walked later. */
	return newArrayNode;

//	ProxySymbol *accessResultType = NULL;

	/* Record specific errors in isValidArrayAccess */
//	if ((!indices) || (!var)) return NULL;	
//	accessResultType = isValidArrayAccess((Symbol *) var, indices);

	/* It's in here that we have to set the offsets of these proxy symbols
	 * such that the final symbol (e.g., the cumulative, non-array type
	 * symbol resultant from all the indexing)
	 */
//	return accessResultType;
}


/*
 * Concatenate two arrays of array indexes, maintaining order.
 *
 * Return a pointer to a concatenated list.
 */
struct treeNode *concatArrayIndexList(struct treeNode *baseNode, 
    struct treeNode *newExprNode) {

	struct treeNode *tmpNode;	

	if ((!(baseNode->symbol)) && (!(newExprNode->symbol))) {
		baseNode->symbol = NULL;
		return NULL;
	}
	if (!(baseNode->symbol)) {
		return newExprNode;
	}
	if (!(newExprNode->symbol)) {
		return baseNode;
	}

	tmpNode = baseNode;
	while (tmpNode->parent != NULL) tmpNode = tmpNode->parent;
	tmpNode->parent = newExprNode;
	
	return baseNode;
}


/*
 * Create a new array index list as an expression node.
 *
 * Return a pointer to the new list.
 */
struct treeNode *createArrayIndexList(struct treeNode *exp) {
	if (!exp->symbol) return exp;
	return exp;
}


/*******************************************************************************
 *
 * 		Expression operator functions follow
 *
 ******************************************************************************/

struct treeNode *eqOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL; 
	struct treeNode *node = NULL;
	
	ps = exprsOp(x->symbol, EQUAL ,y->symbol);
	node = createTreeNode(ps, EQUAL, x, y);
	
	//if (ps) emitEqualsOp(x, y);
	return node;
}


struct treeNode *notEqOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL; 
	struct treeNode *node = NULL;
	
	ps = exprsOp(x->symbol, NOT_EQUAL ,y->symbol);
	node = createTreeNode(ps, NOT_EQUAL, x, y);
	
	//if (ps) emitInequalityOp(x, y);
	return node;
}


struct treeNode *lessOrEqOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL; 
	struct treeNode *node = NULL;

	ps = exprsOp(x->symbol, LESS_OR_EQUAL ,y->symbol);
	node = createTreeNode(ps, LESS_OR_EQUAL, x, y);
	
	//if (ps) emitLTEOp(x, y);
	return node;
}


struct treeNode *lessOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL; 
	struct treeNode *node = NULL;

	ps = exprsOp(x->symbol, LESS ,y->symbol);
	node = createTreeNode(ps, LESS, x, y);
	
	//if (ps) emitLTOp(x, y);
	return node;
}


struct treeNode *gtOrEqOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL; 
	struct treeNode *node = NULL;

	ps = exprsOp(x->symbol, GREATER_OR_EQUAL, y->symbol);
	node = createTreeNode(ps, GREATER_OR_EQUAL, x, y);
	
	//if (ps) emitGTEOp(x, y);
	return node;
}


struct treeNode *gtOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL; 
	struct treeNode *node = NULL;

	ps = exprsOp(x->symbol, GREATER, y->symbol);
	node = createTreeNode(ps, GREATER, x, y);
	
	//if (ps) emitGTOp(x, y);
	return node;
}


struct treeNode *unaryPlusOp(struct treeNode *y) {
	ProxySymbol *ps = NULL; 
	struct treeNode *node = NULL;
	
	ps = exprsOp(NULL, PLUS, y->symbol);
	node = createTreeNode(ps, PLUS, createLeafNode(NULL), y);
	// TODO emit

	return node;
}

struct treeNode *unaryMinusOp(struct treeNode  *y) {
	ProxySymbol *ps = NULL;
	struct treeNode *node = NULL;
	
	ps = exprsOp(NULL, MINUS ,y->symbol);
	node = createTreeNode(ps, MINUS, createLeafNode(NULL), y);

	// TODO emit
	return node;
}


struct treeNode *plusOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL; 
	ps = exprsOp(x->symbol, PLUS ,y->symbol);			  
	//if (ps) emitAddition( (Symbol *) x, (Symbol *) y);

	struct treeNode *node = createTreeNode(ps, PLUS, x, y);	

	return node;
}


struct treeNode *minusOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL; 
	ps = exprsOp(x->symbol, MINUS ,y->symbol);
	//if (ps) emitSubtraction(x, y);

	struct treeNode *node = createTreeNode(ps, MINUS, x, y);	
	
	return node;
}


struct treeNode *orOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL; 
	ps = exprsOp(x->symbol, OR, y->symbol);
	//if (ps) emitOr(x, y);

	struct treeNode *node = createTreeNode(ps, OR, x, y);	

	return node;
}


struct treeNode *multOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL; 
	struct treeNode *node = NULL;
	
	ps = exprsOp(x->symbol, MULTIPLY ,y->symbol);
	node = createTreeNode(ps, MULTIPLY, x, y);

	//if (ps) emitMultiplication(x, y);
	return node;
}


struct treeNode *divideOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL; 
	struct treeNode *node = NULL;
	
	ps = exprsOp(x->symbol, DIVIDE ,y->symbol);
	node = createTreeNode(ps, DIVIDE, x, y);
	
	//if (ps) emitDivision(x, y);
	return node; 
}


struct treeNode *divOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL; 
	struct treeNode *node = NULL;

	ps = exprsOp(x->symbol, DIV ,y->symbol);
	node = createTreeNode(ps, DIV, x, y);

	//if (ps) emitDivision(x, y);
	return node;
}


struct treeNode *modOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL;
	struct treeNode *node = NULL;
	
	ps = exprsOp(x->symbol, MOD ,y->symbol);
	node = createTreeNode(ps, MOD, x, y);
	
	//if (ps) emitMod(x, y);
	return node;
}


struct treeNode *andOp(struct treeNode *x, struct treeNode *y) {
	ProxySymbol *ps = NULL;
	struct treeNode *node = NULL;
	
	ps = exprsOp(x->symbol, AND ,y->symbol);
	node = createTreeNode(ps, AND, x, y);
	
	//if (ps) emitAnd(x, y);
	return node;
}


struct treeNode *unaryNotOp(struct treeNode *y) {
	ProxySymbol *ps = NULL;
	struct treeNode *node = NULL;
	
	ps = exprsOp(NULL, NOT ,y->symbol);
	node = createTreeNode(ps, NOT, createLeafNode(NULL), y);

	// TODO emit
	return node;
}


/*
 * Perform the operator action on the two operands.
 * 
 * Parameters:
 * 	x: left-hand operand. Can be NULl for unary operators
 * 	y: right-hand operand.
 * 	opToken: operator token
 * Retuns:
 * 	A ProxySymbol of kind CONST_KIND if x and y are both constants,
 * 	a ProxySymbol of kind TYPE_KIND if one of x or y is not constant,
 * 	or NULL if the operands are not operator combatible.
 */
ProxySymbol *exprsOp(ProxySymbol *x, int opToken, ProxySymbol *y){
	ProxySymbol *ps = NULL;
	Symbol *typeSym = NULL;

	/* 
	 * Get the type resulting from performing opToken on the two
	 * operands
	 */
	typeSym = assertOpCompat(
	    getTypeSym((Symbol *) x),
	    opToken,
	    getTypeSym((Symbol *)y));
	
	/* if assertOpCompat returned NULL, operands are not compatible. */
	if (typeSym == NULL) return NULL;
	
	/* 
	 * If x and y are both constants (or y is constant and operator is
	 * unary) return a new ProxySymbol of kind CONST_KIND
	 */
	/* need this nest of IFs to check avoid dereferencing x->kind if
	 * x is null ....
	 */
	if ( (x == NULL) && (y->kind == CONST_KIND )){
		ps = (ProxySymbol *)createConstSymbol(NULL);
		setInnerTypeSymbol(ps, typeSym);
		constCalc(ps, x, opToken, y);
		setConstResultFlag(ps);
		return ps;
	} else if( (x == NULL) && (y->kind != CONST_KIND ) ){
		return typeSym;
	} else if ( (x->kind == CONST_KIND) && (y->kind == CONST_KIND) ){
		ps = (ProxySymbol *)createConstSymbol(NULL);
		setInnerTypeSymbol(ps, typeSym);
		constCalc(ps, x, opToken, y);
		setConstResultFlag(ps);
		return ps;
	} else {
		return typeSym;
	}
}

void
constCalc(ProxySymbol *ps, ProxySymbol *x, int opToken, ProxySymbol *y) {

	int intVal;
	
	switch (opToken) {
	case EQUAL:
		if ((getType(x) == STRING_T) && (getType(y) == STRING_T)) {
			setSimpleConstVal(ps,(double)doStrEqCmp(x,y));
		} else if ((getType(x) == SCALAR_T) && (getType(y) == SCALAR_T)) {
			// SCALAR EQUAL
			setSimpleConstVal(ps,(double)doScalarEqCmp(x,y));
		} else {
			setSimpleConstVal(ps, (double)doEqCmp(x,y));
		}
		break;
		
	case NOT_EQUAL:	
		if ((getType(x) == STRING_T) && (getType(y) == STRING_T)) {
			setSimpleConstVal(ps,(double)!doStrEqCmp(x,y));
		} else if ((getType(x) == SCALAR_T) && (getType(y) == SCALAR_T)) {
			// SCALAR NOT EQUAL
			setSimpleConstVal(ps,(double)doScalarNotEqCmp(x,y));
		} else {
			setSimpleConstVal(ps, (double)doNotEqCmp(x,y));
		}
		break;
		
	case LESS_OR_EQUAL:
		if ((getType(x) == STRING_T) && (getType(y) == STRING_T)) {
			/* x str is less or equal y str */
			intVal = (doStrLessCmp(x,y) || doStrEqCmp(x,y));
			setSimpleConstVal(ps,(double)intVal);
		} else if ((getType(x) == SCALAR_T) && (getType(y) == SCALAR_T)) {
			// lESSS OR EQUAL
			setSimpleConstVal(ps,(double)doScalarLessOrEqCmp(x,y));
		} else {
			setSimpleConstVal(ps, (double)doLessOrEqCmp(x,y));
		}
		break;
		
	case LESS:
		if ((getType(x) == STRING_T) && (getType(y) == STRING_T)) {
			setSimpleConstVal(ps,(double)doStrLessCmp(x,y));
		} else if ((getType(x) == SCALAR_T) && (getType(y) == SCALAR_T)) {
			// lESS
			setSimpleConstVal(ps,(double)doScalarLessCmp(x,y));
		} else {
			setSimpleConstVal(ps,(double)doLessCmp(x,y));
		}
		break;
		
	case GREATER_OR_EQUAL:
		if ((getType(x) == STRING_T) && (getType(y) == STRING_T)) {
			/* x str is greater or equal y str */
			intVal = (doStrGtCmp(x,y) || doStrEqCmp(x,y));
			setSimpleConstVal(ps,(double)intVal);
		} else if ((getType(x) == SCALAR_T) && (getType(y) == SCALAR_T)) {
			// GREATER_OR_EQUAL
			setSimpleConstVal(ps,(double)doScalarGtOrEqCmp(x,y));	
		} else {
			setSimpleConstVal(ps,(double)doGtOrEqCmp(x,y));
		}
		break;
		
	case GREATER:
		if ((getType(x) == STRING_T) && (getType(y) == STRING_T)) {
			setSimpleConstVal(ps,(double)doStrGtCmp(x,y));
		} else if ((getType(x) == SCALAR_T) && (getType(y) == SCALAR_T)) {
			// GREATER
			setSimpleConstVal(ps,(double)doScalarGtCmp(x,y));	
		} else {
			setSimpleConstVal(ps,(double)doGtCmp(x,y));
		}
		break;
		
	case MULTIPLY:
		setSimpleConstVal(ps, (double)calcMult(x,y));
		break;
		
	case DIVIDE:
		setSimpleConstVal(ps, (double)calcDivide(x,y));
		break;
		
	case DIV:
		setSimpleConstVal(ps, (double)calcDiv(x,y));
		break;
		
	case MOD:
		if((getType(x) == INTEGER_T) && (getType(y) == INTEGER_T)){
			setSimpleConstVal(ps, (double)calcMod(x,y));
		}
		break;
		
	case AND:
		setSimpleConstVal(ps, (double)doAndOp(x,y));
		break;
		
	case OR:
		setSimpleConstVal(ps, (double)doOrOp(x,y));
		break;
		
	case PLUS:
		if ( x == NULL ){
			/* x is supposed to be NULL if this is a unary op */
			setSimpleConstVal(ps, (double)doUnaryPlusOp(y));
		}else{
			/* addition */
			setSimpleConstVal(ps, (double)calcSum(x,y));
		}
		break;
		
	case MINUS:
		if ( x == NULL ){
			/* x is supposed to be NULL if this is a unary op */
			setSimpleConstVal(ps, (double)doUnaryMinusOp(y));
		}else{
			/* subtraction */
			setSimpleConstVal(ps, (double)calcSub(x,y));
		}
		break;
		
	case NOT:
		if ( x == NULL) {
			/* x is supposed to be NULL if this is a unary op */
			setSimpleConstVal(ps, (double)doUnaryNotOp(y));
		}
		break;
		
	default:
		err(EXIT_FAILURE,"Unknown operator passed to constCalc");
	}
}


double
calcSum(ProxySymbol *x, ProxySymbol *y){
	return (double)(getSimpleConstVal(x)) + 
	(double)(getSimpleConstVal(y));
}


double
calcSub(ProxySymbol *x, ProxySymbol *y){
	return (double)(getSimpleConstVal(x)) - 
	(double)(getSimpleConstVal(y));
}


double
calcDivide(ProxySymbol *x, ProxySymbol *y){
	
	if((double)getSimpleConstVal(y) == 0.0){
		errMsg = customErrorString("DIVIDE operation error:"
		" Denominator has the value of zero."
		"\n");
		recordError(errMsg, yylineno, colno, SEMANTIC);
		err(EXIT_FAILURE,"Arithmetic Overflow Exception");
	}
	
	return (double)(getSimpleConstVal(x)) / 
	(double)(getSimpleConstVal(y));
}


int
calcDiv(ProxySymbol *x, ProxySymbol *y){
	
	if((double)getSimpleConstVal(y) == 0.0){
		errMsg = customErrorString("DIV operation error:"
		" Denominator has the value of zero."
		"\n");
		recordError(errMsg, yylineno, colno, SEMANTIC);
		err(EXIT_FAILURE,"Arithmetic Overflow Exception");
	}

	int val = (double)(getSimpleConstVal(x)) / 
	(double)(getSimpleConstVal(y));
	
	return val;	
}


double
calcMult(ProxySymbol *x, ProxySymbol *y){
	return (double)(getSimpleConstVal(x)) * 
	(double)(getSimpleConstVal(y));
}


double
calcMod(ProxySymbol *x, ProxySymbol *y){

	if((int)getSimpleConstVal(y) == 0){
		errMsg = customErrorString("MOD operation error:"
		" Cannot MOD the value of zero."
		"\n");
		recordError(errMsg, yylineno, colno, SEMANTIC);
		err(EXIT_FAILURE,"Arithmetic Overflow Exception");
	}

	return (int)(getSimpleConstVal(x)) % 
	(int)(getSimpleConstVal(y));
}


int
doAndOp(ProxySymbol *x, ProxySymbol *y){
	return (int)(getSimpleConstVal(x)) && 
	(int)(getSimpleConstVal(y));
}


int
doOrOp(ProxySymbol *x, ProxySymbol *y){
	return (int)(getSimpleConstVal(x)) || 
	(int)(getSimpleConstVal(y));
}


int
doGtCmp(ProxySymbol *x, ProxySymbol *y){
	return (double)(getSimpleConstVal(x)) > 
	    (double)(getSimpleConstVal(y));
}

int
doGtOrEqCmp(ProxySymbol *x, ProxySymbol *y){
	return (double)(getSimpleConstVal(x)) >= 
	     (double)(getSimpleConstVal(y));

}


int
doLessCmp(ProxySymbol *x, ProxySymbol *y){
	return (double)(getSimpleConstVal(x)) < 
	    (double)(getSimpleConstVal(y));
}


int
doLessOrEqCmp(ProxySymbol *x, ProxySymbol *y){
	return (double)(getSimpleConstVal(x)) <= 
	    (double)(getSimpleConstVal(y));

}

int
doNotEqCmp(ProxySymbol *x, ProxySymbol *y){
	return (double)(getSimpleConstVal(x)) != 
	    (double)(getSimpleConstVal(y));
}

int
doEqCmp(ProxySymbol *x, ProxySymbol *y){
	return (double)(getSimpleConstVal(x)) == 
	    (double)(getSimpleConstVal(y));

}

int
doUnaryNotOp(ProxySymbol *y) {
	return !(int)(getSimpleConstVal(y));
}


double
doUnaryPlusOp(ProxySymbol *y) {
	/* unary plus is identity operation, so sign shouldn't change */
	return (double)(getSimpleConstVal(y));
}


double
doUnaryMinusOp(ProxySymbol *y) {
	return 0.0-(double)(getSimpleConstVal(y));
}

/*
 *	Check whether the two given strings are equal char by char
 * 
 *	Parameters: ProxySymbol: x a symbol of string constant 
 *				ProxySymbol; y a symbol of string constant 
 *
 *	Returns:	1:	two given strings are equal 
 *				0:  two given strings are not equal
 *
 */
int
doStrEqCmp(ProxySymbol *x, ProxySymbol *y){
	int i;
	int len = (int)getConstVal(x)->String.strlen;
	char *str1 = (char *)getConstVal(x)->String.str;
	char *str2 = (char *)getConstVal(y)->String.str;
	
	
	for (i = 0; i < len ; i++ ) {
		if(str1[i] != str2[i]){
			return 0;
		}	
	}
	
	return 1;
}

/*
 *	Compare whether one string is less than another char by char
 * 
 *	Parameters: ProxySymbol: x a symbol of string constant 
 *				ProxySymbol; y a symbol of string constant 
 *
 *	Returns:	1:	x < y
 *				0:	x !< y
 *
 */

int
doStrLessCmp(ProxySymbol *x, ProxySymbol *y){
	int i;
	int len = (int)getConstVal(x)->String.strlen;
	char *str1 = (char *)getConstVal(x)->String.str;
	char *str2 = (char *)getConstVal(y)->String.str;
	
	for (i = 0; i < len-1 ; i++ ) {
		if( ( str1[i] == str2[i] ) && ( str1[i+1] < str2[i+1] ) ){
				return 1;
		}
	}
	return 0;
}

/*
 *	Compare whether one string is greater than another char by char
 * 
 *	Parameters: ProxySymbol: x a symbol of string constant 
 *				ProxySymbol; y a symbol of string constant 
 *
 *	Returns:	1:	x > y
 *				0:	x !> y
 *
 */
int
doStrGtCmp(ProxySymbol *x, ProxySymbol *y){
	int i;
	int len = (int)getConstVal(x)->String.strlen;
	char *str1 = (char *)getConstVal(x)->String.str;
	char *str2 = (char *)getConstVal(y)->String.str;
	
	for (i = 0; i < len-1 ; i++ ) {
		if( ( str1[i] == str2[i] ) && ( str1[i+1] > str2[i+1] ) ){
				return 1;
		}
	}
	return 0;
}


int 
doScalarEqCmp(ProxySymbol *x, ProxySymbol *y) {
	int low = x->kindPtr.ConstKind->value.Integer.value;
	int high = y->kindPtr.ConstKind->value.Integer.value;
	
	if(low == high ){
		return 1;
	} else {
		return 0;
	}
}


int doScalarNotEqCmp(ProxySymbol *x, ProxySymbol *y) {
	int low = x->kindPtr.ConstKind->value.Integer.value;
	int high = y->kindPtr.ConstKind->value.Integer.value;

	if ( low != high ) {
		return 1;
	} else {
		return 0;
	}
}


int doScalarGtCmp(ProxySymbol *x, ProxySymbol *y) {
	int low = x->kindPtr.ConstKind->value.Integer.value;
	int high = y->kindPtr.ConstKind->value.Integer.value;

	if ( low > high ) {
		return 1;
	} else {
		return 0;
	}
}


int doScalarGtOrEqCmp(ProxySymbol *x, ProxySymbol *y) {
	int low = x->kindPtr.ConstKind->value.Integer.value;
	int high = y->kindPtr.ConstKind->value.Integer.value;

	if ( low >= high ) {
		return 1;
	} else {
		return 0;
	}
}


int doScalarLessCmp(ProxySymbol *x, ProxySymbol *y) {
	int low = x->kindPtr.ConstKind->value.Integer.value;
	int high = y->kindPtr.ConstKind->value.Integer.value;
	
	if ( low < high ) {
		return 1;
	} else {
		return 0;
	}
}


int doScalarLessOrEqCmp(ProxySymbol *x, ProxySymbol *y) {
	int low = x->kindPtr.ConstKind->value.Integer.value;
	int high = y->kindPtr.ConstKind->value.Integer.value;

	if ( low <= high ) {
		return 1;
	} else {
		return 0;
	}
}

/*
 * Compare that the given types are compatible when using the given
 * operator.
 *
 * Parameters:  typeSymbol1: a symobol of type TYPE
 *		opToken: operation to check against	
 *		typeSymbol1: a symobol of type TYPE
 *
 * Returns: Return a pointer to a Symbol struct that is populated 
 * 		with the resulting type when performing the operation. 
 *		Otherwise, returns and if possible, sets an opError message.
 */
Symbol *assertOpCompat(
    Symbol *typeSymbol1, int opToken, Symbol *typeSymbol2) {
	
	type_t type1 = getType(typeSymbol1);
	type_t type2 = getType(typeSymbol2);

	if ( (!typeSymbol1) && (!typeSymbol2)) return NULL;

#if DEBUG
	printf("\n%s %s %s\n", typeToString(getType(typeSymbol1)), 
	opToString(opToken), typeToString(getType(typeSymbol2)));
#endif

	/* if typeSymbol1 pointer is null but the operator is PLUS or MINUS (i.e.,
	 * it is a unary operator) then we assume the best */ 
	if ( typeSymbol1 == NULL
		&& isUnaryOperator(opToken)
		&& isRealIntBool(type2)
	) {
		return typeSymbol2;
	}

	/* At this point, we do not accept nulls */
	if ( typeSymbol1 == NULL || typeSymbol2 == NULL ) {
		opError(typeToString(type2), opToken, typeToString(type2));
		return NULL; /* else it was an error */		
	}

	/* If the operator is relational, we just need op compatible types */
	if ( isRelationalOperator(opToken) 
		&& areOpCompatible(typeSymbol1, typeSymbol2) ) {
		return getPreDefBool(preDefTypeSymbols);
	}	

	/* If the operator is relational and both types are scalar */
	if ( isRelationalOperator(opToken) 
		&& areSameType(typeSymbol1, typeSymbol2) 
		&& type1 == SCALAR_T 
	) 
	{
		return getPreDefBool(preDefTypeSymbols); 
	}

	/* If operator is a logical operator, we only accept boolean types */
	if (isLogicalOperator(opToken)) {
		/*check to see if both types are boolean*/
		if ((type1 != BOOLEAN_T) || (type2 != BOOLEAN_T)) {
			opError(typeToString(type1), opToken,
			    typeToString(type2));
			return NULL;
		}

		/* Else return pointer to pre-defined boolean type */
		return getPreDefBool(preDefTypeSymbols); 
	}


	if (areArithmeticCompatible(typeSymbol1, typeSymbol2)) {
		switch (opToken) {
			case PLUS: //drop through
			case MINUS: //drop through
			case MULTIPLY:
				if (areBothInts(typeSymbol1, typeSymbol2)) {
					return getPreDefInt(preDefTypeSymbols);
				}
				else {
				       return getPreDefReal(preDefTypeSymbols);
				} 
				break;
			case DIVIDE:
				return getPreDefReal(preDefTypeSymbols);
			case DIV: //drop throught
			case MOD:
				if (areBothInts(typeSymbol1, typeSymbol2)) {
					return getPreDefInt(preDefTypeSymbols);
				}
				break;
			default:
				/* NOT REACHED */
				break;	
		}
	}
	
	opError(typeToString(type1), opToken, typeToString(type2));
	return NULL;
}


/*
 * Boolean function to determine if two symbols are assignment
 * compatible
 *
 * Parameters:  x: symbol to compare
 *		y: symbol to compare
 *
 * Returns: 1 if both symbols are compatible. 0 otherwise
 */
int isAssignmentCompat(Symbol *x, Symbol *y) {
	Symbol *type1, *type2;

	type1 = getTypeSym(x);
	type2 = getTypeSym(y);
	/*checking the kind first*/
	if (areKindAssignCompat(x,y)) {
		if (areSameType(type1, type2)) {
			return 1;
		} else if (areCompatibleStrings(type1, type2)) {
			return 1;
		} else if ((getType(type1) == REAL_T) && 
	    	(getType(type2) == INTEGER_T)) {
			return 1;
		} else if (isConstInScalar(y,type1)) {
			return 1;
		}
	} 
	errMsg = customErrorString("The type %s cannot be assigned a value"
	    " of type %s", typeToString(getType(type1)), 
	    typeToString(getType(type2)));
	recordError(errMsg, yylineno, colno, SEMANTIC);
	
	return 0;
}
