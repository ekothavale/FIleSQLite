#ifndef TYPES_H
#define TYPES_H

#include "common.h"

#define M 4 				// order (number of children a node can have) of the tree
#define PAGE_SIZE 4096 		// size in bytes of each page
#define NUM_SLOTS 64 		// Size of slot array within each page (each page can hold 72 tuples)
#define NUM_VALS 700 		// in reality this will be the size of the page minus the slot array and the header

typedef struct slot {
	uint16_t slotnum; // identifier of each record
	uint16_t offset; // offset within the cell array of each record
	uint16_t size;
}slot;

/*
each record is a row of data in a table
each element of the record is proceeded by an escape character representing its c type
\s - string
\i - int
\u - u_int32
\l - long
\d - double
\b - bool
\! - NULL
'\ - intentional backslash as a part of a string
*/ 
typedef struct record {
	char* data;
	uint32_t pageNum;
	uint16_t slotNum;
}record;

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

/*
typedef struct page {
	uint32_t pageNum;
	int cleanSlots; // the number of non-deleted slots in use
	int deletedSlots; // the number of deleted slots in use
	int usedMem; // amount of mem taking in bytes by values stored in the page
	uint16_t wastedBytes;
	slot slotarr[NUM_SLOTS];
	record cells[NUM_VALS];
	int valsOffset; // offset from end of page
	struct node* parent;
}page;
*/

typedef struct page {
	node* parent;
	int records[NUM_SLOTS];
	int numRecords;
	uint32_t pageNum;
}page;

#endif