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

#ifndef COMMON_H
#define COMMON_H

#define DEBUG_TRACE_EXECUTION

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


typedef enum TokenType {
    TOKEN_SELECT,
    TOKEN_INSERT,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_STAR,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_SLASH,
    TOKEN_BANG,
    TOKEN_EQUALS,
    TOKEN_QUESTION,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_PAREN,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_AND,
    TOKEN_PIPE,
    TOKEN_UNKNOWN
}TokenType;

// token representation
// all literal tokens will have payload represented as a string, even numericals and booleans
typedef struct Token {
    TokenType type;
    char* payload;
}Token;

typedef struct TokenizedQuery {
    Token* tokens;
    int count;
    int capacity;
} TokenizedQuery;

typedef uint64_t address;

#endif // COMMON_H

