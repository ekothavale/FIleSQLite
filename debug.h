#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include "common.h"
#include "chunk.h"
#include "types.h"

void printTokenizedQuery(TokenizedQuery* tquery);
void printChunk(Chunk* chunk);

node* generateTestBPlusTree();
void printIntArray(int* arr, int length);
void printPage(page* p);
void printNode(node* n);
void printTree(node* root, int level);

#endif // DEBUG_H