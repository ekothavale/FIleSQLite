/*
The backend for this DBMS uses a B+ tree, a cousin of the B tree.
The B+ tree functions the same as the B tree, except data is only stored in leaf nodes.
Internal nodes store keys to guide the logarithmic search for nodes.
Furthermore, all leaf nodes are connected to form a sorted (doubly) linked list.
This speeds up operations such as surveying the entire database.
This file implements the B+ tree. The data in each node will be stored as a slotted page
which is implemented in another file.
*/

// Optimize
//     first for algorithmic time complexity
//     second for smallest possible nodes

/* TODO:
 - Finish implementing b tree algorithms in this file
 - Build file IO API
 - Reimplement the b tree algorithms on disk
*/

/* Keys:
 - Each key is a 32bit unsigned int
 - Last n bits represent the slot number within a page
 - Remaining bits represent the page number
 - n = number of slots per page
 - How many slots should be in a page?
 - How big are pages?
*/

#include "bplus.h"

node* root;

// ##########################################################################################################################################
// ##########################################################################################################################################
// TREE CREATION FUNCTIONS

node* newRoot(node* child, int childCount) {
	node* new = malloc(sizeof(node));
	new->childCount = childCount;
	new->children[0] = child;
	new->parent = NULL;
	new->prev = NULL;
	new->next = NULL;
	new->isLeaf = false;

	new->maxPageNumber = child->maxPageNumber;
	child->parent = new;
	return new;
}

// creates a blank page and fills in the page number and parent
page* newPage(uint32_t pageNum, node* parent) {
	page* p = calloc(1, sizeof(page));
	p->pageNum = pageNum;
	p->parent = parent;
	return p;
}

/*
creates a blank leaf or interior node
*/
node* newNode(bool isLeaf, node* parent) {
	node* n = calloc(1, sizeof(node));
	n->isLeaf = isLeaf;
	n->parent = parent;
	return n;
}

