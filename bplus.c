/*
Copyright (c) 2026 Ethan Kothavale

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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

// eventually clamp all the Ms in the shiftArray calls to their proper values

#include "bplus.h"

// ##########################################################################################################################################
// ##########################################################################################################################################
// TREE CREATION FUNCTIONS

/*
creates a new root node
*/
node* newRoot(node* child, uint64_t childAddr, table* t) {
	node* new = malloc(t->nodeSize);
	new->childCount = 1;
	new->children[0] = childAddr;
	new->parent = 0;
	new->prev = 0;
	new->next = 0;
	new->isLeaf = false;
	new->maxPageNumber = child->maxPageNumber;
	uint64_t rootAdd = allocNode(t);
	child->parent = rootAdd;
	markNode(childAddr, child, t);
	markNode(rootAdd, new, t);
	t->root = rootAdd;
	return new;
}

/*
creates a blank leaf or interior node
*/
node* newNode(bool isLeaf, uint64_t parent) {
	node* n = calloc(1, sizeof(node));
	n->isLeaf = isLeaf;
	n->parent = parent;
	n->childCount = 0;
	n->maxPageNumber = 0;
	return n;
}

/*
creates and initializes a new table file and fills it with a new b+ tree, with one empty node and one empty page
@param pageNum - the page number of the starting page
@return - table struct containing the necessary data to use the table
mallocs new memory (table)
*/
table* createTree(char* tablename, uint32_t pageNum) {
	// create structs
	table* t = createTable(tablename);
	node* root = calloc(1, sizeof(node));
	uint64_t rootAddr = allocNode(t);
	slotted_page* page = makeSPage(pageNum, PAGE_NUM_SLOTS, PAGE_NUM_ENTRIES, PAGE_ARR_CAP);
	uint64_t pageAddr = allocPage(t);

	// initialize struct members (root and page already 0ed out)
	root->childCount = 1;
	root->children[0] = pageAddr;
	root->keys[0] = pageNum;
	root->isLeaf = true;
	root->maxPageNumber = pageNum;

	page->header.parent = rootAddr;

	// write structs and clean up
	writeNewTree(page, pageAddr, root, rootAddr, t);
	free(root);
	free(page);
	return t;
}

