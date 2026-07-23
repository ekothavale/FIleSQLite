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

#ifndef LEXER_H
#define LEXER_H

#include "../common.h"
#include "../memory.h"

#define INITIAL_TOKEN_COUNT 128

typedef enum {
	// Single-character tokens
	TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
	TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
	TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
	// One or two character tokens
	TOKEN_BANG, TOKEN_BANG_EQUAL,
	TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER, TOKEN_GREATER_EQUAL,
	TOKEN_LESS, TOKEN_LESS_EQUAL,
	// Literals
	TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
	// Keywords — single words only; multi-word clauses (GROUP BY, INSERT INTO, etc.) matched at parse level
	TOKEN_ADD, TOKEN_ALL, TOKEN_ALTER, TOKEN_AND, TOKEN_ANY, TOKEN_AS, TOKEN_ASC,
	TOKEN_BACKUP, TOKEN_BETWEEN, TOKEN_BY,
	TOKEN_CASE, TOKEN_CHECK, TOKEN_COLUMN, TOKEN_CONSTRAINT, TOKEN_CREATE,
	TOKEN_DATABASE, TOKEN_DEFAULT, TOKEN_DELETE, TOKEN_DESC, TOKEN_DISTINCT, TOKEN_DROP,
	TOKEN_EXEC, TOKEN_EXISTS,
	TOKEN_FOREIGN, TOKEN_FROM, TOKEN_FULL,
	TOKEN_GROUP,
	TOKEN_HAVING,
	TOKEN_IN, TOKEN_INDEX, TOKEN_INNER, TOKEN_INSERT, TOKEN_INTO, TOKEN_IS,
	TOKEN_JOIN,
	TOKEN_KEY,
	TOKEN_LEFT, TOKEN_LIKE, TOKEN_LIMIT,
	TOKEN_NOT, TOKEN_NULL,
	TOKEN_ON, TOKEN_OR, TOKEN_ORDER, TOKEN_OUTER,
	TOKEN_PRIMARY, TOKEN_PROCEDURE,
	TOKEN_REPLACE, TOKEN_RIGHT, TOKEN_ROWNUM,
	TOKEN_SELECT, TOKEN_SET,
	TOKEN_TABLE, TOKEN_TOP, TOKEN_TRUNCATE,
	TOKEN_UNION, TOKEN_UNIQUE, TOKEN_UPDATE,
	TOKEN_VALUES, TOKEN_VIEW,
	TOKEN_WHERE,
	// Signals
	TOKEN_ERROR, TOKEN_EOF, TOKEN_EMPTY
} token_type;

typedef struct token {
	token_type type;
	const char* start;
	int length;
	int line;
} token;

// dynamic array to hold tokens
typedef struct tokenized {
    token* tokens; // pointer to the start of token dynamic array
    int capacity; // token dynamic array size
    int count; // token dynamic array count (grow when count >= capacity)
} tokenized;

// Public API — callable from outside this translation unit
void initLexer(const char* source);
token scanToken();
void initTokenized(tokenized* t);
void addToken(tokenized* t, token tok);
tokenized lexQuery(const char* source);

#endif // SCANNER_H