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
API for reading and writing with database tables

Layout of a table file:

 FACE3419 | Metadata Length | Num page rows | Num node rows | Page rows per node row |
 Pages per row | Nodes per row | Page size | Node size | Pointer to free space | Root pointer |
 M |

Addressing:

 All nodes and pages are referenced by the distance of their first byte from the start of the file
 Represented by an unsigned 64-bit integer
*/

#include "tableIO.h"
#include <stdbool.h>

/*
jumps to a specific address in the table's source
*/
static bool jump(uint64_t address, table* t) {
	if (fseek(t->source, address, SEEK_SET) == 0) {
		t->cursor = address;
		return true;
	} else {
		printf("Error: failed to navigate to address: %lu when reading a binary file\n", address);
	}
}

// UNTESTED
/*
UNSAFE FUNCTION - assumes there's enough space in the array for the long
big endian
*/
static void writeULongBytewise(char* arr, uint64_t lui) {
	for (int i = 7; i >= 0; i--) {
		*(arr+i) = lui && 0xFF;
		lui >>= 8;
	}
}

static void writeUIntBytewise(char* arr, uint32_t ui) {
	for (int i = 3; i >= 0; i--) {
		*(arr+i) = ui && 0xFF;
		ui >>= 8;
	}
}

bool loadMeta(FILE* file, table* table, char* fname) {
	int buf[METALEN];
	fread(&buf, 4, METALEN, file);
	if(buf[0] != MAGIC) {
		printf("Error: failed to open table %s because the file was of an incorrect type\n", fname);
		return false;
	}
	table->source = file;
	table->cursor = 0;
	table->metalen = buf[1];
	table->pageRows = buf[2];
	table->nodeRows = buf[3];
	table->pageNodeRatio = buf[4];
	table->pageRowLen = buf[5];
	table->nodeRowLen = buf[6];
	table->pageSize = buf[7];
	table->nodeSize = buf[8];
	table->free = (long) buf[9] << 32 + (long) buf[10];
	table->root = (long) buf[11] << 32 + (long) buf[12];
	table->M = buf[13];
	return true;
}

bool writeMeta(FILE* file, table* table) {
	int buf[] = {
		MAGIC,
		table->metalen,
		table->pageRows,
		table->nodeRows,
		table->pageNodeRatio,
		table->pageRowLen,
		table->nodeRowLen,
		table->pageSize,
		table->nodeSize,
		(int) table->free >> 32,
		(int) table->free & 0xFFFFFFFF,
		(int) table->root >> 32,
		(int) table->root & 0xFFFFFFFF,
		table->M

	};
	fwrite(buf, 4, METALEN, file);
	return true;
}

bool loadTable(char* fname, table* table) {
	FILE* tfile = fopen(fname, "rb+");
	if (!tfile) {
		printf("Error: Failed to open table %s\n", fname);
	}
	loadMeta(tfile, table, fname);
	return true;
}

/*
loads a nodes's cursor into a node
assumes the current location of the cursor is a valid node
*/
void loadPage(table* t) {
	;
}

/*
loads a table's cursor into a node
assumes the current location of the cursor is a valid node
*/
void loadNode(table* t) {
	;
}

// write page
/*
writes a page from the dirty queue to an address
page layout:
 | 0 | parent | pageNum | usedData | numRecords | numEntries | arrCap |
 | maxEntries | maxSlots | slots | ... | records |

*/
void writeNextPage(table* t) { // all pages will be looked up in the dirty queue by their address
	if (t->pageDirty.count <= 0) return;
	// get next write order off the stack
	page_write_order order = t->pageDirty.stack[t->pageDirty.count-1];
	t->pageDirty.count--;
	jump(order.address, t);
	char* buffer = calloc(t->pageSize, 1);
	slotted_page* p = order.page;
	// write header
	header h = p->header;
	writeULongBytewise(buffer+1, h.parent);
	writeUIntBytewise(buffer+9, h.pageNum);
	writeUIntBytewise(buffer+13, h.usedData);
	writeUIntBytewise(buffer+17, h.numRecords);
	writeUIntBytewise(buffer+21, h.numEntries);
	writeUIntBytewise(buffer+25, h.arrCap);
	writeUIntBytewise(buffer+29, h.maxEntries);
	writeUIntBytewise(buffer+33, h.maxSlots);
	// write slots
	int offset = 37;
	for (int i = 0; i < h.numRecords; i++) {
		writeUIntBytewise(buffer+offset, p->slots[i].ID);
		writeUIntBytewise(buffer+offset+4, p->slots[i].ptr);
		writeUIntBytewise(buffer+offset+8, p->slots[i].len);
		writeUIntBytewise(buffer+offset+12, p->slots[i].size);
		offset += 16;
	}
	// write records
	int entryOffset = 0;
	for (int i = 0; i < h.numRecords; i++) {
		// entry: type (2B) | size (4B) | data
		entry e = p->entries[i];
		entryOffset += e.size + 6;
		if (entryOffset + offset > t->pageSize) { // records with overwrite slots (without malicious inputs this should never occur)
			printf("Error: representation of page %u stores more data than the specified page capacity.\n", h.pageNum);
			// once error handling is implemented should kill program here
		}
		char* entryStart = buffer + t->pageSize-entryOffset;
		*entryStart = '\\';
		*(entryStart+1) = DATATYPE_CODES[e.type];
		writeUIntBytewise(entryStart+2, e.size);
		memcpy(entryStart+6, e.data, e.size); // potentially dangerous
	}
	// copy buffer to disk and clean up
	fwrite(buffer, 1, t->pageSize, t->source);
	free(buffer);
}
// write node
void writeNextNode(table* t) {
	if (t->nodeDirty.count <= 0) return;
	// get next write order off the stack
	node_write_order order = t->nodeDirty.stack[t->nodeDirty.count-1];
	t->nodeDirty.count--;
	jump(order.address, t);
	char* buffer = calloc(t->nodeSize, 1);
	node* n = order.node;
	// write metadata
	buffer[0] = 1;
	writeULongBytewise(buffer+1, n->parent);
	writeULongBytewise(buffer+9, n->prev);
	writeULongBytewise(buffer+17, n->next);
	writeUIntBytewise(buffer+25, n->childCount);
	writeUIntBytewise(buffer+29, n->maxPageNumber);
	buffer[33] = n->isLeaf;
	// write children
	int offset = 34;
	for (int i = 0; i < n->childCount; i++) {
		writeULongBytewise(buffer+offset, n->children[i]);
		offset += 8;
	}
	// write keys
	for (int i = 0; i < n->childCount-1; i++) {
		writeUIntBytewise(buffer+offset, n->keys[i]);
		offset += 4;
	}
	// copy buffer to disk and clean up
	fwrite(buffer, 1, t->nodeSize, t->source);
	free(buffer);
}

/*
void loadParent(node n) {
	;
}

void loadLeft(node n) {
	;
}

void loadRight(node n) {
	;
}*/

// mark page dirty

// mark node dirty

// allocate new stripe

// condense stripe

// condense all stripes