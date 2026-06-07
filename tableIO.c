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

// ##########################################################################################################################################
// ##########################################################################################################################################
// STATIC HELPER FUNCTIONS


/*
allocate more room than what's available in a page since there are checks
to ensure the pages don't overflow
*/
static size_t calcSlotsSize(table* t) {
	return t->pageSize / sizeof(sp_slot) / 8;
}

static size_t calcEntriesSize(table* t) {
	return t->pageSize * 0.9;
}

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

static bool jumpRel(long offset, table* t) {
	if (fseek(t->source, offset, SEEK_CUR) == 0) {
		t->cursor += offset;
		return true;
	} else {
		printf("Error: failed to make relative jump to address: %lu when reading a binary file\n", t->cursor);
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

/*
reads one byte at the table's cursor + an offset
returns the cursor to the original position
*/
static char readByte(long offset, table* t) {
	char a;
	jumpRel(offset, t);
	fread(&a, 1, 1, t->source);
	jumpTel(-offset, t);
	return a;
}

/*
reads an unsigned integer at the table's cursor + an offset
returns the cursor to the original position
*/
static uint32_t readUInt(long offset, table* t) {
	uint32_t a;
	jumpRel(offset, t);
	fread(&a, 4, 1, t->source);
	jumpRel(-offset, t);
	return a;
}

/*
reads an unsigned long at the table's cursor + an offset
returns the cursor to the original position
*/
static uint64_t readULong(long offset, table* t) {
	uint64_t a;
	jumpRel(offset, t);
	fread(&a, 8, 1, t->source);
	jumpRel(-offset, t);
	return a;
}

/*
reads an arbitrary number of bytes from the source file into the buffer
returns the cursor to the original position
*/
static void readArbitrary(char* buffer, uint32_t len, long offset, table* t) {
	jumpRel(offset, t);
	fread(buffer, 1, len, t->source);
	jumpRel(-offset, t);
}

/*
reads one byte at the table's cursor + an offset
does not return the cursor to the original position
*/
static char consumeByte(long offset, table* t) {
	char a;
	jumpRel(offset, t);
	fread(&a, 1, 1, t->source);
	return a;
}

/*
reads an unsigned integer at the table's cursor + an offset
does not return the cursor to the original position
*/
static uint32_t consumeUInt(long offset, table* t) {
	uint32_t a;
	jumpRel(offset, t);
	fread(&a, 4, 1, t->source);
	return a;
}

/*
reads an unsigned long at the table's cursor + an offset
does not return the cursor to the original position
*/
static uint64_t consumeULong(long offset, table* t) {
	uint64_t a;
	jumpRel(offset, t);
	fread(&a, 8, 1, t->source);
	return a;
}

/*
reads an arbitrary number of bytes from the source file into the buffer
does not return the cursor to the original position
*/
static void consumeArbitrary(char* buffer, uint32_t len, long offset, table* t) {
	jumpRel(offset, t);
	fread(buffer, 1, len, t->source);
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
	table->pageStripes = buf[2];
	table->nodeStripes = buf[3];
	table->pageNodeRatio = buf[4];
	table->pageStripeLen = buf[5];
	table->nodeStripeLen = buf[6];
	table->pageSize = buf[7];
	table->nodeSize = buf[8];
	table->pageFree = (long) buf[9] << 32 + (long) buf[10];
	table->nodeFree = (long) buf[11] << 32 + (long) buf[12];
	table->root = (long) buf[13] << 32 + (long) buf[14];
	table->M = buf[15];
	return true;
}

bool writeMeta(FILE* file, table* t) {
	int buf[] = {
		MAGIC,
		t->metalen,
		t->pageStripes,
		t->nodeStripes,
		t->pageNodeRatio,
		t->pageStripeLen,
		t->nodeStripeLen,
		t->pageSize,
		t->nodeSize,
		(int) t->pageFree >> 32,
		(int) t->pageFree & 0xFFFFFFFF,
		(int) t->nodeFree >> 32,
		(int) t->nodeFree & 0xFFFFFFFF,
		(int) t->root >> 32,
		(int) t->root & 0xFFFFFFFF,
		t->M

	};
	jump(0, t);
	fwrite(buf, 4, METALEN, file);
	return true;
}

void setStacks(table* t) {
	t->pageDirty.size = DIRTY_STACK_INTIAL_SIZE;
	t->pageDirty.count = 0;
	t->pageDirty.stack = malloc(sizeof(slotted_page) * DIRTY_STACK_INTIAL_SIZE);
	t->nodeDirty.size = DIRTY_STACK_INTIAL_SIZE;
	t->nodeDirty.count = 0;
	t->nodeDirty.stack = malloc(sizeof(node) * DIRTY_STACK_INTIAL_SIZE);
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// PUBLIC API FUNCTIONS

bool loadTable(char* fname, table* t) {
	FILE* tfile = fopen(fname, "rb+");
	if (!tfile) {
		printf("Error: Failed to open table %s\n", fname);
		return false;
	}
	t->source = tfile;
	t->cursor = 0;
	loadMeta(tfile, t, fname);
	setStacks(t);
	return true;
}

/*
reads a page from an address into a chunk of memory
@param: p - a slotted page to load the data from disk into
*/
bool readPage(uint64_t address, slotted_page* p, table* t) {
	uint64_t prev = t->cursor;
	jump(address, t);
	if (readByte(0, t) != 0) {
		printf("Error: attempted to read page at address %lu but page was invalid\n", address);
		return false;
	}
	if (p == NULL) {
		printf("Error: tried to read a page into a chunk of memory but the pointer given was null\n");
		return false;
	}
	// header
	p->header.parent = readULong(1, t);
	p->header.pageNum = readUInt(9, t);
	p->header.usedData = readUInt(13, t);
	p->header.numRecords = readUInt(17, t);
	p->header.numEntries = readUInt(21, t);
	p->header.arrCap = readUInt(25, t);
	p->header.maxEntries = readUInt(29, t);
	p->header.maxSlots = readUInt(33, t);
	// slots
	if (!p->slots) {
		p->slots = malloc(calcSlotsSize(t));
	}
	int offset = 37;
	for (int i = 0; i < p->header.numRecords; i++) {
		p->slots[i].ID = readUInt(offset, t);
		p->slots[i].len = readUInt(offset+4, t);
		p->slots[i].size = readUInt(offset+8, t);
		p->slots[i].ptr = readUInt(offset+12, t);
		offset += 16;
	}
	// records
	int entryOffset = 0;
	jumpRel(t->pageSize, t); // navigating to 1 byte after the end of the page
	if(!p->entries) {
		p->entries = malloc(calcEntriesSize(t));
	}
	for (int i = 0; i < p->header.numEntries; i++) {
		// entry: <--  data | size (4B) | type (2B)  <--
		char code = consumeByte(-1, t);
		for (int j = 0; j < NUM_DATATYPES; j++) {
			if (DATATYPE_CODES[j] == code) p->entries[j].type = j;
		}

		uint32_t size = consumeUInt(-5, t);
		p->entries[i].size = size;
		if (p->entries[i].data) free(p->entries[i].data);
		p->entries[i].data = malloc(size);
		consumeArbitrary(p->entries[i].data, size, -size, t);

	}
	jump(prev, t);
	return true;
}

/*
moves a table's cursor to a page and loads it
*/
bool loadPage(uint64_t address, table* t) {
	jump(address, t);
	return readPage(address, t->page, t);
}

/*
reads a page from an address into a chunk of memory
@param: p - a slotted page to load the data from disk into
*/
bool readNode(uint64_t address, node* n, table* t) {
	uint64_t prev = t->cursor;
	jump(address, t);

	if (readByte(0, t) != 1) {
		printf("Error: attempted to read page at address %lu but page was invalid\n", address);
		return false;
	}
	if (n == NULL) {
		printf("Error: tried to read a node into a chunk of memory but the pointer given was null\n");
		return false;
	}

	// read metadata
	n->parent = readULong(1, t);
	n->prev = readULong(9, t);
	n->next = readULong(17, t);
	n->childCount = readUInt(25, t);
	n->maxPageNumber = readUInt(29, t);
	n->isLeaf = readByte(33, t);

	// read children
	int offset = 34;
	for (int i = 0; i < n->childCount; i++) {
		n->children[i] = readULong(offset, t);
		offset += 8;
	}
	// read keys
	for (int i = 0; i < n->childCount-1; i++) {
		n->keys[i] = readUInt(offset, t);
		offset += 4;
	}

	// return to original cursor position
	jump(prev, t);
}
/*
moves a table's cursor to a node and loads it
assumes the current location of the cursor is a valid node
*/
void loadNode(uint64_t address, table* t) {
	jump(address, t);
	return readNode(address, t->node, t);
}

// write page
/*
writes a page from the dirty queue to an address
page layout:
 | 0 | parent | pageNum | usedData | numRecords | numEntries | arrCap |
 | maxEntries | maxSlots | slots | ... | records |

REVERSE ORDER IN WHICH RECORDS ARE ADDED SINCE WE NEED TO READ THE ARRAY BACKWARDS
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
		writeUIntBytewise(buffer+offset+8, p->slots[i].len);
		writeUIntBytewise(buffer+offset+12, p->slots[i].size);
		writeUIntBytewise(buffer+offset+4, p->slots[i].ptr);
		offset += 16;
	}
	// write records
	int entryOffset = 0;
	for (int i = 0; i < h.numRecords; i++) {
		// entry: <--  data | size (4B) | type (2B)  <--
		entry e = p->entries[i];
		int addition = e.size + 6;
		if (entryOffset + addition + offset > t->pageSize) { // records with overwrite slots (without malicious inputs this should never occur)
			printf("Error: representation of page %u stores more data than the specified page capacity.\n", h.pageNum);
			// once error handling is implemented should kill program here
		}
		char* entryStart = buffer + t->pageSize - entryOffset - 1;
		*entryStart = DATATYPE_CODES[e.type];
		*(entryStart-1) = '\\';
		writeUIntBytewise(entryStart-5, e.size);
		memcpy(entryStart - 5 - e.size, e.data, e.size); // potentially dangerous
		entryOffset += addition;
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

/*MIGHT NEED THESE TO RETURN TRUE OR FALSE*/
void loadParent(node* n, node* parent, table* t) {
	readNode(n->parent, parent, t);
}

void loadPrev(node* n, node* prev, table* t) {
	readNode(n->prev, prev, t);
}

void loadNext(node* n, node* next, table* t) {
	readNode(n->next, next, t);
}

// mark page dirty
void markPage(uint64_t address, slotted_page* p, table* t) {
	if (t->pageDirty.count == t->pageDirty.size) {
		uint32_t newSize = t->pageDirty.size * DIRTY_STACK_GROWTH_RATE;
		page_write_order* new = malloc(newSize * sizeof(page_write_order));
		memmove(new, t->pageDirty.stack, t->pageDirty.size);
		free(t->pageDirty.stack);
		t->pageDirty.stack = new;
		t->pageDirty.size = newSize;
	}
	t->pageDirty.stack[t->pageDirty.count].address = address;
	t->pageDirty.stack[t->pageDirty.count++].page = p;
}

// mark node dirty
void markNode(uint64_t address, node* n, table* t) {
	if (t->nodeDirty.count == t->nodeDirty.size) {
		uint32_t newSize = t->nodeDirty.size * DIRTY_STACK_GROWTH_RATE;
		node_write_order* new = malloc(newSize * sizeof(node_write_order));
		memmove(new, t->nodeDirty.stack, t->nodeDirty.size);
		free(t->nodeDirty.stack);
		t->nodeDirty.stack = new;
		t->nodeDirty.size = newSize;
	}
	t->nodeDirty.stack[t->nodeDirty.count].address = address;
	t->nodeDirty.stack[t->nodeDirty.count++].node = n;
}

// delete object
void deleteObject(uint64_t address, table* t) {
	char code = 2; // a two in the first byte of an obejct means it's garbage
	jump(address, t);
	fwrite(&code, 1, 1, t->source);
}

// allocate new stripe
/*
allocates a new stripe and sets table.pageFree to that address
*/
void newPageStripe(table* t) {
	t->pageFree = t->metalen + t->pageStripes * t->pageStripeLen * t->pageSize + t->nodeStripes * t->nodeStripeLen * t->nodeSize;
	t->pageStripes++;
}

void newNodeStripe(table* t) {
	t->nodeFree = t->metalen + t->pageStripes * t->pageStripeLen * t->pageSize + t->nodeStripes * t->nodeStripeLen * t->nodeSize;
	t->nodeStripes++;
}

// GARBAGE COLLECTION
/*
// condense stripe

@param address - address of the first byte of the stripe

void condensePageStripe(uint64_t address, table* t) {
	jump(address, t);
	int garbage = 0;
	for (int i = 0; i + garbage < t->pageStripeLen; i++) {
		if (readByte(0, t) == 2) { // first byte of an object being 2 means its garbage
			if (garbage > 0) {
				//
			}
		} else if (readByte(0, t) != 0) {
			printf("Error: tried to condense memory stripe at %d but address was invalid\n", address);
		}
	}
}

// condense all stripes
*/