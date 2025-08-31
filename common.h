#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// handleable representation of query input
typedef struct token {
    TokenType type;
}token;

typedef enum TokenType {
    TOKEN_SELECT,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_WHITESPACE,
    TOKEN_OPERATOR
}TokenType;

// bytecode
typedef enum opcode {
    OP_SELECT
}opcode;

