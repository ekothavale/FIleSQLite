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

#ifndef TABLEIO_H
#define TABLEIO_H

#include <stdio.h>
#include "common.h"
#include "page.h"
#include "node.h"

#define MAGIC 0xFACE3419
#define METALEN 16 // number of 4-byte words needed to represent a table's metadata

typedef struct page_write_order {
	slotted_page* page;
	uint64_t address;
}page_write_order;

typedef struct node_write_order {
	node* node;
	uint64_t address;
}node_write_order;

typedef struct page_stack {
	uint32_t size;
	uint32_t count;
	page_write_order* stack;
}page_stack;

typedef struct node_stack {
	uint32_t size;
	uint32_t count;
	node_write_order* stack;
}node_stack;

typedef struct delete_stack {
	uint32_t size;
	uint32_t count;
	uint64_t* stack;
}delete_stack;

typedef struct table {
	page_stack pageDirty; // stack of dirty pages
	node_stack nodeDirty; // stack of dirty nodes
	FILE* source; // physical file
	u_int64_t cursor; // current address
	u_int64_t pageFree; // address of next free page space
	u_int64_t nodeFree; // address of next free node space
	u_int64_t root; // pointer to the root of the tree
	slotted_page* page; // pointer to current object if it's a page
	node* node; // pointer to current object if it's a node
	int metalen; // size of the metadata in the table file in bytes
	int pageStripes; // number of page stripes in file
	int pageStripeLen; // number of pages per page stripe
	int nodeStripes; // number of node stripes in file
	int nodeStripeLen; // number of nodes per stripe
	int pageNodeRatio; // how many page stripes there are per node stripe
	int pageSize; // size of page in bytes
	int nodeSize; // size of node in bytes
	int M; // maximum number of children each node can have
}table;

// load page
bool loadPage(uint64_t address, table* t);
// load node
bool readNode(uint64_t address, node* n, table* t);
void loadNode(uint64_t address, table* t);
// write page
void writeNextPage(table* t);
void writePage(uint32_t address, table* t); // all pages will be looked up in the dirty queue by their address
// write node
void writeNextNode(table* t);
void writeNode(uint32_t address, table* t);
// load parent
void loadParent(node* n, node* parent, table* t);
// load previous node (left node)
void loadPrev(node* n, node* prev, table* t);
// load next node (right node)
void loadNext(node* n, node* next, table* t);
// add page to dirty queue
void markPage(uint64_t address, slotted_page* p, table* t);
// add node to dirty queue
void markNode(uint64_t address, node* n, table* t);
// mark an object for deletion
void markDelete(uint64_t address, table* t);
// allocate new stripe
void newStripe(table* t);
uint64_t allocNode(table* t);
uint64_t allocPage(table* t);
// condense a stripe
void condenseStripe(table* t);
// condense all stripes
void condenseAll(table* t);

#endif