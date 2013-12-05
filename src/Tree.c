#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "Definitions.h"
#include "Tree.h"
#include "Emit.h"

#ifndef TESTBUILD
#include "parser.tab.h"	/* token definitions used in operator compat checks */
#endif
#ifdef TESTBUILD
#include "tokens.h"
#endif




/*
 * Emits code to result the expression contained in the baby tree
 * i.e. tree with root and one left and one right node
 *
 * Parameters:
 *		x: RHS operand
 *		opToken: operator
 *		y: LHS operand
 *
 * Return: void
 */
void emitBabyTree(Symbol *x, int opToken, Symbol *y) {

	switch (opToken) {
		case PLUS:
			emitAddition( (Symbol *) x, (Symbol *) y);
			break;
		default:
			printf("FUCK MY LIFE\n");
	}
}


/*
 * Walks thourge the parse tree. Emits code to resolve each expression
 * at an operator node.
 *
 * Parameters:
 *		node: starting node
 *
 * Return: symbol of resulting expression type
 */
Symbol *postOrderWalk(struct treeNode *node) {
	Symbol *x = NULL;
	Symbol *y = NULL;

	if (node->left == NULL) {
		return node->symbol;
	}
	else {
		x = postOrderWalk(node->left);
		y = postOrderWalk(node->right);
		emitBabyTree(x, node->opToken, y);
		return node->symbol;
	}
}		


/*
 * Allocates memory for a struct treeNode and returns pointer
 * to it.
 *
 * Parameters: void
 *
 * Return: struct treeNode pointer with allocated memory
 */
struct treeNode *allocateTreeNode() {
	struct treeNode *node = calloc(1, sizeof(struct treeNode));

	if ( node == NULL ) {
		err(1, "Could not alloc memory for tree node.");
		exit(EXIT_FAILURE);		
	}

	return node;
}


/*
 * Create and populates an treeNode struct in the binary tree
 *
 * Parameters:
 *		symbol: symbol of the node
 *		opToken: operator of treeNode
 *		left: left child node
 *		right: right child node
 *
 * Return: newly created node in tree
 */
struct treeNode *createNodeCommon(Symbol *symbol, int opToken, 
	struct treeNode *left, struct treeNode *right ) 
{
	struct treeNode *node = allocateTreeNode();
	
	node->symbol = symbol;
	node->opToken = opToken;
	node->parent = NULL;
	node->left = left;
	node->right = right;

	return node;
}


/*
 * Creates regular tree node in the binary parsing tree. This 
 * type of node will have passed values for left and right 
 * children as well as an real opToken value. Also, sets the
 * parent for the left/right children
 *
 * Parameters:
 *		symbol: symbol of the node
 *		opToken: operator of treeNode
 *		left: left child node
 *		right: right child node
 *
 * Return: newly created node in tree
 */
struct treeNode *createTreeNode(Symbol *symbol, int opToken,
	struct treeNode *left, struct treeNode *right) 
{
	struct treeNode *node = NULL;

	node = createNodeCommon(symbol, opToken, left, right);	
	node->left->parent = node;
	node->right->parent = node;

	return node;
}


/*
 * Creates a leaf node in the binary parsing tree. This type of 
 * node will only have a symbol set. All other values in the struct
 * are set to NULL or NO_OP
 *
 * Parameters:
 *		symbol: symbol of the node
 *
 * Return: newly created node in tree
 */
struct treeNode *createLeafNode(Symbol *symbol) {
	struct treeNode *node = NULL;

	node = createNodeCommon(symbol, NO_OPT, NULL, NULL);

	return node;
}
