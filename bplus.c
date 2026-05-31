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
tree* newTree(slotted_page* p) {
	node* new = newNode(true, NULL);
	new->childCount = 1;

	new->children[0] = p;
	new->keys[0] = p->header.pageNum;
	tree* t = malloc(sizeof(tree));
	t->root = new;
	t->pageMaxSlots = p->header.maxSlots;
	t->pageMaxEntries = p->header.maxEntries;
	t->pageCap = p->header.arrCap;
	return t;
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
int shiftIntArrayR(int* array, int start, int len) {
	if (start > len-1) {
		printf("Start index %d beyond length %d of array in shiftIntArrayR\n", start, len);
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
int shiftIntArrayL(int* array, int target, int len) {
	if (target > len-1) {
		printf("Start index %d beyond length %d of array in shiftIntArrayL\n", target, len);
		return -1;
	}
	for (int i = target; i < len-1; i++) {
		array[i] = array[i+1];
	}
	array[len-1] = 0;
	return 0;
}

int shiftPageArrayR(slotted_page** array, int start, int len) {
	if (start > len-1) {
		printf("Start index %d beyond length %d of array in shiftPageArrayR\n", start, len);
		return -1;
	}
	for (int i = len-1; i > start; i--) {
		array[i] = array[i-1];
	}
	array[start] = NULL;
	return 0;
}

int shiftPageArrayL(slotted_page** array, int target, int len) {
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

int shiftNodeArrayR(node** array, int start, int len) {
	if (start > len-1) {
		printf("Start index %d beyond length %d of array in shiftNodeArrayR\n", start, len);
		return -1;
	}
	for (int i = len-1; i > start; i--) {
		array[i] = array[i-1];
	}
	array[start] = NULL;
	return 0;
}

int shiftNodeArrayL(node** array, int target, int len) {
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
	return p->header.numRecords >= NUM_SLOTS;
}

bool isNodeFull(node* n) {
	return n->childCount >= M;
}

bool nodeAtMinimum(node* n) {
	return n->childCount <= HALF_M;
}

bool isRoot(node* n) {
	return n->parent == NULL && n->childCount > 0;
}

// finds a page in a tree by page number
// returns null if page is not in tree
slotted_page* findPage(uint32_t pageNum, tree* t) {
	node* tree = t->root;
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
        slotted_page* p = (slotted_page*)tree->children[i];
        if (p->header.pageNum == pageNum) {
            return p;
        }
    }
	return NULL;
}

/*
finds a page in a tree by page number and returns it
if the page does not exist, creates a page in the right spot and returns it
*/
slotted_page* findAndInsert(uint32_t pageNum, tree* t) {
	node* tree = t->root;
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
            return (slotted_page*) tree->children[i];
        } else if (key > pageNum) { // optimization so that we don't have to unnecessarily finish a loop
			slotted_page* p = makeSPage(pageNum, t->pageMaxSlots, t->pageMaxEntries, t->pageCap);
			addPage(tree, p, t);
			return p;
		}
    }
	slotted_page* p = makeSPage(pageNum, t->pageMaxSlots, t->pageMaxEntries, t->pageCap);
    addPage(tree, p, t);
	return p;
}

// UNTESTED
/*
Searches for a page by number in the tree and deletes it
@return true - page is found and deleted
@return false - page deletion was unsuccessful
*/
bool findAndDelete(uint32_t pageNum, tree* t) {
	node* tree = t->root;
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
            return deletePage(tree, pageNum, t);
		}
    }
	return false;
}


/* UNTESTED
returns an internal node's next sibling (not cousin)
*/
node* getNextInternal(node* n) {
	node* parent = n->parent;
	if (!parent) return NULL;
	int maxkey = n->keys[n->childCount-2];
	for (int i = 0; i < parent->childCount-1; i++) { // search for n in parent's children and return next node if found
		if (maxkey <= parent->keys[i]) return (node*) parent->children[i+1];
	}
	return NULL; // otherwise n is the last node so there's no viable next
}

