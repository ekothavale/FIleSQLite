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
#define METALEN 14

typedef struct page_write_order {
	slotted_page* page;
	uint64_t address;
}page_write_order;

typedef struct node_write_order {
	btree_node* node;
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

typedef struct table {
	page_stack pageDirty;
	node_stack nodeDirty;
	FILE* source;
	u_int64_t cursor;
	u_int64_t free;
	u_int64_t root;
	slotted_page* page;
	int metalen;
	int pageRows;
	int pageRowLen;
	int nodeRows;
	int nodeRowLen;
	int pageNodeRatio;
	int pageSize;
	int nodeSize;
	int M;
}table;

// load page
void loadPage(table* t);
// load node
void loadNode(table* t);
// write page
void writeNextPage(table* t);
void writePage(uint32_t address, table* t); // all pages will be looked up in the dirty queue by their address
// write node
void writeNextNode(table* t);
void writeNode(uint32_t address, table* t);
// load parent
void loadParent(table* t);
// load left
void loadLeft(table* t);
// load right
void loadRight(table* t);
// add page to dirty queue
// add node to dirty queue
void pushQ(table* t);
// allocate new stripe
void newStripe(table* t);
// condense a stripe
void condenseStripe(table* t);
// condense all stripes
void condenseAll(table* t);

#endif