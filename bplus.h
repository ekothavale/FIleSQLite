#ifndef BPLUS_H
#define BPLUS_H

#include "common.h"
#include "page.h"

#define M 4		// order (number of children a node can have) of the tree
#define HALF_M (M / 2)
#define PAGE_SIZE 4096 		// size in bytes of each page
#define NUM_SLOTS 64 		// Size of slot array within each page (each page can hold 72 tuples)
#define NUM_VALS 700 		// in reality this will be the size of the page minus the slot array and the header

// UNTESTED
#define MAX_KEY(n) \
	((n)->keys[n->childCount-1])

// UNTESTED
#define MIN_KEY(n) \
	((n)->keys[0])

typedef struct node {
	void* children[M];
	// keys = [3, 5] means 1, 2, 3 - left child, 4, 5 - middle child, 6+ - right child
	// nodes have room for M keys but only leaf nodes will use all M slots
	int keys[M];
	struct node* parent;
	struct node* next;
	struct node* prev; // remove if two way scanning not necessary

	int childCount;
	uint32_t maxPageNumber;

	bool isLeaf; // if the node is a leaf node
}node;

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


slotted_page* findPage(uint32_t pageNum, tree* tree);
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