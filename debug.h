#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include "common.h"
#include "page.h"
#include "chunk.h"
#include "bplus.h"

void printTokenizedQuery(TokenizedQuery* tquery);
void printChunk(Chunk* chunk);

node* generateTestBPlusTree();
void printIntArray(int* arr, int length);
void printNode(node* n);
void printTree(tree* t);
bool checkTreePointers(tree* t);

/* Slotted-page pretty-printers (types defined in page.h) */
void printEntry(entry* e);
void printSPSlot(sp_slot* s);
void printSlottedPage(slotted_page* p);

#endif // DEBUG_H