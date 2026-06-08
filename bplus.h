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

#include "common.h"
#include "page.h"
#include "node.h"
#include "tableIO.h"

// UNTESTED
#define MAX_KEY(n) \
	((n)->keys[n->childCount-1])

// UNTESTED
#define MIN_KEY(n) \
	((n)->keys[0])

const int HALF_M = M / 2;

// struct to control root pointer for a tree
// this is the RAM equivalent of a table's root pointer when disk storage is implemented
typedef struct tree {
	node* root;
	uint32_t pageMaxSlots;
	uint32_t pageMaxEntries;
	uint32_t pageCap;
}tree;

void freeTree(tree* t);
tree* newTree(slotted_page* p);


slotted_page* findPage(uint32_t pageNum, table* tree);
slotted_page* findAndInsert(uint32_t pageNum, tree* tree);
bool findAndDelete(uint32_t pageNum, tree* tree);
node* newNode(bool isLeaf, node* parent);
uint32_t findNextPageNum(slotted_page* p);
void freePage(slotted_page* p);

// Insertion functions
void insertPageIntoChildren(node* n, slotted_page* p);
node* splitNode(node* n, tree* t);
void addPage(node* n, slotted_page* newPage, tree* t);
bool deletePage(node* n, uint32_t pageNum, tree* t);
node* balanceTreeAdd(node* n, tree* t);
node* balanceTreeDelete(node* n, tree* t);
bool insertTuple(int tuple, u_int32_t pageNum, tree* t);
bool writeVal(slotted_page* p, int tuple);

#endif // BPLUS_H