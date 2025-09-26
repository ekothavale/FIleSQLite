#include "debug.h"
#include "chunk.h"

char* stringTokens[] = {"SELECT", "IDENTIFIER", "NUMBER", "STRING", "STAR", "PLUS", "MINUS", "SLASH", "BANG", "EQUALS", "QUESTION", "RIGHT_PAREN", "LEFT_PAREN", "COMMA", "DOT", "AND", "PIPE", "UNKNOWN"};

void printTokenizedQuery(TokenizedQuery* tquery) {
    for (int i = 0; i < tquery->count; i++) {
        Token* token = &tquery->tokens[i];
        printf("Token %d: Type=%s, Payload=%p, Address=%p\n", i, stringTokens[token->type], (void*) token->payload, (void*) token);
    }
	printf("Total tokens: %d\n", tquery->count);
}

void printChunk(Chunk* chunk) {
    for (int i = 0; i < chunk->count; i++) {
        printf("Chunk %d: Value=%d, Address=%p\n", i, chunk->code[i], (void*) &chunk->code[i]);
    }
	printf("Total chunks: %d\n", chunk->count);
}