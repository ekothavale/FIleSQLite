#ifndef BPLUS_H
#define BPLUS_H

#include "common.h"
#include "types.h"
#include "debug.h"

// UNTESTED
#define MAX_KEY(n) \
	((n)->keys[n->childCount-1])

// UNTESTED
#define MIN_KEY(n) \
	((n)->keys[0])


void freeTree(node* r);
void insertTuple(int tuple, uint32_t pageNum, node* tree);
node* newTree(uint32_t pageNum);

// Unit Testing
bool writeVal(page* p, int val);
page* findPage(uint32_t pageNum, node* tree);
page* newPage(uint32_t pageNum, node* parent);
node* newNode(bool isLeaf, node* parent);
bool addPage(node* n, page* p);
uint32_t findNextPageNum(page* p);
bool addNode(node* parent, node* child);

page* splitPage(page* p, uint32_t pageNum);
#endif // BPLUS_H