void deleteTree(table* t) {
	deleteTable(t);
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
int shiftUIntArrayR(uint32_t* array, int start, int len) {
	if (start > len-1) {
		printf("Start index %d beyond length %d of array in shiftUIntArrayR\n", start, len);
		return -1;
	}
	for (int i = len-1; i > start; i--) {
		array[i] = array[i-1];
	}
	array[start] = 0;
	return 0;
}

/*
shifts the elements of an array left, overwriting target
@input target - the first spot to be overwritten
@input len - the total length of the array (or at least the values you care about)
*/
int shiftUIntArrayL(uint32_t* array, int target, int len) {
	if (target > len-1) {
		printf("Start index %d beyond length %d of array in shiftUIntArrayL\n", target, len);
		return -1;
	}
	for (int i = target; i < len-1; i++) {
		array[i] = array[i+1];
	}
	array[len-1] = 0;
	return 0;
}

int shiftAddressArrayR(uint64_t* array, int start, int len) {
	if (start > len-1) {
		printf("Start index %d beyond length %d of array in shiftAddressArrayR\n", start, len);
		return -1;
	}
	for (int i = len-1; i > start; i--) {
		array[i] = array[i-1];
	}
	array[start] = 0;
	return 0;
}

int shiftAddressArrayL(uint64_t* array, int target, int len) {
	if (target > len-1) {
		printf("Start index %d beyond length %d of array in shiftPageArrayL\n", target, len);
		return -1;
	}
	for (int i = target; i < len-1; i++) {
		array[i] = array[i+1];
	}
	array[len-1] = 0;
	return 0;
}

bool isPageFull(slotted_page* p) {
	return p->header.numRecords >= PAGE_NUM_SLOTS;
}

bool isNodeFull(node* n) {
	return n->childCount >= M_GLOBAL;
}

bool nodeAtMinimum(node* n) {
	return n->childCount <= HALF_M;
}

bool isRoot(node* n) {
	return n->parent == 0 && n->childCount > 0;
}


// finds a page in a tree by page number and returns its address
// returns null if page is not in tree
uint64_t findPage(uint32_t pageNum, table* t) {
	loadNode(t->root, t);
    if (t->node == NULL || t->node->childCount == 0) {
        printf("Attempted to find page in invalid tree\n");
        return 0; // input was an invalid tree
    }
	if (pageNum > t->node->maxPageNumber) return 0;
    // Compare pageNum against keys in node
    while (!t->node->isLeaf) {
        int found = 0;
        for (int i = 0; i < t->node->childCount - 1; i++) {
            // Searching for the correct key position
            if (pageNum <= t->node->keys[i]) {
                loadNode(t->node->children[i], t);
                found = 1;
                break;
            }
        }
        // If key wasn't less than any indices, the last child is the correct path
        if (!found) {
            loadNode(t->node->children[t->node->childCount - 1], t);
        }
    }
    // We have found the correct leaf
    for (int i = 0; i < t->node->childCount; i++) {
        if (t->node->keys[i] == pageNum) {
            return t->node->children[i];
        }
    }
	return 0;
}

/*
finds a page in a tree by page number and returns its address
if the page does not exist, creates a page in the right spot and returns it
*/
uint64_t findAndInsert(uint32_t pageNum, table* t) {
	loadNode(t->root, t);
    if (t->node == NULL || t->node->childCount == 0) {
        printf("Attempted to find page in invalid tree\n");
        return 0; // input was an invalid tree
    }
    // Compare pageNum against keys in node
    while (!t->node->isLeaf) {
        int found = 0;
        for (int i = 0; i < t->node->childCount - 1; i++) {
            // Searching for the correct key position
            if (pageNum <= t->node->keys[i]) {
                loadNode(t->node->children[i], t);
                found = 1;
                break;
            }
        }
        // If key wasn't less than any indices, the last child is the correct path
        if (!found) {
            loadNode(t->node->children[t->node->childCount - 1], t);
        }
    }
    // We have found the correct leaf
    for (int i = 0; i < t->node->childCount; i++) {
        uint32_t key = t->node->keys[i];
        if (key == pageNum) {
            return t->node->children[i];
        } else if (key > pageNum) { // optimization so that we don't have to unnecessarily finish a loop
			slotted_page* p = makeSPage(pageNum, PAGE_NUM_SLOTS, PAGE_NUM_ENTRIES, PAGE_ARR_CAP);
			uint64_t pageAddr = allocPage(t);
			addPage(t->node, t->cursor, p, pageAddr, t);
			return pageAddr;
		}
    }
	slotted_page* p = makeSPage(pageNum, PAGE_NUM_SLOTS, PAGE_NUM_ENTRIES, PAGE_ARR_CAP);
	uint64_t pageAddr = allocPage(t);
    addPage(t->node, t->cursor, p, pageAddr, t);
	return pageAddr;
}

/*
Searches for a page by number in the tree and deletes it
@return true - page is found and deleted
@return false - page deletiosn was unsuccessful
*/
bool findAndDelete(uint32_t pageNum, table* t) {
	loadNode(t->root, t);
    if (t->node == NULL || t->node->childCount == 0) {
        printf("Attempted to find page in invalid tree\n");
        return NULL; // input was an invalid tree
    }
    // Compare pageNum against keys in node
    while (!t->node->isLeaf) {
        int found = 0;
        for (int i = 0; i < t->node->childCount - 1; i++) {
            // Searching for the correct key position
            if (pageNum <= t->node->keys[i]) {
                loadNode(t->node->children[i], t);
                found = 1;
                break;
            }
        }
        // If key wasn't less than any indices, the last child is the correct path
        if (!found) {
            loadNode(t->node->children[t->node->childCount - 1], t);
        }
    }
    // We have found the correct leaf
    for (int i = 0; i < t->node->childCount; i++) {
        uint32_t key = t->node->keys[i];
        if (key == pageNum) {
            return deletePage(t->node, t->cursor, pageNum, t);
		}
    }
	return false;
}


/*
returns the address of an internal node's next sibling (not cousin)
*/
uint64_t getNextInternal(node* n, uint64_t nAddr, table* t) {
	if (!n->parent) return 0;
	node parent;
	loadParent(n, &parent, t);
	for (int i = 0; i < parent.childCount - 1; i++) {
		if (parent.children[i] == nAddr) return parent.children[i+1];
	}
	return 0;
}

/*
returns the address of an internal node's previous sibling (not cousin)
assumes a node is internal
*/
uint64_t getPrevInternal(node* n, uint64_t nAddr, table* t) {
	if (!n->parent) return 0;
	node parent;
	loadParent(n, &parent, t);
	for (int i = 1; i < parent.childCount; i++) {
		if (parent.children[i] == nAddr) return parent.children[i-1];
	}
	return 0;
}

/*
Given a page, find the next available page number
@return 0 for failure
There is no guarantee this function will return a page number that is legal by the properties of a search tree.
Probably a good target for refactoring after the first draft of the tree.
*/
uint32_t findNextPageNum(slotted_page* p) {
	return p->header.pageNum + 1;
}

uint32_t updateMaxPageNum(node* n, table* t) {
	if (n->isLeaf) {
		loadPage(n->children[n->childCount-1], t);
		return t->page->header.pageNum;
	}
	node child;
	readNode(n->children[n->childCount-1], &child, t);
	return child.maxPageNumber;
}


// ##########################################################################################################################################
// ##########################################################################################################################################
// INSERTION FUNCTIONS

/*
puts a page into a parent node's children and keys arrays
assumes node is not full
*/
void insertPageIntoChildren(node* n, uint64_t nodeAddr, slotted_page* p, uint64_t pageAddr, table* t) {
	p->header.parent = nodeAddr;
	// check if page should be inserted into the middle of the children
	for (int i = 0; i < n->childCount; i++) {
		if (n->keys[i] > p->header.pageNum) {
			shiftUIntArrayR(n->keys, i, M_GLOBAL);
			n->keys[i] = p->header.pageNum;
			shiftAddressArrayR(n->children, i, M_GLOBAL);
			n->children[i] = pageAddr;
			n->childCount++;
			markNode(nodeAddr, n, t);
			return;
		}
	}
	// otherwise, the correct spot must be at the end
	n->keys[n->childCount] = p->header.pageNum;
	n->children[n->childCount] = pageAddr;
	n->childCount++;
	n->maxPageNumber = p->header.pageNum;
	markNode(nodeAddr, n, t);
	return;
}

/*
puts a node into a parent node's children and keys arrays
assumes node is not full
*/
void splitUpdateParent(node* parent, node* child, uint64_t childAddr, int newKey, table* t) {
	if (isNodeFull(parent)) {
		printf("Balancing parent from splitUpdateParent()\n");
		uint64_t dummy;
		balanceTreeAdd(parent, child->parent, &dummy, t);
	}
	// look for correct spot in parent's keys
	for (int i = 0; i < parent->childCount-1; i++) {
		if (parent->keys[i] > newKey) {
			shiftUIntArrayR(parent->keys, i, M_GLOBAL);
			parent->keys[i] = newKey;
			shiftAddressArrayR(parent->children, i+1, M_GLOBAL);
			/*parent->children[i+1] = parent->children[i+2];
			parent->children[i+2] = child;*/
			parent->children[i+1] = childAddr;
			parent->childCount++;
			return;
		}
	}
	// otherwise, the correct spot must be at the end
	parent->keys[parent->childCount-1] = newKey; // no previous key to replace
	parent->children[parent->childCount] = childAddr;
	parent->childCount++;
	markNode(child->parent, parent, t);
	return;
}

// splits a node, making sure the new node is properly connected to the b+tree
// assumes the parent node is not full
node* splitNode(node* n, uint64_t address, uint64_t* newAddrOut, table* t) {
	//if (isNodeFull(n->parent)) printf("Error: tried to split a node with a full parent");
	node* new = newNode(n->isLeaf, n->parent);
	uint64_t newAddr = allocNode(t);
	*newAddrOut = newAddr;
	int middleKid = n->childCount / 2;

	// save separator key before modifying keys (internal nodes only)
	uint32_t separatorKey = n->keys[middleKid - 1];

	// copy children — update their parent pointer on disk
	for (int i = 0; i < middleKid; i++) {
		new->children[i] = n->children[i + middleKid];
		n->children[i + middleKid] = 0;
	}

	// copy keys: leaf gets middleKid keys; internal promotes middleKid-1 (separator goes to parent)
	if (n->isLeaf) {
		for (int i = 0; i < middleKid; i++) {
			new->keys[i] = n->keys[i + middleKid];
			n->keys[i + middleKid] = 0;
		}
	} else {
		for (int i = 0; i < middleKid - 1; i++) {
			new->keys[i] = n->keys[i + middleKid];
			n->keys[i + middleKid] = 0;
		}
		n->keys[middleKid - 1] = 0; // clear promoted separator from n
	}

	new->childCount = middleKid;
	n->childCount -= middleKid;

	// insert new node into the leaf linked list
	if (n->isLeaf) {
		if (n->next) {
			uint64_t oldNext = n->next;
			n->next = newAddr;
			new->prev = address;
			new->next = oldNext;
			node tmp;
			readNode(oldNext, &tmp, t);
			tmp.prev = newAddr;
			markNode(oldNext, &tmp, t);
		} else {
			n->next = newAddr;
			new->prev = address;
		}
	}

	// update maxPageNumber for both halves
	new->maxPageNumber = n->maxPageNumber;
	if (n->isLeaf) {
		n->maxPageNumber = n->keys[n->childCount - 1];
	} else {
		n->maxPageNumber = n->keys[n->childCount - 2];
	}

	// insert new node into parent
	uint32_t splitKey = n->isLeaf ? n->maxPageNumber : separatorKey;
	node parent;
	readNode(n->parent, &parent, t);
	splitUpdateParent(&parent, new, newAddr, splitKey, t);
	markNode(address, n, t);
	markNode(newAddr, new, t);
	return new;
}

/*
@param n = the node to be split
*/
node* balanceTreeAdd(node* n, uint64_t address, uint64_t* newAddrOut, table* t) {
	if (!isNodeFull(n)) {
		printf("Error: called balanceTreeAdd() on a node that wasn't full\n");
		return NULL;
	}
	node parent;
	readNode(n->parent, &parent, t);
	if (isRoot(n)) {
		newRoot(n, address, t);
	} else if (isNodeFull(&parent)) {
		uint64_t dummy;
		balanceTreeAdd(&parent, n->parent, &dummy, t);
	}
	return splitNode(n, address, newAddrOut, t);
}

// adds a page to a node and balances the tree recursively
void addPage(node* n, uint64_t nodeAddr, slotted_page* p, uint64_t pageAddr, table* t) {
	if (isNodeFull(n)) {
		uint64_t newNodeAddr;
		node* new = balanceTreeAdd(n, nodeAddr, &newNodeAddr, t);
		if (p->header.pageNum > n->maxPageNumber) {
			insertPageIntoChildren(new, newNodeAddr, p, pageAddr, t);
			return;
		}
	}
	insertPageIntoChildren(n, nodeAddr, p, pageAddr, t);
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// DELETION FUNCTIONS

/*
Assumes n's siblings are empty enough to merge with n since merging should only be done if borrowing fails
returns the address of the surviving node
*/
uint64_t mergeNode(node* n, uint64_t addr, table* t) {
	printf("Merging nodes\n");
	node* survivor = n;
	uint64_t survAddr = addr;
	node* source = malloc(t->nodeSize);
	uint64_t sourceAddr;
	node* prev = malloc(t->nodeSize);
	// some operations differ whether n is a leaf node or not
	if (n->isLeaf) {
		// determine source and survivor
		loadNext(n, source, t);
		sourceAddr = n->next;
		loadPrev(n, prev, t);
		if (n->prev && prev->parent == n->parent) {
			survivor = prev;
			survAddr = n->prev;
			source = n;
			sourceAddr = addr;
		}
		// copy keys and children
		for (int i = 0; i < source->childCount; i++) {
			survivor->keys[survivor->childCount] = source->keys[i];
			survivor->children[survivor->childCount++] = source->children[i];
		}

		// update linked list
		if (source->next) {
			survivor->next = source->next;
			node tmp;
			readNode(source->next, &tmp, t);
			tmp.prev = survAddr;
			markNode(survivor->next, &tmp, t);
		}
	// n is an internal node
	} else {
		// determine source and survivor
		uint64_t prevAddr = getPrevInternal(n, addr, t);
		if (prevAddr) readNode(prevAddr, prev, t);
		if (prevAddr && prev->parent == n->parent) {
			survivor = prev;
			survAddr = prevAddr;
			source = n;
			sourceAddr = addr;
		} else {
			sourceAddr = getNextInternal(n, addr, t);
			readNode(sourceAddr, source, t);
		}
		// copy keys and children
		for (int i = 0; i < source->childCount; i++) {
			survivor->keys[survivor->childCount-1] = source->keys[i]; // copies extra garbage key
			survivor->children[survivor->childCount++] = source->children[i];
		}
	}
	// update MPN
	survivor->maxPageNumber = source->maxPageNumber;
	markNode(survAddr, survivor, t);
	// update parents
	node* parent = malloc(t->nodeSize);
	loadParent(n, parent, t);
	for (int i = 1; i < parent->childCount; i++) {
		if (parent->children[i] == sourceAddr) {
			shiftAddressArrayL((parent->children), i, M_GLOBAL);
			shiftUIntArrayL(parent->keys, i-1, M_GLOBAL);
		}
	}
	parent->childCount--;
	// conditionally balance parent, free source and return survivor
	printf("Deleting node at %llu\n", sourceAddr);
	markDelete(sourceAddr, t);
	if (parent->childCount < HALF_M) balanceTreeDelete(parent, n->parent, t);
	markNode(n->parent, parent, t);
	free(source);
	free(prev);
	free(parent);
	return survAddr;
}

/* valid borrow targets:
exist
have more than M/2 children
have the same parent as n
*/
bool isValidBorrow(node* n, node* target) {
	return (target && target->childCount > M_GLOBAL/2 && target->parent == n->parent);
}

// assumes that next is a valid target for a borrow
void borrowNext(node* n, uint64_t nAddr, node* next, uint64_t nextAddr, table* t) {
	n->children[n->childCount] = next->children[0];
	n->keys[n->childCount++] = next->keys[0];
	next->childCount--;
	shiftAddressArrayL(next->children, 0, M_GLOBAL);
	shiftUIntArrayL(next->keys, 0, M_GLOBAL);
	markNode(nAddr, n, t);
	markNode(nextAddr, next, t);
	// update parent
	node parent;
	loadParent(n, &parent, t);
	int borrowed = n->keys[n->childCount-1];
	for (int i = 0; i < parent.childCount; i++) {
		if (borrowed > parent.keys[i]) {
			parent.keys[i] = borrowed;
			markNode(n->parent, &parent, t);
			return;
		}
	}
	printf("Error: Function borrowNext() borrowed a key from next that was less than a key from n\n");
	return;

}

// assumes that prev is a valid target for a borrow
void borrowPrev(node* n, uint64_t nAddr, node* prev, uint64_t prevAddr, table* t) {
	shiftUIntArrayR(n->keys, 0, M_GLOBAL);
	shiftAddressArrayR(n->children, 0, M_GLOBAL);
	prev->childCount--;
	n->keys[0] = prev->keys[prev->childCount];
	n->children[0] = prev->children[prev->childCount];
	prev->keys[prev->childCount] = 0;
	prev->children[prev->childCount] = 0;
	n->childCount++;
	markNode(nAddr, n, t);
	markNode(prevAddr, prev, t);
	// update parent
	node parent;
	loadParent(n, &parent, t);
	int borrowed = n->keys[0];
	for (int i = 0; i < parent.childCount; i++) {
		if (borrowed == parent.keys[i]) {
			parent.keys[i] = prev->keys[prev->childCount-1];
			markNode(n->parent, &parent, t);
			return;
		}
	}
}

/*
implements b+ tree borrowing for internal nodes
assumes n, next and their parent are valid and internal nodes
*/
void borrowNextThroughParent(node* n, uint64_t nAddr, node* next, uint64_t nextAddr, table* t) {
	node parent;
	loadParent(n, &parent, t);
	for (int i = 0; i < parent.childCount; i++) {
		if (parent.children[i] == nAddr) {
			n->keys[n->childCount-1] = parent.keys[i];
			parent.keys[i] = next->keys[0];
			uint64_t borrowedAddr = next->children[0];
			n->children[n->childCount++] = borrowedAddr;
			node borrowed;
			readNode(borrowedAddr, &borrowed, t);
			n->maxPageNumber = borrowed.maxPageNumber;
			shiftUIntArrayL(next->keys, 0, M_GLOBAL-1);
			shiftAddressArrayL(next->children, 0, M_GLOBAL);
			next->childCount--;
			next->children[next->childCount] = 0;
			next->keys[next->childCount-1] = 0;
			markNode(nAddr, n, t);
			markNode(nextAddr, next, t);
			markNode(n->parent, &parent, t);
			return;
		}
	}
}

void borrowPrevThroughParent(node* n, uint64_t nAddr, node* prev, uint64_t prevAddr, table* t) {
	node parent;
	loadParent(n, &parent, t);
	for (int i = 1; i < parent.childCount; i++) {
		if (parent.children[i] == nAddr) {
			shiftUIntArrayR(n->keys, 0, M_GLOBAL);
			shiftAddressArrayR(n->children, 0, M_GLOBAL);
			n->keys[0] = parent.keys[i-1];
			prev->childCount--;
			n->children[0] = prev->children[prev->childCount];
			parent.keys[i-1] = prev->keys[prev->childCount-1];
			prev->children[prev->childCount] = 0;
			prev->keys[prev->childCount-1] = 0;
			markNode(nAddr, n, t);
			markNode(prevAddr, prev, t);
			markNode(n->parent, &parent, t);
			return;
		}
	}
}

uint64_t balanceTreeDelete(node* n, uint64_t addr, table* t) {
	// if n is a leaf node
	if (n->isLeaf) { // needs to come before root case since a node that is both a root and a leaf can have one page child
		node next;
		loadNext(n, &next, t);
		node* parent = malloc(sizeof(node));
		loadParent(n, parent, t);
		if (isValidBorrow(n, &next)) {
			borrowNext(n, addr, &next, n->next, t);
			if (parent->childCount < HALF_M) balanceTreeDelete(parent, n->parent, t);
			free(parent);
			return addr;
		}
		node prev;
		loadPrev(n, &prev, t);
		if (isValidBorrow(n, &prev)) {
			borrowPrev(n, addr, &prev, n->prev, t);
			if (parent->childCount < HALF_M) balanceTreeDelete(parent, n->parent, t);
			free(parent);
			return addr;
		}
		free(parent);
		return mergeNode(n, addr, t);
	// if n is a root node
	} else if (addr == t->root && n->childCount == 1) {
		node* r = malloc(sizeof(node));
		uint64_t rAddr = n->children[0];
		readNode(rAddr, r, t);
		t->root = n->children[0];
		r->parent = 0;
		markNode(n->children[0], r, t);
		printf("Collapsing tree\n");
		free(n);
		return rAddr;
	// if n is an internal node
	} else {
		uint64_t nextAddr = getNextInternal(n, addr, t);
		node nextNode;
		if (nextAddr) readNode(nextAddr, &nextNode, t);
		if (nextAddr && isValidBorrow(n, &nextNode)) {
			borrowNextThroughParent(n, addr, &nextNode, nextAddr, t);
			return addr;
		}
		uint64_t prevAddr = getPrevInternal(n, addr, t);
		node prevNode;
		if (prevAddr) readNode(prevAddr, &prevNode, t);
		if (prevAddr && isValidBorrow(n, &prevNode)) {
			borrowPrevThroughParent(n, addr, &prevNode, prevAddr, t);
			return addr;
		}
		return mergeNode(n, addr, t);
	}
}


/*
Deletes a page from a node
@return - whether the page was successfully deleted or not
*/
bool deletePage(node* n, uint64_t nAddr, uint32_t pageNum, table* t)  {
	if (!n->isLeaf) {
		printf("Error: Tried to delete page in inner node\n");
		return false;
	}
	if (n->childCount == HALF_M && !isRoot(n)) {
		readNode(balanceTreeDelete(n, nAddr, t), n, t);
	}
	// search for page in node's children
	for (int i = 0; i < n->childCount; i++) {
		if (n->keys[i] == pageNum) {
			markDelete(n->children[i], t);
			shiftUIntArrayL(n->keys, i, M_GLOBAL);
			shiftAddressArrayL( n->children, i, M_GLOBAL);
			n->childCount--;
			if (i == n->childCount) n->maxPageNumber = n->keys[n->childCount-1];
			markNode(nAddr, n, t);
			return true;
		}
	}
	// otherwise, page doesn't exist in the node
	printf("Error: Tried to delete page %d from node at %p, but page was not found\n", pageNum, n);
	return false;
}


// ##########################################################################################################################################
// ##########################################################################################################################################
// DEBUGGING FUNCTIONS
