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
node* newTree(uint32_t pageNum);


page* findPage(uint32_t pageNum, node* tree);
page* findAndInsert(uint32_t pageNum, node* tree);
page* newPage(uint32_t pageNum, node* parent);
node* newNode(bool isLeaf, node* parent);
uint32_t findNextPageNum(page* p);
void freePage(page* p);

// Insertion functions
void insertPageIntoChildren(node* n, page* p);
node* splitNode(node* n);
void addPage(node* n, page* newPage);
node* balanceTreeAdd(node* n);
bool insertTuple(int tuple, u_int32_t pageNum, node* tree);
bool writeVal(page* p, int tuple);

#endif // BPLUS_H