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

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include "common.h"
#include "page.h"
#include "chunk.h"
#include "bplus.h"

void printTokenizedQuery(TokenizedQuery* tquery);
void printChunk(Chunk* chunk);

void generateTestBPlusTree(table* t);
void printIntArray(int* arr, int length);
void printNode(node* n);
void printTree(table* t);
bool checkTreePointers(table* t);

/* Slotted-page pretty-printers (types defined in page.h) */
void printEntry(entry* e);
void printSPSlot(sp_slot* s);
void printSlottedPage(slotted_page* p);

#endif // DEBUG_H