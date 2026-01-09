#ifndef BPLUS_H
#define BPLUS_H

#include "common.h"

#define M 4 				// order (number of children a node can have) of the tree
#define PAGE_SIZE 4096 		// size in bytes of each page
#define NUM_SLOTS 64 		// Size of slot array within each page (each page can hold 72 tuples)
#define NUM_VALS 700 		// in reality this will be the size of the page minus the slot array and the header
#define PAGE_CAPACITY (NUM_VALS * 4) // memory capacity of page storage (right now assuming all values are ints)

// UNTESTED
#define MAX_KEY(n) \
	((n)->keys[n->childCount-1])

// UNTESTED
#define MIN_KEY(n) \
	((n)->keys[0])


typedef struct page {
	int pageNum; // unsigned int greater than 0
	int usedSlots;
	int usedMem; // amount of mem taking in bytes by values stored in the page
	int slotarr[NUM_SLOTS];
	int vals[NUM_VALS];
	int valsOffset; // offset from end of page
	struct node* parent;
}page;

typedef struct node {
	void* children[M];
	// keys = [3, 5] means 1, 2, 3 - left child, 4, 5 - middle child, 6+ - right child
	// nodes have room for M keys but only leaf nodes will use all M slots
	int keys[M];
	struct node* parent;
	struct node* next;
	struct node* prev; // remove if two way scanning not necessary

	int childCount;
	int maxPageNumber;

	bool isLeaf; // if the node is a leaf node
}node;

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