// creates a new b+ tree starting with just one leaf node and one page
node* newTree(uint32_t pageNum) {
	node* new = newNode(true, NULL);
	new->childCount = 1;

	page* p = newPage(pageNum, new);
	new->children[0] = p;

	return new;
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// GENERAL HELPER FUNCTIONS

// this should eventually be moved to a file containing general helper functions for the entire project
int max(int a, int b) {
	return a >= b ? a : b;
}

// shifts the elements of an array right by 1 starting at the given index
// assumes there is a free space in the array
// len is the total length of the array
// start is the free space to be created
int shiftIntArray(int* array, int start, int len) {
	if (start > len-1) {
		printf("Start index %d beyond length %d of array in shiftIntArray\n", start, len);
		return -1;
	}
	for (int i = len-1; i > start; i--) {
		array[i] = array[i-1];
	}
	array[start] = 0;
	return 0;
}

int shiftPageArray(page** array, int start, int len) {
	if (start > len-1) {
		printf("Start index %d beyond length %d of array in shiftPageArray\n", start, len);
		return -1;
	}
	for (int i = len-1; i > start; i--) {
		array[i] = array[i-1];
	}
	array[start] = NULL;
	return 0;
}

int shiftNodeArray(node** array, int start, int len) {
	if (start > len-1) {
		printf("Start index %d beyond length %d of array in shiftPageArray\n", start, len);
		return -1;
	}
	for (int i = len-1; i > start; i--) {
		array[i] = array[i-1];
	}
	array[start] = NULL;
	return 0;
}

// UNTESTED
bool isPageFull(page* p) {
	return p->cleanSlots >= NUM_SLOTS;
}

// UNTESTED
bool isNodeFull(node* n) {
	return n->childCount >= M;
}

// UNTESTED
bool isRoot(node* n) {
	return n->parent == NULL && n->childCount > 0;
}

// finds a page in a tree by page number
// returns null if page is not in tree
page* findPage(uint32_t pageNum, node* tree) {
    if (tree == NULL || tree->childCount == 0) {
        printf("Attempted to find page in invalid tree\n");
        return NULL; // input was an invalid tree
    }
	if (pageNum > tree->maxPageNumber) return NULL;
	if (pageNum <= 0) return NULL;
    // Compare pageNum against keys in node
    while (!tree->isLeaf) {
        int found = 0;
        for (int i = 0; i < tree->childCount - 1; i++) {
            // Searching for the correct key position
            if (pageNum <= tree->keys[i]) {
                tree = tree->children[i];
                found = 1;
                break;
            }
        }
        // If key wasn't less than any indices, the last child is the correct path
        if (!found) {
            tree = tree->children[tree->childCount - 1];
        }
    }
    // We have found the correct leaf
    for (int i = 0; i < tree->childCount; i++) {
        page* p = (page*)tree->children[i];
        if (p->pageNum == pageNum) {
            return p;
        }
    }
    return NULL;
}

/*
CHANGE ALL PAGENUMS TO UNSIGNED INTS IN ALL FUNCTIONS
Given a page, find the next available page number
@return 0 for failure
There is no guarantee this function will return a page number that is legal by the properties of a search tree.
Probably a good target for refactoring after the first draft of the tree.
*/
uint32_t findNextPageNum(page* p) {
	if (!p || !p->parent) return 0;
	uint32_t try = p->pageNum + 1;
	node* parent = p->parent;
	for (int i = 0; i < parent->childCount; i++) {
		if (((page*) parent->children[i])->pageNum > try) return try;
		if (((page*) parent->children[i])->pageNum == try) try++;
	}
	return 0; // if unable to find page return failure
}

uint32_t updateMaxPageNum(node* n) {
	if (n->isLeaf) return ((page*) n->children[n->childCount-1])->pageNum;
	return ((node*) n->children[n->childCount-1])->maxPageNumber;
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// INSERTION FUNCTIONS

/* adds a page to a node
assumes node is not full
@return true, operation was successful
@return false, operation was not performed because conditions were not right
*/
bool addPage(node* n, page* p) {
	if (p == NULL) {
		printf("Error: tried to add page but page was NULL\n");
		return false;
	}
	if (n == NULL) {
		printf("Error: tried to add page to NULL address\n");
		return false;
	}
	if (isNodeFull(n) || !n->isLeaf) {
		printf("Error: tried to add page to incompatible node\n");
		return false; // shouldn't run in these conditions
	}
	p->parent = n;
	for (int i = 0; i < n->childCount; i++) {
		if (p->pageNum < n->keys[i]) {
			// We've found the correct spot for the page
			n->childCount++;
			shiftIntArray(n->keys, i, n->childCount);
			n->keys[i] = p->pageNum;
			shiftPageArray((page**) n->children, i, n->childCount);
			n->children[i] = p;
			n->maxPageNumber = max(n->maxPageNumber, p->pageNum);
			return true;
		}
	}
	// if here, new page should go last
	n->keys[n->childCount] = p->pageNum;
	n->children[n->childCount] = p;
	n->childCount++;
	n->maxPageNumber = max(n->maxPageNumber, p->pageNum);
	return true;
}

/*
adds a child node to another node
assumes node is not full and node is not a leaf
@return true, operation was successful
@return false, operation was not performed since the conditions weren't right
*/
bool addNode(node* parent, node* child) {
	if (child == NULL) {
		printf("Error: tried to add node but node was NULL\n");
		return false;
	}
	if (parent == NULL) {
		printf("Error: tried to add node but parent node was NULL\n");
	}
	if (isNodeFull(parent) || parent->isLeaf) {
		printf("Error: tried to add node to incompatible parent node\n");
		return false; // shouldn't run in these conditions
	}

	child->parent = parent;

	for (int i = 0; i < parent->childCount - 1; i++) {
		if (child->maxPageNumber < parent->keys[i]) {
			// We've found the right spot to insert the child
			printIntArray(parent->keys, M);
			shiftIntArray(parent->keys, i, parent->childCount);
			parent->keys[i] = child->maxPageNumber;
			printIntArray(parent->keys, M);
			shiftNodeArray(((node**)parent->children), i, parent->childCount + 1);
			parent->children[i] = child;
			parent->childCount++;
			return true;
		}
	}
	// if function reaches this point, new node is either last or second last
	if (child->maxPageNumber > parent->maxPageNumber) {
		// new key is the maximum of the previous node
		parent->keys[parent->childCount - 1] = ((node*) parent->children[parent->childCount - 1])->maxPageNumber;
		parent->children[parent->childCount] = child;
		parent->childCount++;
		parent->maxPageNumber = child->maxPageNumber;
	} else {
		parent->keys[parent->childCount - 1] = child->maxPageNumber;
		parent->children[parent->childCount] = parent->children[parent->childCount-1]; // shift last node forwards one space
		parent->children[parent->childCount-1] = child;
		parent->childCount++;
	}
	return true;
}

// UNTESTED
// splits a node
node* splitNode(node* n) {
	node* new = newNode(n->isLeaf, n->parent);

	// copying over children and pointers
	for (int i = 0; i < n->childCount/2; i++) {
		new->children[i] = n->children[i + n->childCount/2];
		n->children[i + n->childCount/2] = NULL;
	}
	for (int i = 0; i < new->childCount - 1; i++) {
		new->keys[new->childCount - 2] = n->keys[n->childCount - 2 - i];
		n->keys[n->childCount - 2 - i] = 0;
	}
	new->childCount = n->childCount/2;
	n->childCount -= n->childCount/2;

	if (n->isLeaf) {
		// Inserting into linked list of leaves
		node* tmp = n->next;
		n->next = new;
		new->prev = n;
		new->next = tmp;
		tmp->prev = new;
	}

	// Updating and setting maxPageNumber
	updateMaxPageNum(n);
	updateMaxPageNum(new);

	return new;
}

// UNTESTED
// adds a child node to a parent node and balances the tree recursively
void addNodeAndBalance(node* n, node* newChild) {
	if (isNodeFull(n)) {
		node* new = splitNode(n);
		// if parent node is root node then create a new root
		if (isRoot(n)) {
			node* r = newRoot(n, 1);
		}
	
		// decide which node to add the new node to and add it
		for (int i = 0; i < n->parent->childCount; i++) {
			if (n->parent->children[i] == n) {
				if (newChild->maxPageNumber > n->parent->keys[i]) {
					addNode(new, newChild);
				} else {
					addNode(n, newChild);
				}
				break;
			}
		}
		// add new node to parent and balance if necessary
		addNodeAndBalance(n->parent, new);
		return;
	}
	// otherwise just add the node
	addNode(n, newChild);
}

// UNTESTED
// adds a page to a node and balances the tree recursively
void addPageAndBalance(node* n, page* newPage) {
	// add new page number and pointer to respective arrays in n
	// if this would exceed n's capacity, split n and call addNodeAndBalance()
	if (isNodeFull(n)) {
		// split the node
		node* new = splitNode(n);
		// if current node is root node then create a new root
		if (isRoot(n)) {
			node* r = newRoot(n, 1);
		}

		// decide which node to add the page to and add it
		for (int i = 0; i < n->parent->childCount; i++) {
			if (n->parent->children[i] == n) {
				if (newPage->pageNum > n->parent->keys[i]) {
					addPage(new, newPage);
				} else {
					addPage(n, newPage);
				}
				break;
			}
		}
		// add the new node to parent and balance if necessary
		addNodeAndBalance(n->parent, new);
		return;
	}
	// else just add the new page to the node
	addPage(n, newPage);
}

// UNTESTED
// adds new tuple to page and recursively balances tree
void addTupleAndBalance(page* p, record r) {
	int num = findNextPageNum(p);
	if (!num) {
		printf("Error: findNextPageNum returned 0 and this error isn't supported yet\n");
		num = p->pageNum - 1;
	}
	page* new = splitPage(p, num);
	writeVal(p, r);
	addPageAndBalance(p->parent, new);
}

// UNTESTED
/*
inserts a new tuple into the b+tree
*/
void insertTuple(record r, u_int32_t pageNum, node* tree) {
	page* p = findPage(pageNum, tree); // find page
	if (isPageFull(p)) {
		addTupleAndBalance(p, r);
	} else {
		if (!writeVal(p, r)) printf("Error: tried to write tuple to incompatible page\n");
	}
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// PAGE FUNCTIONS
// These may at some point be abstracted to a different file

// UNTESTED
// splits a page in two, copies metadata and moves half of the stored data over
page* splitPage(page* p, uint32_t pageNum) {
	// allocate new page
	page* new = newPage(pageNum, p->parent);
	// move over stored data
	if (pageNum > p->pageNum) {
		
	}
	return new;
}

/* writes a value (currently of type int) to a page
@param p: pointer to a page
@param val: value to be written
@return: boolean if write was successful or not
*/
bool writeVal(page* p, record r) {
	if (isPageFull(p)) return false;
	p->cells[NUM_VALS - 1 - p->valsOffset] = r;
	p->usedMem += sizeof(r);
	p->slotarr[p->cleanSlots] = p->valsOffset;
	p->cleanSlots++;
	p->valsOffset++;
	return true;
}

bool addRecord(page* p, record r) {
	// if page is full:
	// 	if the size of the record plus the size of the slot is less than the page's wasted bytes:
	//	  compact the cells and proceed
	//  else:
	// 	  return false
	// find spot in slot array for record
	// shift slots down and put the slot there
	// iterate through deleted slots
	// if there is a deleted slot that is at least twice the size of the record:
	// 	shift all cells down 1
	// 	put record in that cell
	//	decrease size of deleted slot by the size of that record
	//	... | DELETED | ... -> ... | R | DELETED' | ... size(DELETED) = size(R) + size(DELETED)
	//	if the new size of the deleted cell is 4 bytes or less:
	//	  add the new size of the deleted cell to p->wastedBytes
	//	  remove the slot pointing to the cell
	//	  shift any other deleted slots down
	// else:
	// 	add the record to the end of the next open cell
	// return true
	;

}

bool deleteRecord(page* p, record r) {
	;
}

bool updateRecord(page* p, record r) {
	;
}

bool readRecord(page* p, record r) {
	;
}

void sortSlotArray(page* p) {
	;
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// DEBUGGING FUNCTIONS
// These are here not in debug.c because I'm not sure which of these will be kept when this is reimplemented on disk

void freePage(page* p) {
	free(p);
}

void freeTree(node* r) {
	if (r == NULL) return;
	if (r->isLeaf) {
		for (int i = 0; i < r->childCount; i++) {
			freePage(r->children[i]);
		}
		return;
	}
	for (int i = 0; i < r->childCount; i++) {
		freeTree(r->children[i]);
	}
	free(r);
}
