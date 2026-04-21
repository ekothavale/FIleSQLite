#ifndef BPLUS_H
#define BPLUS_H

#include "common.h"
#include "types.h"

// UNTESTED
#define MAX_KEY(n) \
	((n)->keys[n->childCount-1])

// UNTESTED
#define MIN_KEY(n) \
	((n)->keys[0])

// struct to control root pointer for a tree
// this is the RAM equivalent of a table's root pointer when disk storage is implemented
typedef struct tree {
	node* root;
}tree;

void freeTree(tree* t);
tree* newTree(uint32_t pageNum);


page* findPage(uint32_t pageNum, tree* tree);
page* findAndInsert(uint32_t pageNum, tree* tree);
bool findAndDelete(uint32_t pageNum, tree* tree);
page* newPage(uint32_t pageNum, node* parent);
node* newNode(bool isLeaf, node* parent);
uint32_t findNextPageNum(page* p);
void freePage(page* p);

// Insertion functions
void insertPageIntoChildren(node* n, page* p);
node* splitNode(node* n, tree* t);
void addPage(node* n, page* newPage, tree* t);
bool deletePage(node* n, uint32_t pageNum, tree* t);
node* balanceTreeAdd(node* n, tree* t);
node* balanceTreeDelete(node* n, tree* t);
bool insertTuple(int tuple, u_int32_t pageNum, tree* t);
bool writeVal(page* p, int tuple);

#endif // BPLUS_H