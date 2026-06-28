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

#ifndef BPLUS_H
#define BPLUS_H

#include "../common.h"
#include "page.h"
#include "node.h"
#include "tableIO.h"

// UNTESTED
#define MAX_KEY(n) \
	((n)->keys[n->childCount-1])

// UNTESTED
#define MIN_KEY(n) \
	((n)->keys[0])

#define HALF_M (M_GLOBAL / 2)

// struct to control root pointer for a tree
// this is the RAM equivalent of a table's root pointer when disk storage is implemented
typedef struct tree {
	node* root;
	uint32_t pageMaxSlots;
	uint32_t pageMaxEntries;
	uint32_t pageCap;
}tree;

table* createTree(char* tablename, uint32_t pageNum);
void deleteTree(table* t);


address findPage(uint32_t pageNum, table* t);
address findAndInsert(uint32_t pageNum, table* t);
bool findAndDelete(uint32_t pageNum, table* tree);
node* newNode(bool isLeaf, address parent);
uint32_t findNextPageNum(slotted_page* p);
void freePage(slotted_page* p);

// Insertion functions
void insertPageIntoChildren(node* n, address nodeAddr, slotted_page* p, address pageAddr, table* t);
node* splitNode(node* n, address addr, address* newAddrOut, table* t);
void addPage(node* n, address nodeAddr, slotted_page* p, address pageAddr, table* t);
bool deletePage(node* n, address addr, uint32_t pageNum, table* t);
node* balanceTreeAdd(node* n, address addr, address* newAddrOut, table* t);
address balanceTreeDelete(node* n, address addr, table* t);
bool insertTuple(int tuple, u_int32_t pageNum, tree* t);
bool writeVal(slotted_page* p, int tuple);

#endif // BPLUS_H