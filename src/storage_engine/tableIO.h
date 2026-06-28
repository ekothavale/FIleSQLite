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
#define TABLE_DIRECTORY "tables/" // directory in which table files are placed
#define TABLE_EXTENSION ".tbl" // file extension for table files

typedef struct page_write_order {
	slotted_page* page;
	address address;
}page_write_order;

typedef struct node_write_order {
	node* node;
	address address;
}node_write_order;

typedef struct delete_order {
	address address;
}delete_order;

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
	delete_order* stack;
}delete_stack;

typedef struct table {
	page_stack pageDirty; // stack of dirty pages
	node_stack nodeDirty; // stack of dirty nodes
	delete_stack delete; // stack of objects to be deleted
	FILE* source; // physical file
	char* name; // name of table (corresponding file path is tables/[name].tbl)
	address cursor; // current address
	address pageFree; // address of next free page space
	address nodeFree; // address of next free node space
	address root; // pointer to the root of the tree
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

// manage table struct
void freeTable(table* t);
// manage database tables
table* createTable(char* tablename);
bool loadTable(char* tablename, table* t);
bool deleteTable(table* t);
// loading pages
bool readPage(address address, slotted_page* p, table* t);
bool loadPage(address address, table* t);
// loading nodes
bool readNode(address address, node* n, table* t);
bool loadNode(address address, table* t);
void loadParent(node* n, node* parent, table* t);
void loadPrev(node* n, node* prev, table* t);
void loadNext(node* n, node* next, table* t);
// writing
void writeNextPage(table* t);
void writeNextNode(table* t);
void writeNewTree(slotted_page* p, address pageAddr, node* n, address nodeAddr, table* t);
// marking dirty objects
void markPage(address address, slotted_page* p, table* t);
void markNode(address address, node* n, table* t);
void markDelete(address address, table* t); // can be used for any object type
void commit(table* t);
// allocate new addresses
void newStripe(table* t);
address allocNode(table* t);
address allocPage(table* t);
// file-level garbage collection (not yet implemented)
void condenseStripe(table* t);
void condenseAll(table* t);

// for testing purposes


#endif