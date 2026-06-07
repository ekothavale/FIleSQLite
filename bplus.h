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
	btree_node* root;
	uint32_t pageMaxSlots;
	uint32_t pageMaxEntries;
	uint32_t pageCap;
}tree;

void freeTree(tree* t);
tree* newTree(slotted_page* p);


slotted_page* findPage(uint32_t pageNum, tree* tree);
slotted_page* findAndInsert(uint32_t pageNum, tree* tree);
bool findAndDelete(uint32_t pageNum, tree* tree);
btree_node* newNode(bool isLeaf, btree_node* parent);
uint32_t findNextPageNum(slotted_page* p);
void freePage(slotted_page* p);

// Insertion functions
void insertPageIntoChildren(btree_node* n, slotted_page* p);
btree_node* splitNode(btree_node* n, tree* t);
void addPage(btree_node* n, slotted_page* newPage, tree* t);
bool deletePage(btree_node* n, uint32_t pageNum, tree* t);
btree_node* balanceTreeAdd(btree_node* n, tree* t);
btree_node* balanceTreeDelete(btree_node* n, tree* t);
bool insertTuple(int tuple, u_int32_t pageNum, tree* t);
bool writeVal(slotted_page* p, int tuple);

#endif // BPLUS_H