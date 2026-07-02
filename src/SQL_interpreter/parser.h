/*
Copyright (c) 2026 Ethan Kothavale

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef PARSER_H
#define PARSER_H

#include "vm.h"
#include "lexer.h"

typedef enum ast_type {
	TYPE_QUERY,
	TYPE_SELECT_STMT,
	TYPE_INSERT_STMT,
	TYPE_UPDATE_STMT,
	TYPE_DELETE_STMT,
	TYPE_CREATE_STMT,
	TYPE_DROP_STMT,
	TYPE_ALTER_STMT,
	TYPE_WHERE_CLAUSE,
	TYPE_GROUP_CLAUSE,
	TYPE_HAVING_CLAUSE,
	TYPE_ORDER_CLAUSE,
	TYPE_LIMIT_CLAUSE,
	TYPE_EXPR,
	TYPE_OR_EXPR,
	TYPE_AND_EXPR,
	TYPE_NOT_EXPR,
	TYPE_COMPARISON,
	TYPE_ADDITIVE,
	TYPE_MULTIPLICATIVE,
	TYPE_UNARY,
	TYPE_PRIMARY,
	TYPE_IDENTIFIER,
	TYPE_LIST_NODE,
	TYPE_SELECT_ITEM,
	TYPE_COL_DEF,
	TYPE_ASSIGNMENT
} ast_type;

typedef struct ast_node {
	ast_type type;
	token tok;   // populated for leaf nodes and nodes whose subtype is encoded by a keyword token
	bool flag;
	ast_node* children[7];
} ast_node;

bool compile(const char* source, Chunk* chunk);

#endif