/* UNTESTED
returns an internal node's previous sibling (not cousin)
assumes a node is internal
*/
node* getPrevInternal(node* n) {
	node* parent = n->parent;
	if (!parent) return NULL;
	if (parent->children[0] == n) return NULL;
	int maxKey = n->keys[n->childCount-2];
	for (int i = 0; i < parent->childCount-1; i++) {
		if (maxKey <= parent->keys[i]) return (node*) parent->children[i-1];
	}
	return parent->children[parent->childCount-2]; // if not found in node then n is the last node and second to last node should be returned
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

uint32_t updateMaxPageNum(node* n) {
	if (n->isLeaf) return ((slotted_page*) n->children[n->childCount-1])->header.pageNum;
	return ((node*) n->children[n->childCount-1])->maxPageNumber;
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// INSERTION FUNCTIONS

/*
puts a page into a parent node's children and keys arrays
assumes node is not full
*/
void insertPageIntoChildren(node* n, slotted_page* p) {
	p->header.parent = n;
	// check if page should be inserted into the middle of the children
	for (int i = 0; i < n->childCount; i++) {
		if (n->keys[i] > p->header.pageNum) {
			shiftIntArrayR(n->keys, i, M);
			n->keys[i] = p->header.pageNum;
			shiftPageArrayR((slotted_page**) n->children, i, M);
			n->children[i] = p;
			n->childCount++;
			return;
		}
	}
	// otherwise, the correct spot must be at the end
	n->keys[n->childCount] = p->header.pageNum;
	n->children[n->childCount] = p;
	n->childCount++;
	n->maxPageNumber = p->header.pageNum;
	return;
}

/*
puts a node into a parent node's children and keys arrays
assumes node is not full
*/
void splitUpdateParent(node* parent, node* child, int newKey, tree* t) {
	if (isNodeFull(parent)) {
		printf("Balancing parent from splitUpdateParent()\n");
		balanceTreeAdd(parent, t);
	}
	// look for correct spot in parent's keys
	for (int i = 0; i < parent->childCount-1; i++) {
		if (parent->keys[i] > newKey) {
			shiftIntArrayR(parent->keys, i, M);
			parent->keys[i] = newKey;
			shiftNodeArrayR((node**) parent->children, i+1, M);
			/*parent->children[i+1] = parent->children[i+2];
			parent->children[i+2] = child;*/
			parent->children[i+1] = child;
			parent->childCount++;
			return;
		}
	}
	// otherwise, the correct spot must be at the end
	parent->keys[parent->childCount-1] = newKey; // no previous key to replace
	parent->children[parent->childCount] = child;
	parent->childCount++;
	return;
}

// splits a node, making sure the new node is properly connected to the b+tree
// assumes the parent node is not full
node* splitNode(node* n, tree* t) {
	if (isNodeFull(n->parent)) printf("Error: tried to split a node with a full parent");
	node* new = newNode(n->isLeaf, n->parent);
	int middleKid = n->childCount/2;
	// copying over children and keys
	for (int i = 0; i < middleKid; i++) {
		new->children[i] = n->children[i + middleKid];
		if (n->isLeaf) ((slotted_page*) new->children[i])->header.parent = new;
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
	splitUpdateParent(n->parent, new, n->maxPageNumber, t);
	return new;
}

/*
@param n = the node to be split
*/
node* balanceTreeAdd(node* n, tree* t) {
	if (!isNodeFull(n)) {
		printf("Error: called balanceTreeAdd() on a node that wasn't full\n");
		return NULL;
	}
	if (isRoot(n)) {
		node* r = newRoot(n);
		t->root = r;
	}
	// recursive step
	// need to make sure new node is put into the correct node
	else if (isNodeFull(n->parent)) {
		node* newP = balanceTreeAdd(n->parent, t);
	}
	node* new = splitNode(n, t);
	return new;
}

// adds a page to a node and balances the tree recursively
void addPage(node* n, slotted_page* newPage, tree* t) {
	if (isNodeFull(n)) {
		node* new = balanceTreeAdd(n, t);
		// decide which node to add the page to and add it
		if (newPage->header.pageNum > n->maxPageNumber) {
			insertPageIntoChildren(new, newPage);
			return;
		}
	}
	// else just add the new page to the node
	insertPageIntoChildren(n, newPage);
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// DELETION FUNCTIONS

// UNTESTED
/*
Assumes n's siblings are empty enough to merge with n since merging should only be done if borrowing fails
*/
node* mergeNode(node* n, tree* t) {
	printf("Merging nodes\n");
	node* survivor = n;
	node* source;
	// some operations differ whether n is a leaf node or not
	if (n->isLeaf) {
		// determine source and survivor
		source = n->next;
		if (n->prev && n->prev->parent == n->parent) {
			survivor = n->prev;
			source = n;
		}
		// copy keys and children
		for (int i = 0; i < source->childCount; i++) {
			survivor->keys[survivor->childCount] = source->keys[i];
			survivor->children[survivor->childCount++] = source->children[i];
		}

		// update linked list
		if (source->next) {
			survivor->next = source->next;
			source->next->prev = survivor;
		}
	// n is an internal node
	} else {
		// determine source and survivor
		node* prev = getPrevInternal(n);
		if (prev && prev->parent == n->parent) {
			survivor = prev;
			source = n;
		} else {
			source = getNextInternal(n);
		}
		// copy keys and children
		for (int i = 0; i < source->childCount; i++) {
			survivor->keys[survivor->childCount-1] = source->keys[i]; // copies extra garbage key
			survivor->children[survivor->childCount++] = source->children[i];
		}
	}
	// update MPN
	survivor->maxPageNumber = source->maxPageNumber;
	// update parent
	node* parent = source->parent;
	for (int i = 1; i < parent->childCount; i++) {
		if (parent->children[i] == source) {
			shiftNodeArrayL(((node**) parent->children), i, M);
			shiftIntArrayL(parent->keys, i-1, M);
		}
	}
	parent->childCount--;
	// conditionally balance parent, free source and return survivor
	printf("Freeing node at %p\n", source);
	free(source);
	if (parent->childCount < HALF_M) balanceTreeDelete(parent, t);
	return survivor;
}

/* valid borrow targets:
exist
have more than M/2 children
have the same parent as n
*/
bool isValidBorrow(node* n, node* target) {
	return (target && target->childCount > M/2 && target->parent == n->parent);
}

// assumes that next is a valid target for a borrow
void borrowNext(node* n, node* next) {
	n->children[n->childCount] = next->children[0];
	n->keys[n->childCount++] = next->keys[0];
	next->childCount--;
	shiftPageArrayL((slotted_page**) next->children, 0, M);
	shiftIntArrayL(next->keys, 0, M);
	// update parent
	int borrowed = n->keys[n->childCount-1];
	for (int i = 0; i < n->parent->childCount; i++) {
		if (borrowed > n->parent->keys[i]) {
			n->parent->keys[i] = borrowed;
			return;
		}
	}
	printf("Error: Function borrowNext() borrowed a key from next that was less than a key from n\n");
	return;

}

// assumes that prev is a valid target for a borrow
void borrowPrev(node* n, node* prev) {
	shiftIntArrayR(n->keys, 0, M);
	shiftPageArrayR((slotted_page**) n->children, 0, M);
	prev->childCount--;
	n->keys[0] = prev->keys[prev->childCount];
	n->children[0] = prev->children[prev->childCount];
	prev->keys[prev->childCount] = 0;
	prev->children[prev->childCount] = NULL;
	n->childCount++;
	// update parent
	int borrowed = n->keys[0];
	for (int i = 0; i < n->parent->childCount; i++) {
		if (borrowed == n->parent->keys[i]) {
			n->parent->keys[i] = prev->keys[prev->childCount-1];
			return;
		}
	}
}

/*
implements b+ tree borrowing for internal nodes
assumes n, next and their parent are valid and internal nodes
*/
void borrowNextThroughParent(node* n, node* next) {
	node* parent = n->parent;
	for (int i = 0; i < parent->childCount; i++) {
		if (parent->children[i] == n) {
			n->keys[n->childCount-1] = parent->keys[i];
			parent->keys[i] = next->keys[0];
			n->children[n->childCount++] = n->children[0];
			n->maxPageNumber = ((node*) n->children[0])->maxPageNumber;
			shiftIntArrayL(next->keys, 0, M-1);
			shiftNodeArrayL((node**) next->children, 0, M);
			next->childCount--;
			next->children[next->childCount] = NULL;
			next->keys[next->childCount-1] = 0;
			return;
		}
	}
}

void borrowPrevThroughParent(node* n, node* prev) {
	node* parent = n->parent;
	for (int i = 0; i < parent->childCount; i++) {
		if (parent->children[i] == n) {
			shiftIntArrayR(n->keys, 0, M);
			shiftNodeArrayR((node**) n->children, 0, M);
			n->keys[0] = parent->keys[parent->childCount-2];
			prev->childCount--;
			n->children[0] = prev->children[prev->childCount];
			parent->keys[parent->childCount-2] = prev->keys[prev->childCount-1];
			prev->children[prev->childCount] = NULL;
			prev->keys[prev->childCount-1] = 0;
			return;
		}
	}
}

// UNTESTED
node* balanceTreeDelete(node* n, tree* t) {
	// if n is a leaf node
	if (n->isLeaf) { // needs to come before root case since a node that is both a root and a leaf can have one page child
		node* next = n->next;
		if (isValidBorrow(n, next)) {
			borrowNext(n, next);
			if (n->parent->childCount < HALF_M) balanceTreeDelete(n->parent, t);
			return n;
		}
		node* prev = n->prev;
		if (isValidBorrow(n, prev)) {
			borrowPrev(n, prev);
			if (n->parent->childCount < HALF_M) balanceTreeDelete(n->parent, t);
			return n;
		}
		return mergeNode(n, t);
	// if n is a root node
	} else if (n == t->root && n->childCount == 1) {
		node* r = ((node*) n->children[0]);
		t->root = r;
		r->parent = NULL;
		printf("Collapsing tree\n");
		free(n);
		return r;
	// if n is an internal node
	} else {
		node* next = getNextInternal(n);
		if (isValidBorrow(n, next)) {
			borrowNextThroughParent(n, next);
			return n;
		}
		node* prev = getPrevInternal(n);
		if (isValidBorrow(n, prev)) {
			borrowPrevThroughParent(n, prev);
			return n;
		}
		return mergeNode(n, t);
	}
}


// UNTESTED
/*
Deletes a page from a node
@return - whether the page was successfully deleted or not
*/
bool deletePage(node* n, uint32_t pageNum, tree* t)  {
	if (!n->isLeaf) {
		printf("Error: Tried to delete page in inner node\n");
		return false;
	}
	if (n->childCount == HALF_M && !isRoot(n)) {
		n = balanceTreeDelete(n, t);
	}
	// search for page in node's children
	for (int i = 0; i < n->childCount; i++) {
		if (n->keys[i] == pageNum) {
			freePage((slotted_page*) n->children[i]);;
			shiftIntArrayL(n->keys, i, M);
			shiftPageArrayL((slotted_page**) n->children, i, M);
			n->childCount--;
			if (i == n->childCount) n->maxPageNumber = n->keys[n->childCount-1];
			return true;
		}
	}
	// otherwise, page doesn't exist in the node
	printf("Error: Tried to delete page %d from node at %p, but page was not found\n", pageNum, n);
	return false;
}

// need deleteTuple function


// ##########################################################################################################################################
// ##########################################################################################################################################
// DEBUGGING FUNCTIONS
// These are here not in debug.c because I'm not sure which of these will be kept when this is reimplemented on disk

void freePage(slotted_page* p) {
	free(p);
}

void freeTreeHelper(node* r) {
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

void freeTree(tree* t) {
	freeTreeHelper(t->root);
	free(t);
}
