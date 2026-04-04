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
node* newRoot(node* child) {
	node* new = malloc(sizeof(node));
	new->childCount = 1;
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
	new->keys[0] = pageNum;

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
UNTESTED
finds a page in a tree by page number and returns it
if the page does not exist, creates a page in the right spot and returns it
*/
page* findAndInsert(uint32_t pageNum, node* tree) {
    if (tree == NULL || tree->childCount == 0) {
        printf("Attempted to find page in invalid tree\n");
        return NULL; // input was an invalid tree
    }
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
        uint32_t key = tree->keys[i];
        if (key == pageNum) {
            return (page*) tree->children[i];
        } else if (key > pageNum) { // optimization so that we don't have to unnecessarily finish a loop
			page* p = newPage(pageNum, tree);
			addPage(tree, p);
			return p;
		}
    }
	page* p = newPage(pageNum, tree);
    addPage(tree, p);
	return p;
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
	p->parent = n;
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
	n->maxPageNumber = p->pageNum;
	return;
}

// UNTESTED
/*
puts a node into a parent node's children and keys arrays
assumes node is not full
*/
void splitUpdateParent(node* parent, node* child, int newKey) {
	if (isNodeFull(parent)) {
		printf("Balancing parent from splitUpdateParent()\n");
		balanceTree(parent);
	}
	// look for correct spot in parent's keys
	printf("New key: %d\n", newKey);
	printIntArray(parent->keys, M);
	for (int i = 0; i < parent->childCount-1; i++) {
		if (parent->keys[i] > newKey) {
			shiftIntArray(parent->keys, i, M);
			parent->keys[i] = newKey;
			shiftNodeArray((node**) parent->children, i+1, M);
			/*parent->children[i+1] = parent->children[i+2];
			parent->children[i+2] = child;*/
			parent->children[i+1] = child;
			parent->childCount++;
			printIntArray(parent->keys, M);
			return;
		}
	}
	// otherwise, the correct spot must be at the end
	parent->keys[parent->childCount-1] = newKey; // no previous key to replace
	parent->children[parent->childCount] = child;
	parent->childCount++;
	printIntArray(parent->keys, M);
	return;
}

// UNTESTED
// splits a node, making sure the new node is properly connected to the b+tree
// assumes the parent node is not full
node* splitNode(node* n) {
	if (isNodeFull(n->parent)) printf("Error: tried to split a node with a full parent");
	node* new = newNode(n->isLeaf, n->parent);
	int middleKid = n->childCount/2;
	// copying over children and keys
	for (int i = 0; i < middleKid; i++) {
		new->children[i] = n->children[i + middleKid];
		if (n->isLeaf) ((page*) new->children[i])->parent = new;
		else ((node*) new->children[i])->parent = new;
		n->children[i + middleKid] = NULL;
	}

	for (int i = 0; i < middleKid; i++) {
		new->keys[i] = n->keys[i + middleKid];
		n->keys[i + middleKid] = 0;
	}
	new->childCount = middleKid;
	n->childCount -= middleKid;
	// Inserting into linked list of leaves
	if (n->isLeaf) {
		if (n->next) {
			node* tmp = n->next;
			n->next = new;
			new->prev = n;
			new->next = tmp;
			tmp->prev = new;
		} else {
			n->next = new;
			new->prev = n;
		}
	}

	// Updating and setting maxPageNumber
	new->maxPageNumber = n->maxPageNumber;
	n->maxPageNumber = n->keys[n->childCount-1];
	// inserting new node into parent's keys and pointers
	splitUpdateParent(n->parent, new, n->maxPageNumber);
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
		node* r = newRoot(n);
	}
	// recursive step
	// need to make sure new node is put into the correct node
	else if (isNodeFull(n->parent)) {
		node* newP = balanceTree(n->parent);
	}
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
@return: true - successfully wrote to the specified page
@return: false - specified page was full and page number must be adjusted
*/
bool insertTuple(int tuple, u_int32_t pageNum, node* tree) {
	page* p = findAndInsert(pageNum, tree);
	if (p == NULL) {
		printf("Error: Could not find page %d while inserting\n", pageNum);
		return false;
	}
	if (isPageFull(p)) {
		printf("Tried to write to page %d but it is full\n", pageNum);
		return false;
	} else {
		if (!writeVal(p, tuple)) printf("Error: tried to write tuple to incompatible page\n");
		return true;
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
