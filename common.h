#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef enum TokenType {
    TOKEN_SELECT,
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

// bytecode
typedef enum opcode {
    OP_SELECT
}opcode;

#endif // COMMON_H

