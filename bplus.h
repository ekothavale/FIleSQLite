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
void insertTuple(int tuple, int pageNum, node* tree);
node* newTree(int pageNum);

// Unit Testing
bool writeVal(page* p, int val);
page* findPage(int pageNum, node* tree);
page* newPage(int pageNum, node* parent);
bool addPage(node* n, page* p);
int findNextPageNum(page* p);


#endif // BPLUS_H