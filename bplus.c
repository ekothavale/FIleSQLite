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

// ##########################################################################################################################################
// ##########################################################################################################################################
// TREE CREATION FUNCTIONS

/*
creates a new root node
*/
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

/*
creates a blank page and fills in the page number and parent
*/
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
	n->childCount = 0;
	n->maxPageNumber = 0;
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

/* shifts the elements of an array right by 1 starting at the given index
assumes there is a free space in the array
len is the total length of the array
start is the free space to be created
*/
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
	return p->numRecords >= NUM_SLOTS;
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
Given a page, find the next available page number
@return 0 for failure
There is no guarantee this function will return a page number that is legal by the properties of a search tree.
Probably a good target for refactoring after the first draft of the tree.
*/
uint32_t findNextPageNum(page* p) {
	return p->pageNum + 1;
}

uint32_t updateMaxPageNum(node* n) {
	if (n->isLeaf) return ((page*) n->children[n->childCount-1])->pageNum;
	return ((node*) n->children[n->childCount-1])->maxPageNumber;
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// INSERTION FUNCTIONS

// UNTESTED
/*
puts a page into a parent node's children and keys arrays
assumes node is not full
*/
void insertPageIntoChildren(node* n, page* p) {
	// check if page should be inserted into the middle of the children
	for (int i = 0; i < n->childCount; i++) {
		if (n->keys[i] > p->pageNum) {
			shiftIntArray(n->keys, i, M);
			n->keys[i] = p->pageNum;
			shiftPageArray((page**) n->children, i, M);
			n->children[i] = p;
			n->childCount++;
			return;
		}
	}
	// otherwise, the correct spot must be at the end
	n->keys[n->childCount] = p->pageNum;
	n->children[n->childCount] = p;
	n->childCount++;
	return;
}


// UNTESTED
// splits a node, making sure the new node is properly connected to the b+tree
// assumes the parent node is not full
node* splitNode(node* n) {
	if (isNodeFull(n->parent)) printf("Error: tried to split a node with a full parent");
	node* new = newNode(n->isLeaf, n->parent);

	int halfwayPoint = n->childCount/2;
	// inserting new node into parent's keys and pointers
	for (int i = 0; i < n->parent->childCount; i++) {
		if (n->parent->children[i] == n){
			shiftIntArray(n->parent->keys, i, M);
			shiftNodeArray((node**) n->parent->children, i+1, M);
			if (n->isLeaf) {
				n->parent->keys[i] = ((page*) n->children[halfwayPoint])->pageNum;
			} else {
				n->parent->keys[i] = ((node*) n->children[halfwayPoint])->maxPageNumber;
			}
			n->parent->children[i+1] = new;
			break;
		}
	}
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
	// Inserting into linked list of leaves
	if (n->isLeaf) {
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
/*
@param n = the node to be split
*/
node* balanceTree(node* n) {
	if (!isNodeFull(n)) {
		printf("Error: called balance() on a node that wasn't full\n");
		return NULL;
	}
	if (isRoot(n)) {
		node* r = newRoot(n, 1);
	}
	// recursive step
	if (isNodeFull(n->parent)) balanceTree(n->parent);
	node* new = splitNode(n);
	return new;
}

// UNTESTED
// adds a page to a node and balances the tree recursively
void addPage(node* n, page* newPage) {
	if (isNodeFull(n)) {
		node* new = balanceTree(n);
		// decide which node to add the page to and add it
		if (newPage->pageNum > n->maxPageNumber) {
			insertPageIntoChildren(new, newPage);
			return;
		}
	}
	// else just add the new page to the node
	insertPageIntoChildren(n, newPage);
}

// UNTESTED
/*
Writes a value into a slotted page
*/
bool writeVal(page* p, int tuple) {
	if (p == NULL) return false;
	if (isPageFull(p)) {
		printf("Tried to write to page %d which was full\n", p->pageNum);
		return false;
	}
	p->records[p->numRecords] = tuple;
	p->numRecords++;
	return true;
}

// UNTESTED
/*
inserts a new tuple into the b+tree
@return - the page number of the page into which the tuple was written
*/
uint32_t insertTuple(int tuple, u_int32_t pageNum, node* tree) {
	page* p = findPage(pageNum, tree); // find page
	if (p == NULL) {
		printf("Tried to insert into page %d which doesn't exist\n", pageNum);
		return pageNum;
	}
	if (isPageFull(p)) {
		printf("Making new page\n");
		uint32_t newNum = findNextPageNum(p);
		printf("New Page Num: %u\n", newNum);
		page* newP = newPage(newNum, p->parent);
		printf("Adding new page\n");
		addPage(p->parent, newP);
		if (!writeVal(newP, tuple)) printf("Error: failed to write tuple to a page created in the insertTuple() function\n");
		return newNum;
	} else {
		if (!writeVal(p, tuple)) printf("Error: tried to write tuple to incompatible page\n");
		return p->pageNum;
	}
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// PAGE FUNCTIONS
// These may at some point be abstracted to a different file

void addRecord(page* p, record r) {
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

void deleteRecord(page* p, record r) {
	;
}

void updateRecord(page* p, record r) {
	;
}

void readRecord(page* p, record r) {
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
