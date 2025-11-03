#ifndef BPLUS_H
#define BPLUS_H

#include "common.h"

// UNTESTED
#define MAX_KEY(n) \
	((n)->keys[n->childCount-1])

#define MIN_KEY(n) \
	((n)->keys[0])

const int M = 3; // order (number of children a node can have) of the tree

int height = 0; // FOR DEBUGGING PURPOSES: height of the tree

const int PAGE_SIZE = 4096; // size in bytes of each page
const int NUM_SLOTS = 64; // Size of slot array within each page (each page can hold 72 tuples)
				   // numSlots should be power of 2 since it's represented by log(numSlots) bits in key
const int NUM_VALS = 700; // in reality this will be the size of the page minus the slot array and the header
const int PAGE_CAPACITY = NUM_VALS * 4; // memory capacity of page storage (right now assuming all values are ints)

typedef struct page{
	int pageNum;
	int usedSlots;
	int usedMem; // amount of mem taking in bytes by values stored in the page
	int* slotarr[NUM_SLOTS];
	int vals[NUM_VALS];
	int* stackTop; // starts at end of page and gets decremented
	node* parent;
}page;

typedef struct node{
	void* children[M];
	// keys = [3, 5] means 1, 2, 3 - left child, 4, 5 - middle child, 6+ - right child
	int keys[M-1];
	node* parent;
	node* next;
	node* prev; // remove if two way scanning not necessary

	int childCount;
	int maxPageNumber;

	bool isLeaf; // if the node is a leaf node
}node;

void freeTree(node* r);
void insertTuple(int tuple, int pageNum, node* tree);
node* newTree(int pageNum);

#endif