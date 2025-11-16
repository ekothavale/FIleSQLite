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

void printPage(page* p) {
	printf("Page %d\n", p->pageNum);
	printf("Parent node: %a", p->parent);
	printf("Used slots: %d\nUsed memory: %dB\n", p->usedSlots, p->usedMem);
	printf("Slot array:\n");
	for (int i = 0; i < p->usedSlots - 1; p++) {
		printf("%d | ", p->slotarr[i]);
	}
	printf("%d\n", p->slotarr[p->usedSlots-1]);
	printf("Stack Top: %a\n", p->stackTop);
	printf("Memory:\n");
	for (int* l = p->stackTop; l < NUM_VALS - 1; l++) {
		printf("%d | ", *l);
	}
	printf("%d\n", p->vals[NUM_VALS-1]);
	print("\n");

}