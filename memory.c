#include "memory.h"

Token* allocTokens(int numTokens) {
	Token* tokens = (Token*) calloc(numTokens, sizeof(Token));
	if (tokens == NULL) {
		fprintf(stderr, "Could not access memory to tokenize input\n");
		exit(1);
	}
	return tokens;
}

static void freeToken(Token* token) {
    free(token->payload);
}

void freeTokenizedQuery(TokenizedQuery* tquery) {
    for (int i = 0; i < tquery->count; i++) {
        freeToken(&(tquery->tokens[i]));
    }
    free(tquery);
}

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void* result = realloc(pointer, newSize);
  if (result == NULL) {
	fprintf(stderr, "Memory allocation failed\n");
	exit(1);
  }
  return result;
}