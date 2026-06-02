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

#ifndef NODE_H
#define NODE_H

#include "common.h"
#include "const.h"

typedef struct btree_node {
	void* children[M];
	// keys = [3, 5] means 1, 2, 3 - left child, 4, 5 - middle child, 6+ - right child
	// nodes have room for M keys but only leaf nodes will use all M slots
	int keys[M];
	struct btree_node* parent;
	struct btree_node* next;
	struct btree_node* prev; // remove if two way scanning not necessary

	int childCount;
	uint32_t maxPageNumber;

	bool isLeaf; // if the node is a leaf node
}btree_node;

typedef struct node {
	uint64_t children[M];
	uint32_t keys[M];
	uint64_t parent;
	uint64_t next;
	uint64_t prev;

	uint32_t childCount;
	uint32_t maxPageNumber;

	bool isLeaf;
}node;

#endif