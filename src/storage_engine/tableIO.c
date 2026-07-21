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

/*
TODO:
 - MarkDelete
 - Garbage collection
 - Change array shifting to address
*/

#include "tableIO.h"
#include <stdbool.h>
#include <sys/stat.h>

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
static bool jump(address address, table* t) {
	if (fseek(t->source, address, SEEK_SET) == 0) {
		t->cursor = address;
		return true;
	} else {
		printf("Error: failed to navigate to address: %llu when reading a binary file\n", address);
		return false;
	}
}

/*
jumps to an offset relative to the cursor's current position in the table's source
*/
static bool jumpRel(long offset, table* t) {
	if (fseek(t->source, offset, SEEK_CUR) == 0) {
		t->cursor += offset;
		return true;
	} else {
		printf("Error: failed to make relative jump to address: %llu when reading a binary file\n", t->cursor);
		return false;
	}
}


/*
UNSAFE FUNCTION - assumes there's enough space in the array for the long
big endian
*/
static void writeULongBytewise(char* arr, uint64_t lui) {
	for (int i = 7; i >= 0; i--) {
		*(arr+i) = lui & 0xFF;
		lui >>= 8;
	}
}

static void writeUIntBytewise(char* arr, uint32_t ui) {
	for (int i = 3; i >= 0; i--) {
		*(arr+i) = ui & 0xFF;
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
	jumpRel(-offset - 1, t);
	return a;
}

/*
reads an unsigned integer at the table's cursor + an offset
returns the cursor to the original position
*/
static uint32_t readUInt(long offset, table* t) {
	unsigned char a[4];
	jumpRel(offset, t);
	fread(a, 4, 1, t->source);
	jumpRel(-offset - 4, t);
	uint32_t out = (uint32_t) a[0] << 24 | (uint32_t) a[1] << 16 | (uint32_t) a[2] << 8 | (uint32_t) a[3];
	return out;
}

/*
reads an unsigned long at the table's cursor + an offset
returns the cursor to the original position
*/
static uint64_t readULong(long offset, table* t) {
	unsigned char a[8];
	jumpRel(offset, t);
	fread(a, 8, 1, t->source);
	jumpRel(-offset - 8, t);
	uint64_t out = (uint64_t) a[0] << 56 | (uint64_t) a[1] << 48 | (uint64_t) a[2] << 40 | (uint64_t) a[3] << 32
					| (uint64_t) a[4] << 24 | (uint64_t) a[5] << 16 | (uint64_t) a[6] << 8 | (uint64_t) a[7];
	return out;
}

/*
reads an arbitrary number of bytes from the source file into the buffer
returns the cursor to the original position
*/
static void readArbitrary(char* buffer, uint32_t len, long offset, table* t) {
	jumpRel(offset, t);
	fread(buffer, 1, len, t->source);
	jumpRel(-offset-len, t);
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

/*
copies the contents of the source page into the target page
*/
static void copyPage(slotted_page* source, slotted_page* target) {
	*target = *source;
}

/*
copies the contents of the source node into the target node
*/
static void copyNode(node* source, node* target) {
	*target = *source;
}

static bool validateTableFile(FILE* file, char* fname) {
	// Check file extension
	char* ext = strrchr(fname, '.');
	if (!ext || strcmp(ext, TABLE_EXTENSION) != 0) {
		printf("Error: '%s' does not have the %s extension\n", fname, TABLE_EXTENSION);
		return false;
	}

	// Check magic number (writeMeta writes MAGIC as a native-endian uint32_t)
	uint32_t magic;
	rewind(file);
	if (fread(&magic, 4, 1, file) != 1) {
		printf("Error: could not read magic number from '%s'\n", fname);
		rewind(file);
		return false;
	}
	rewind(file);

	if (magic != MAGIC) {
		printf("Error: '%s' has magic number 0x%X, expected 0x%X\n", fname, magic, MAGIC);
		return false;
	}

	return true;
}

/*
Loads the metadata from a file into a table
Assumes the file has been validated as a correct table file
*/
static bool loadMeta(FILE* file, char* fname, table* table) {
	uint32_t buf[METALEN];
	fread(&buf, 4, METALEN, file);
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
	table->pageFree = ((uint64_t) (uint32_t) buf[9] << 32) | (uint32_t) buf[10];
	table->nodeFree = ((uint64_t) (uint32_t) buf[11] << 32) | (uint32_t) buf[12];
	table->root    = ((uint64_t) (uint32_t) buf[13] << 32) | (uint32_t) buf[14];
	table->M = buf[15];
	return true;
}

bool writeMeta(FILE* file, table* t) {
	uint32_t buf[] = {
		MAGIC,
		t->metalen,
		t->pageStripes,
		t->nodeStripes,
		t->pageNodeRatio,
		t->pageStripeLen,
		t->nodeStripeLen,
		t->pageSize,
		t->nodeSize,
		(uint32_t) (t->pageFree >> 32),
		(uint32_t) (t->pageFree & 0xFFFFFFFF),
		(uint32_t) (t->nodeFree >> 32),
		(uint32_t) (t->nodeFree & 0xFFFFFFFF),
		(uint32_t) (t->root >> 32),
		(uint32_t) (t->root & 0xFFFFFFFF),
		t->M
	};
	jump(0, t);
	fwrite(buf, 4, METALEN, file);
	return true;
}

/*
initializes a table's write stacks
mallocs new memory (stacks)
*/
static void setStacks(table* t) {
	t->pageDirty.size = DIRTY_STACK_INTIAL_SIZE;
	t->pageDirty.count = 0;
	t->pageDirty.stack = malloc(sizeof(page_write_order) * DIRTY_STACK_INTIAL_SIZE);
	t->nodeDirty.size = DIRTY_STACK_INTIAL_SIZE;
	t->nodeDirty.count = 0;
	t->nodeDirty.stack = malloc(sizeof(node_write_order) * DIRTY_STACK_INTIAL_SIZE);
	t->delete.size = DIRTY_STACK_INTIAL_SIZE;
	t->delete.count = 0;
	t->delete.stack = malloc(sizeof(delete_order) * DIRTY_STACK_INTIAL_SIZE);
}

static void freeStacks(table* t) {
	free(t->pageDirty.stack);
	free(t->nodeDirty.stack);
	free(t->delete.stack);
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// PUBLIC API FUNCTIONS

/*
frees a table struct, including the struct itself and all of its memory allocated members
frees allocated memory
*/
void freeTable(table* t) {
	freeStacks(t);
	free(t->name);
	if (t->node) {
		free(t->node);
	} if (t->page) {
		free(t->page);
	}
	free(t);
}

/*
Creates a new file for a database table and returns the matching table struct
Used to create a new table
mallocs new memory (table)
*/
table* createTable(char* tablename) {
	// Build path: "tables/<tablename>.tbl"
	const char* dir = TABLE_DIRECTORY;
	const char* ext = TABLE_EXTENSION;
	size_t pathlen = strlen(dir) + strlen(tablename) + strlen(ext) + 1;
	char* path = malloc(pathlen);
	snprintf(path, pathlen, "%s%s%s", dir, tablename, ext);

	mkdir(dir, 0755); // no-op if directory already exists

	FILE* f = fopen(path, "wb+");
	free(path);
	if (!f) {
		printf("Error: failed to create table file for '%s'\n", tablename);
		return NULL;
	}

	table* t = malloc(sizeof(table));
	t->source        = f;
	t->cursor        = 0;
	t->metalen       = METALEN * 4;
	t->pageStripes   = 1;
	t->nodeStripes   = 1;
	t->pageStripeLen = 8;
	t->nodeStripeLen = 8;
	t->pageNodeRatio = 1;
	t->pageSize      = PAGE_SIZE;
	t->nodeSize      = 34 + M_GLOBAL * 12; // 34B fixed header + M children (8B) + M keys (4B)
	t->M             = M_GLOBAL;
	t->pageFree      = t->metalen;
	t->nodeFree      = (uint64_t)t->metalen
	                 + (uint64_t)t->pageStripes * t->pageStripeLen * t->pageSize;
	t->root          = 0;
	t->page          = NULL;
	t->node          = NULL;
	t->name          = strdup(tablename);

	setStacks(t);
	writeMeta(f, t);
	return t;
}

/*
initializes a table struct from a table file
used to open an existing table
*/
bool loadTable(char* tablename, table* t) {
	if (!tablename || !t) {
		printf("Error: loadTable() recieved a null input\n");
		return false;
	}
	// fname = tables/[tablename].tbl\0
	// size = 7 + strlen(tablename) + 4 + 1
	char* dir = TABLE_DIRECTORY;
	char* ext = TABLE_EXTENSION;
	size_t lenFName = 7 + strlen(tablename) + 5;
	char* fname = malloc(lenFName);
	snprintf(fname, lenFName, "%s%s%s", dir, tablename, ext);

	FILE* tfile = fopen(fname, "rb+");
	if (!tfile) {
		printf("Error: failed to open table %s\n", tablename);
		free(fname);
		return false;
	}
	if (!validateTableFile(tfile, fname)) {
		printf("Error: tried to load a table from %s but file was invalid\n", fname);
		free(fname);
		return false;
	}
	t->source = tfile;
	t->cursor = 0;
	t->name   = strdup(tablename);
	t->node   = NULL;
	t->page   = NULL;
	loadMeta(tfile, fname, t);
	setStacks(t);
	free(fname);
	return true;
}

/*
Deletes and frees a table and its table file
Frees allocated memory (table)
*/
bool deleteTable(table* t) {
	if (!t) {
		printf("Error: deleteTable was called on a NULL table struct\n");
		return false;
	}
	char* dir = TABLE_DIRECTORY;
	char* ext = TABLE_EXTENSION;
	size_t lenFName = 7 + strlen(t->name) + 5;
	char* fname = malloc(lenFName);
	snprintf(fname, lenFName, "%s%s%s", dir, t->name, ext);
	int r = remove(fname);
	if (r != 0) {
		printf("Error: failed to delete file for table %s\n", t->name);
		return false;
	}
	r = fclose(t->source);
	if (r != 0) {
		printf("Error: failed to close file connection for table %s\n", t->name);
		return false;
	}
	freeTable(t);
	return true;
}

/*
As of now, the stacks will be searched linearly. This is obviously inefficient
After first draft of back end is done the stacks should be changed to hashmaps
*/

/*
searches the table's page dirty stack for a page at the given address
returns the corresponding write order if found else returns NULL
*/
page_write_order* searchPageStack(address address, table* t) {
	for (int i = 1; i <= t->pageDirty.count; i++) {
		if ((t->pageDirty.stack + t->pageDirty.count - i)->address == address) return t->pageDirty.stack + t->pageDirty.count - i;
	}
	return NULL;
}

node_write_order* searchNodeStack(address address, table* t) {
	for (int i = 1; i <= t->nodeDirty.count; i++) {
		if ((t->nodeDirty.stack + t->nodeDirty.count - i)->address == address) return t->nodeDirty.stack + t->nodeDirty.count - i;
	}
	return NULL;
}

delete_order* searchDeleteStack(address address, table* t) {
	for (int i = 1; i <= t->delete.count; i++) {
		if ((t->delete.stack + t->delete.count - i)->address == address) return t->delete.stack + t->delete.count - i;
	}
	return NULL;
}

/*
reads a page from an address into a chunk of memory
@param: p - a slotted page to load the data from disk into
*/
bool readPage(address addr, slotted_page* p, table* t) {
	// checking write stack
	page_write_order* search = searchPageStack(addr, t);
	if (search) {
		copyPage(search->page, p);
		return true;
	}

	// otherwise read from disk
	address prev = t->cursor;
	jump(addr, t);
	if (readByte(0, t) != 0) {
		printf("Error: attempted to read page at address %llu but page was invalid\n", addr);
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
	jump(addr + t->pageSize, t); // navigating to 1 byte after the end of the page
	if(!p->entries) {
		p->entries = calloc(1, calcEntriesSize(t));
	}
	for (int i = 0; i < p->header.numEntries; i++) {
		// entry: <--  data | size (4B) | type (2B)  <--
		char code = readByte(-1, t);
		jumpRel(-2, t);
		for (int j = 0; j < NUM_DATATYPES; j++) {
			if (DATATYPE_CODES[j] == code) p->entries[i].type = j;
		}

		uint32_t size = readUInt(-4, t);
		jumpRel(-4, t);
		p->entries[i].size = size;
		if (p->entries[i].data) free(p->entries[i].data);
		p->entries[i].data = malloc(size);
		readArbitrary(p->entries[i].data, size, -size, t);
		jumpRel(-size, t);
	}
	jump(prev, t);
	return true;
}

/*
moves a table's cursor to a page and loads it
callocs one page if t->page == NULL
*/
bool loadPage(address address, table* t) {
	if (!t->page) t->page = calloc(1, sizeof(slotted_page));
	jump(address, t);
	return readPage(address, t->page, t);
}

/*
reads a page from an address into a chunk of memory
@param: p - a slotted page to load the data from disk into
*/
bool readNode(address addr, node* n, table* t) {
	// checking write stack
	node_write_order* search = searchNodeStack(addr, t);
	if (search) {
		copyNode(search->node, n);
		return true;
	}

	// otherwise search disk
	address prev = t->cursor;
	jump(addr, t);

	if (readByte(0, t) != 1) {
		printf("Error: attempted to read page at address %llu but page was invalid\n", addr);
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
	int keylim = n->childCount;
	if (!n->isLeaf) {
		keylim--;
	}
	for (int i = 0; i < keylim; i++) {
		n->keys[i] = readUInt(offset, t);
		offset += 4;
	}

	// return to original cursor position
	jump(prev, t);
	return true;
}
/*
moves a table's cursor to a node and loads it
assumes the current location of the cursor is a valid node
callocs one node if t->node == NULL
*/
bool loadNode(address address, table* t) {
	if (!t->node) t->node = calloc(1, sizeof(node));
	jump(address, t);
	return readNode(address, t->node, t);
}

// write page
/*
writes the given page to the given address
page layout:
 | 0 | parent | pageNum | usedData | numRecords | numEntries | arrCap |
 | maxEntries | maxSlots | slots | ... | records |

REVERSE ORDER IN WHICH RECORDS ARE ADDED SINCE WE NEED TO READ THE ARRAY BACKWARDS
*/
static void writePage(slotted_page* p, address address, table* t) {
	// setup
	jump(address, t);
	char* buffer = calloc(t->pageSize, 1);
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
		writeUIntBytewise(buffer+offset+4, p->slots[i].len);
		writeUIntBytewise(buffer+offset+8, p->slots[i].size);
		writeUIntBytewise(buffer+offset+12, p->slots[i].ptr);
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

/*
writes a page from the dirty queue to an address
*/
void writeNextPage(table* t) { // all pages will be looked up in the dirty queue by their address
	if (t->pageDirty.count <= 0) return;
	// get next write order off the stack
	page_write_order order = t->pageDirty.stack[t->pageDirty.count-1];
	t->pageDirty.count--;
	writePage(order.page, order.address, t);
}

// write node

/*
writes the given node to the given address
*/
static void writeNode(node* n, address address, table* t) {
	// setup
	jump(address, t);
	char* buffer = calloc(t->nodeSize, 1);
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
	// write keys (leaf: one key per child; internal: one fewer key than children)
	int keylim = n->childCount;
	if (!n->isLeaf) keylim--;
	for (int i = 0; i < keylim; i++) {
		writeUIntBytewise(buffer+offset, n->keys[i]);
		offset += 4;
	}
	// copy buffer to disk and clean up
	fwrite(buffer, 1, t->nodeSize, t->source);
	free(buffer);
}

void writeNextNode(table* t) {
	if (t->nodeDirty.count <= 0) return;
	// get next write order off the stack
	node_write_order order = t->nodeDirty.stack[t->nodeDirty.count-1];
	t->nodeDirty.count--;
	jump(order.address, t);
	writeNode(order.node, order.address, t);
}

/*
writes a new tree consisting of one empty page and one empty node directly to a table file
should be used only on a new file
*/
void writeNewTree(slotted_page* p, address pageAddr, node* n, address nodeAddr, table* t) {
	writePage(p, pageAddr, t);
	writeNode(n, nodeAddr, t);
	writeMeta(t->source, t);
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
void markPage(address address, slotted_page* p, table* t) {
    page_write_order* existing = searchPageStack(address, t);
    if (existing) {
        copyPage(p, existing->page);
        return;
    }
	if (t->pageDirty.count == t->pageDirty.size) {
		uint32_t newSize = t->pageDirty.size * DIRTY_STACK_GROWTH_RATE;
		page_write_order* new = malloc(newSize * sizeof(page_write_order));
		memmove(new, t->pageDirty.stack, t->pageDirty.size * sizeof(page_write_order));
		free(t->pageDirty.stack);
		t->pageDirty.stack = new;
		t->pageDirty.size = newSize;
	}
	page_write_order* order = t->pageDirty.stack + t->pageDirty.count++;
	order->address = address;
	order->page = malloc(sizeof(slotted_page));
	copyPage(p, order->page);
}

// mark node dirty
void markNode(address address, node* n, table* t) {
    node_write_order* existing = searchNodeStack(address, t);
    if (existing) {
        copyNode(n, existing->node);
        return;
    }
	if (t->nodeDirty.count == t->nodeDirty.size) {
		uint32_t newSize = t->nodeDirty.size * DIRTY_STACK_GROWTH_RATE;
		node_write_order* new = malloc(newSize * sizeof(node_write_order));
		memmove(new, t->nodeDirty.stack, t->nodeDirty.size * sizeof(node_write_order));
		free(t->nodeDirty.stack);
		t->nodeDirty.stack = new;
		t->nodeDirty.size = newSize;
	}
	node_write_order* order = t->nodeDirty.stack + t->nodeDirty.count++;
	order->address = address;
	order->node = malloc(sizeof(node));
	copyNode(n, order->node);
}

// NEED TO CREATE NEW STACKS FOR DELETING PAGES AND NODES
// OBJECTS SHOULD INITIALLY BE MARKED FOR DELETION
void markDelete(address address, table* t) {
	if (searchDeleteStack(address, t)) return; // skip if already in stack
	if (t->delete.count == t->delete.size) {
		uint32_t newSize = t->delete.size * DIRTY_STACK_GROWTH_RATE;
		delete_order* new = malloc(newSize * sizeof(delete_order));
		memmove(new, t->delete.stack, t->delete.size * sizeof(delete_order));
		free(t->delete.stack);
		t->delete.stack = new;
		t->delete.size = newSize;
	}
	delete_order* order = t->delete.stack + t->delete.count++;
	order->address = address;
}

// delete object
void deleteObject(address address, table* t) {
	char code = 2; // a two in the first byte of an obejct means it's garbage
	jump(address, t);
	fwrite(&code, 1, 1, t->source);
}

/*
Empties a table's write stacks and makes the changes to the file on disk
*/
void commit(table* t) {
	uint32_t pageCount = t->pageDirty.count;
	for (int i = 0; i < pageCount; i++) {
		writeNextPage(t);
	}
	uint32_t nodeCount = t->nodeDirty.count;
	for (int i = 0; i < nodeCount; i++) {
		writeNextNode(t);
	}
	while (t->delete.count > 0) {
		address addr = t->delete.stack[--t->delete.count].address;
		deleteObject(addr, t);
	}
	writeMeta(t->source, t);
}

/*
header
node | node | ... | node
page | page | ... | page
page | page | ... | page
...
page | page | ... | page
node | node | ... | node
page | page | ... | page
...
*/

// allocate new stripe
/*
allocates a new stripe and sets table.pageFree to that address
*/
static void newPageStripe(table* t) {
	t->pageFree = t->metalen + t->pageStripes * t->pageStripeLen * t->pageSize + t->nodeStripes * t->nodeStripeLen * t->nodeSize;
	t->pageStripes++;
}

static void newNodeStripe(table* t) {
	t->nodeFree = t->metalen + t->pageStripes * t->pageStripeLen * t->pageSize + t->nodeStripes * t->nodeStripeLen * t->nodeSize;
	t->nodeStripes++;
}

address allocPage(table* t) {
	address out = t->pageFree;
	if (t->pageFree == t->metalen + t->pageStripes * t->pageStripeLen * t->pageSize + t->nodeStripes * t->nodeStripeLen * t->nodeSize) {
		newPageStripe(t);
	}
	t->pageFree += t->pageSize;
	return out;
}

address allocNode(table* t) {
	address out = t->nodeFree;
	if (t->nodeFree == t->metalen + t->pageStripes * t->pageStripeLen * t->pageSize + t->nodeStripes * t->nodeStripeLen * t->nodeSize) {
		newNodeStripe(t);
	}
	t->nodeFree += t->nodeSize;
	return out;
}

// GARBAGE COLLECTION

/*
moves a node from the source disk address to the dest disk address
writes directly to disk without using the write queue
trusts that both addresses given are correct
*/
static void moveNode(address source, address dest, table* t) {
	slotted_page* p = t->page;
	node* n = t->node;
	address addr = t->cursor;
	loadNode(source, t);
	writeNode(t->node, dest, t);
	jump(addr, t);
	t->page = p;
	t->node = n;
}

/*
moves a page from the source disk address to the dest disk address
writes directly to disk without using the write queue
trusts that both addresses given are correct
*/
static void movePage(address source, address dest, table* t) {
	slotted_page* p = t->page;
	node* n = t->node;
	address addr = t->cursor;
	loadPage(source, t);
	writePage(t->page, dest, t);
	jump(addr, t);
	t->page = p;
	t->node = n;
}

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