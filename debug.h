#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include "common.h"
#include "chunk.h"
#include "bplus.h"

void printTokenizedQuery(TokenizedQuery* tquery);
void printChunk(Chunk* chunk);

node* generateTestBPlusTree();
void printPage(page* p);
void printTree(node* root, int level);

#endif // DEBUG_H