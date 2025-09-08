#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// token representation
// all literal tokens will have payload represented as a string, even numericals and booleans
typedef struct Token {
    TokenType type;
    const char* payload;
}Token;

typedef enum TokenType {
    TOKEN_SELECT,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRLIT,
    TOKEN_WHITESPACE,
    TOKEN_OPERATOR
}TokenType;

// bytecode
typedef enum opcode {
    OP_SELECT
}opcode;

