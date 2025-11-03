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
 - Each key is a 32bit int
 - Last n bits represent the slot number within a page
 - Remaining bits represent the page number
 - n = number of slots per page
 - How many slots should be in a page?
 - How big are pages
*/


#include "bplus.h"
#include "common.h"

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

node* root;

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

// creates a new b+ tree starting with just one leaf node
node* newTree() {
	node* new = malloc(sizeof(node));
	new->childCount = 0;
	new->isLeaf = true;
	new->parent = NULL;
	new->prev = NULL;
	new->next = NULL;
	return new;
}

// shifts the elements of an array right by 1 starting at the given index
// assumes there is a free space in the array
// len is the total length of the array
// start is the free space to be created
void shiftIntArray(int* array, int start, int len) {
	for (int i = len-1; i > start; i--) {
		array[i] = array[i-1];
	}
}

// UNTESTED
void shiftPageArray(page** array, int start, int len) {
	for (int i = len-1; i > start; i--) {
		array[i] = array[i-1];
	}
}

// UNTESTED
void shiftNodeArray(node** array, int start, int len) {
	for (int i = len-1; i > start; i--) {
		array[i] = array[i-1];
	}
}

// UNTESTED
// finds a page in a tree by page key
page* findPage(int key, node* tree) {
	int pageNum = key & (2147483647 - NUM_SLOTS + 1);
	// check if new tree
	if (tree->isLeaf && tree->childCount == 0) { // likely want to abstract this out of the function for small performance upgrade
		return tree; 
	}
	// compare pageNum against keys in node
	while(!tree->isLeaf) {
		for (int i = 0; i < tree->childCount-2; i++) {
			// searching for correct key position
			if ((i == 0 && pageNum <= tree->keys[i]) || (pageNum <= tree->keys[i] && pageNum > tree->keys[i-1])) {
				tree = tree->children[i];
				break;
			}
		// if key wasn't less than any inidices then the last child is the correct path
		tree = tree->children[tree->childCount-1];
		}
	}
	// we have found the correct leaf
	for (int i = 0; i < tree->childCount; i++) {
		// search for correct page
		if (i == 0 && pageNum <= tree->keys[i] || pageNum <= tree->keys[i] && pageNum > tree->keys[i-1]) {
			return tree->children[i];
		}
	}
	return tree->children[tree->childCount-1];

}

// UNTESTED
bool isFull(page* p) {
	return p->usedSlots >= NUM_SLOTS || p->usedMem >= PAGE_CAPACITY;
}

// UNTESTED
bool isFull(node* n) {
	return n->childCount >= M;
}

// UNTESTED
bool isRoot(node* n) {
	return n->parent == NULL && n->childCount > 0;
}

// UNTESTED
bool writeVal(page* p, int val) {
	if (isFull(p)) return false;
	p->stackTop--;
	*(p->stackTop) = val;
	p->usedMem += sizeof(val);
	return true;
}

// UNTESTED
// adds a page to a node
// assumes node is not full
// if true, operation was successful
// if false, operation was not performed because conditions were not right
bool addPage(node* n, page* p) {
	if (isFull(n) || !n->isLeaf) {
		printf("Error Flagged while illegally adding page\n");
		return false; // shouldn't run in these conditions
	}
	p->parent = n;
	for (int i = 0; i < n->childCount - 1; i++) {
		if (p->pageNum < n->keys[i]) {
			// We've found the correct spot for the page
			shiftIntArray(n->keys, i, n->childCount-1);
			n->keys[i] = p->pageNum;
			shiftPageArray((page*) n->children, i, n->childCount);
			n->children[i] = p;
			n->childCount++;
			return true;
		}
	}
	// if here, new page should go last
	n->keys[n->childCount - 1] = ((page*) n->children[n->childCount-1])->pageNum;
	n->children[n->childCount] = p;
	n->childCount++;
	n->maxPageNumber = p->pageNum;
	return true;
}

// UNTESTED
// adds a child node to another node
// assumes node is not full and node is not a leaf
// if true, operation was successful
// if false, operation was not performed since the conditions weren't right
bool addNode(node* parent, node* child) {
	if (isFull(parent) || parent->isLeaf) {
		printf("Error flagged while illegally adding node\n");
		return false; // shouldn't run in these conditions
	}
	// set child->parent as parent
	child->parent = parent;
	for (int i = 0; i < parent->childCount - 1; i++) {
		if (MIN_KEY(child) > parent->keys[i]) {
			// We've found the right spot to insert the child
			shiftIntArray(parent->keys, i, parent->childCount-1);
			parent->keys[i] = child->maxPageNumber;
			shiftNodeArray(parent->keys, i, parent->childCount);
			parent->children[i] = child;
			parent->childCount++;
			return true;
		}
	}
	// if here, new node should go last
	// new key is the maximum of the previous node
	parent->keys[parent->childCount - 1] = ((node*) parent->children[parent->childCount - 1])->maxPageNumber;
	parent->children[parent->childCount] = child;
	parent->childCount++;
	parent->maxPageNumber = child->maxPageNumber;
	return true;

}

// UNTESTED
// splits a page in two, copies metadata and moves half of the stored data over
page* splitPage(page* p) {
	// allocate new page
	page* new = malloc(sizeof(page));
	// move over stored data
	for (int i = p->usedSlots/2; i < p->usedSlots; i++) {
		new->slotarr[i - p->usedSlots/2] = p->slotarr[i];
	}
	new->usedSlots = p->usedSlots - p->usedSlots/2;
	new->parent = p->parent;
	for (int i = 0; i < new->usedSlots; i++) {
		writeVal(new, *(new->slotarr[i]));
	}
	return new;
}

// UNTESTED
// splits a node
node* splitNode(node* n) {
	node* new = malloc(sizeof(node));

	new->parent = n->parent;
	
	new->isLeaf = n->isLeaf;

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

		// Updating and setting maxPageNumber
		n->maxPageNumber = ((page*) n->children[n->childCount-1])->pageNum;
		new->maxPageNumber = ((page*) new->children[new->childCount-1])->pageNum;
	}

	// Updating and setting maxPageNumber if nodes are not leaves
	n->maxPageNumber = ((node*) n->children[n->childCount-1])->maxPageNumber;
	new->maxPageNumber = ((node*) new->children[new->childCount-1])->maxPageNumber;

	return new;
}

// UNTESTED
// adds a new node to a node and balances the tree recursively
void addNodeAndBalance(node* n, node* newChild) {
	// add new node and its page number ranges as appropriate to respective arrays in n
	// if this would exceed n's capacity, split n and recursively call algorithm
	if (isFull(n)) {
		node* new = splitNode(n);
		// if current node is root node then create a new root
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
	if (isFull(n)) {
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
void addTupleAndBalance(page* p, int tuple) {
	page* new = splitPage(p);
	writeVal(p, tuple);
	addPageAndBalance(p->parent, new);
}

// UNTESTED
void insertTuple(int tuple, int pageNum, int pageOffs, node* tree) {
	page* p = findPage(pageNum, tree); // find page
	if (isFull(p)) {
		addTupleAndBalance(p, tuple);
	}
}


/*
TODO:
 - Implement insertion
 - Implement deletion
 - make sure tree is balanced for both
 - Test all algorithms
*/