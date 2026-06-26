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

#include "testing.h"

void test_btree() {
	;
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// SLOTTED PAGE TESTS


/* Allocate a fresh slotted_page with room for up to 64 slots and 256 entries. */
static slotted_page* make_test_page(void) {
    slotted_page* p = malloc(sizeof(slotted_page));
    p->header.parent     = 0;
    p->header.pageNum    = 1;
    p->header.usedData   = 0;
    p->header.numRecords = 0;
    p->header.numEntries = 0;
    p->slots   = calloc(64,  sizeof(sp_slot));
    p->entries = calloc(256, sizeof(entry));
    return p;
}

/*
Free a test page.  deleteRecord already frees data strings for deleted
entries, so we only iterate up to the current numEntries count.
*/
static void free_test_page(slotted_page* p) {
    for (uint32_t i = 0; i < p->header.numEntries; i++) {
        free(p->entries[i].data);
    }
    free(p->slots);
    free(p->entries);
    free(p);
}

/* Build an entry whose data is a heap-allocated copy of str. */
static entry make_entry(const char* str, datatype t) {
    entry e;
    e.type = t;
    e.data = malloc(strlen(str) + 1);
    strcpy(e.data, str);
    return e;
}

/* Wrap an array of entries into an sp_record (does not copy). */
static sp_record make_sp_record(entry* entries, uint32_t len) {
    sp_record r = { entries, len };
    return r;
}

/*
test_page_add: add a single record and verify the slot directory and
entry array are updated correctly.
*/
void test_page_add(void) {
    printf("  test_page_add ... ");
    slotted_page* p = make_test_page();

    entry es[] = { make_entry("Alice", T_STRING), make_entry("30", T_INT) };
    bool ok = addRecord(p, 10, make_sp_record(es, 2));

    assert(ok);
    assert(p->header.numRecords == 1);
    assert(p->header.numEntries == 2);
    assert(p->slots[0].ID  == 10);
    assert(p->slots[0].ptr == 0);
    assert(p->slots[0].len == 2);

    free_test_page(p);
    printf("PASS\n");
}

/*
test_page_read: add a record then read it back; also verify that reading
a non-existent ID returns the sentinel {NULL, 0}.
*/
void test_page_read(void) {
    printf("  test_page_read ... ");
    slotted_page* p = make_test_page();

    entry es[] = { make_entry("Bob", T_STRING), make_entry("25", T_INT) };
    addRecord(p, 5, make_sp_record(es, 2));

    sp_record result = readRecord(p, 5);
    assert(result.len == 2);
    assert(result.entries != NULL);
    assert(strcmp(result.entries[0].data, "Bob") == 0);
    assert(strcmp(result.entries[1].data, "25")  == 0);

    sp_record missing = readRecord(p, 99);
    assert(missing.entries == NULL);
    assert(missing.len == 0);

    free_test_page(p);
    printf("PASS\n");
}

/*
test_page_delete: add two records, delete one, and confirm:
  - numRecords decremented
  - the deleted record is no longer readable
  - the remaining record is still intact
*/
void test_page_delete(void) {
    printf("  test_page_delete ... ");
    slotted_page* p = make_test_page();

    entry es1[] = { make_entry("Carol", T_STRING) };
    entry es2[] = { make_entry("Dave",  T_STRING) };
    addRecord(p, 1, make_sp_record(es1, 1));
    addRecord(p, 2, make_sp_record(es2, 1));
    assert(p->header.numRecords == 2);

    bool ok = deleteRecord(p, 1);
    assert(ok);
    assert(p->header.numRecords == 1);

    sp_record gone = readRecord(p, 1);
    assert(gone.entries == NULL);

    sp_record kept = readRecord(p, 2);
    assert(kept.len == 1);
    assert(strcmp(kept.entries[0].data, "Dave") == 0);

    /* deleteRecord already freed Carol's data; only Dave's remains. */
    free_test_page(p);
    printf("PASS\n");
}

/*
test_page_update: add a record, update one field, then read back and
confirm the new value is stored.
*/
void test_page_update(void) {
    printf("  test_page_update ... ");
    slotted_page* p = make_test_page();

    entry es[] = { make_entry("Eve", T_STRING), make_entry("20", T_INT) };
    addRecord(p, 7, make_sp_record(es, 2));

    /* Replace both entries with fresh heap strings. */
    entry new_es[] = { make_entry("Eve", T_STRING), make_entry("21", T_INT) };
    bool ok = updateRecord(p, 7, make_sp_record(new_es, 2));
    assert(ok);

    sp_record result = readRecord(p, 7);
    assert(strcmp(result.entries[0].data, "Eve") == 0);
    assert(strcmp(result.entries[1].data, "21")  == 0);

    free_test_page(p);
    printf("PASS\n");
}

/*
test_page_multiple_records: insert records out of ID order and verify:
  - the slot directory is kept sorted by ID
  - every record is readable by its original ID
*/
void test_page_multiple_records(void) {
    printf("  test_page_multiple_records ... ");
    slotted_page* p = make_test_page();

    entry e30[] = { make_entry("Charlie", T_STRING) };
    entry e10[] = { make_entry("Alice",   T_STRING) };
    entry e20[] = { make_entry("Bob",     T_STRING) };

    addRecord(p, 30, make_sp_record(e30, 1));
    addRecord(p, 10, make_sp_record(e10, 1));
    addRecord(p, 20, make_sp_record(e20, 1));

    assert(p->header.numRecords == 3);

    /* Slot array must be sorted by ID after each insertion. */
    assert(p->slots[0].ID == 10);
    assert(p->slots[1].ID == 20);
    assert(p->slots[2].ID == 30);

    sp_record r = readRecord(p, 20);
    assert(r.len == 1);
    assert(strcmp(r.entries[0].data, "Bob") == 0);

    /* Delete the middle record and verify neighbours are still accessible. */
    deleteRecord(p, 20);
    assert(p->header.numRecords == 2);
    assert(readRecord(p, 20).entries == NULL);
    assert(strcmp(readRecord(p, 10).entries[0].data, "Alice")   == 0);
    assert(strcmp(readRecord(p, 30).entries[0].data, "Charlie") == 0);

    free_test_page(p);
    printf("PASS\n");
}

/* Run all slotted-page tests. */
void test_page(void) {
    printf("=== Slotted Page Tests ===\n");
    test_page_add();
    test_page_read();
    test_page_delete();
    test_page_update();
    test_page_multiple_records();
    printf("=== All slotted page tests passed ===\n");
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// TABLEIO TESTS
//
// NOTE: test_page_roundtrip, test_node_roundtrip, test_page_write_lifo, and
// test_node_write_lifo verify field values that pass through writePage/writeNode.

// Forward declarations for tableIO-internal functions not declared in tableIO.h
bool loadMeta(FILE* file, table* t, char* fname);
bool writeMeta(FILE* file, table* t);
void setStacks(table* t);

#define TEST_PAGE_SIZE 512
#define TEST_NODE_SIZE 128

/* Create an in-memory table backed by a fresh tmpfile. */
static table make_test_table(void) {
    table t;
    memset(&t, 0, sizeof(t));
    t.source = tmpfile();
    t.cursor = 0;
    t.metalen = METALEN * 4;
    t.pageStripes = 1;
    t.nodeStripes = 1;
    t.pageStripeLen = 8;
    t.nodeStripeLen = 4;
    t.pageNodeRatio = 2;
    t.pageSize = TEST_PAGE_SIZE;
    t.nodeSize = TEST_NODE_SIZE;
    t.pageFree = t.metalen;
    t.nodeFree = (uint64_t)t.metalen + (uint64_t)t.pageStripes * t.pageStripeLen * t.pageSize;
    t.root = 0;
    t.M = M_GLOBAL;
    t.page = NULL;
    t.node = NULL;
    setStacks(&t);
    writeMeta(t.source, &t);
    return t;
}

/* Free dirty-stack heap copies and close the backing file. */
static void free_test_table(table* t) {
    for (uint32_t i = 0; i < t->pageDirty.count; i++)
        if (t->pageDirty.stack[i].page) free(t->pageDirty.stack[i].page);
    free(t->pageDirty.stack);
    for (uint32_t i = 0; i < t->nodeDirty.count; i++)
        if (t->nodeDirty.stack[i].node) free(t->nodeDirty.stack[i].node);
    free(t->nodeDirty.stack);
    fclose(t->source);
}

/* Build a page with the given pageNum and parent, zeroed slot/entry arrays. */
static slotted_page* make_io_page(uint32_t pageNum, uint64_t parent) {
    slotted_page* p = calloc(1, sizeof(slotted_page));
    p->header.pageNum    = pageNum;
    p->header.parent     = parent;
    p->header.usedData   = 128;
    p->header.numRecords = 3;
    p->header.numEntries = 0;
    p->header.arrCap     = 200;
    p->header.maxEntries = 10;
    p->header.maxSlots   = 5;
    p->slots   = calloc(10, sizeof(sp_slot));
    p->entries = calloc(10, sizeof(entry));
    return p;
}

static void free_io_page(slotted_page* p) {
    free(p->slots);
    free(p->entries);
    free(p);
}

// --- writeMeta / loadMeta ---

/*
Write metadata to a fresh table, rewind the file, read it back via
loadMeta, and verify every field survives the round-trip.
*/
void test_meta_roundtrip(void) {
    printf("  test_meta_roundtrip ... ");
    table t = make_test_table();
    t.pageStripes    = 3;
    t.nodeStripes    = 2;
    t.pageStripeLen  = 6;
    t.nodeStripeLen  = 4;
    t.pageNodeRatio  = 3;
    t.pageFree       = 0x00002000;
    t.nodeFree       = 0x00006000;
    t.root           = 0x0000A000;
    t.M              = 4;
    writeMeta(t.source, &t);

    table t2;
    memset(&t2, 0, sizeof(t2));
    rewind(t.source);
    bool ok = loadMeta(t.source, &t2, "test");

    assert(ok);
    assert(t2.metalen       == t.metalen);
    assert(t2.pageStripes   == 3);
    assert(t2.nodeStripes   == 2);
    assert(t2.pageStripeLen == 6);
    assert(t2.nodeStripeLen == 4);
    assert(t2.pageNodeRatio == 3);
    assert(t2.pageSize      == TEST_PAGE_SIZE);
    assert(t2.nodeSize      == TEST_NODE_SIZE);
    assert(t2.pageFree      == 0x00002000);
    assert(t2.nodeFree      == 0x00006000);
    assert(t2.root          == 0x0000A000);
    assert(t2.M             == 4);

    free_test_table(&t);
    printf("PASS\n");
}

/*
Verify that 64-bit addresses with non-zero upper 32 bits survive writeMeta
/ loadMeta intact (tests the high/low word split-and-reconstruct logic).
*/
void test_meta_large_addr(void) {
    printf("  test_meta_large_addr ... ");
    table t = make_test_table();
    t.pageFree = 0x0000000100000000ULL;
    t.nodeFree = 0x00000001ABCDEF12ULL;
    t.root     = 0x00000002FEEDBEEFULL;
    writeMeta(t.source, &t);

    table t2;
    memset(&t2, 0, sizeof(t2));
    rewind(t.source);
    loadMeta(t.source, &t2, "test");

    assert(t2.pageFree == 0x0000000100000000ULL);
    assert(t2.nodeFree == 0x00000001ABCDEF12ULL);
    assert(t2.root     == 0x00000002FEEDBEEFULL);

    free_test_table(&t);
    printf("PASS\n");
}

/*
loadMeta must return false when the file's first four bytes are not MAGIC.
*/
void test_meta_bad_magic(void) {
    printf("  test_meta_bad_magic ... ");
    FILE* f = tmpfile();
    uint32_t buf[METALEN];
    memset(buf, 0, sizeof(buf)); // magic = 0, which != MAGIC
    fwrite(buf, 4, METALEN, f);
    rewind(f);

    table t2;
    bool ok = loadMeta(f, &t2, "bad_file");
    assert(!ok);

    fclose(f);
    printf("PASS\n");
}

// --- markPage ---

/*
Marking the same address twice must not increase the dirty count.
*/
void test_mark_page_dedup(void) {
    printf("  test_mark_page_dedup ... ");
    table t = make_test_table();
    slotted_page* p = make_io_page(1, 0);

    markPage(100, p, &t);
    assert(t.pageDirty.count == 1);

    markPage(100, p, &t);                // same address — should be a no-op
    assert(t.pageDirty.count == 1);

    markPage(200, p, &t);                // different address — should be added
    assert(t.pageDirty.count == 2);

    free_io_page(p);
    free_test_table(&t);
    printf("PASS\n");
}

/*
markPage stores a private heap copy of the page; mutating the original
after marking must not change the copy held in the dirty stack.
*/
void test_mark_page_snapshot(void) {
    printf("  test_mark_page_snapshot ... ");
    table t = make_test_table();
    slotted_page* p = make_io_page(7, 0);

    markPage(300, p, &t);
    p->header.pageNum = 99;              // mutate original after mark

    // The snapshot in the dirty stack should still have pageNum == 7
    assert(t.pageDirty.stack[0].page->header.pageNum == 7);

    free_io_page(p);
    free_test_table(&t);
    printf("PASS\n");
}

/*
Pushing more pages than the initial dirty-stack capacity must cause the
stack to grow without losing any entries.
*/
void test_mark_page_growth(void) {
    printf("  test_mark_page_growth ... ");
    table t = make_test_table();

    // Shrink the stack to 4 to trigger growth after just a few marks
    free(t.pageDirty.stack);
    t.pageDirty.size  = 4;
    t.pageDirty.count = 0;
    t.pageDirty.stack = malloc(4 * sizeof(page_write_order));

    slotted_page* p = make_io_page(1, 0);
    for (int i = 0; i < 6; i++)         // 6 > initial size of 4
        markPage((uint64_t)(1000 + i), p, &t);

    assert(t.pageDirty.count == 6);
    assert(t.pageDirty.size  >  4);     // stack was reallocated

    free_io_page(p);
    free_test_table(&t);
    printf("PASS\n");
}

// --- markNode ---

void test_mark_node_dedup(void) {
    printf("  test_mark_node_dedup ... ");
    table t = make_test_table();
    node n = {0};
    n.childCount = 1;

    markNode(500, &n, &t);
    assert(t.nodeDirty.count == 1);

    markNode(500, &n, &t);
    assert(t.nodeDirty.count == 1);

    markNode(600, &n, &t);
    assert(t.nodeDirty.count == 2);

    free_test_table(&t);
    printf("PASS\n");
}

void test_mark_node_snapshot(void) {
    printf("  test_mark_node_snapshot ... ");
    table t = make_test_table();
    node n = {0};
    n.childCount = 3;

    markNode(700, &n, &t);
    n.childCount = 99;                   // mutate after mark

    assert(t.nodeDirty.stack[0].node->childCount == 3);

    free_test_table(&t);
    printf("PASS\n");
}

void test_mark_node_growth(void) {
    printf("  test_mark_node_growth ... ");
    table t = make_test_table();

    free(t.nodeDirty.stack);
    t.nodeDirty.size  = 3;
    t.nodeDirty.count = 0;
    t.nodeDirty.stack = malloc(3 * sizeof(node_write_order));

    node n = {0};
    for (int i = 0; i < 5; i++)
        markNode((uint64_t)(2000 + i), &n, &t);

    assert(t.nodeDirty.count == 5);
    assert(t.nodeDirty.size  >  3);

    free_test_table(&t);
    printf("PASS\n");
}

// --- allocPage / allocNode ---

/*
Each call to allocPage must return the previous pageFree and advance it
by exactly pageSize.
*/
void test_alloc_page(void) {
    printf("  test_alloc_page ... ");
    table t = make_test_table();
    uint64_t base = t.pageFree;

    uint64_t a1 = allocPage(&t);
    uint64_t a2 = allocPage(&t);
    uint64_t a3 = allocPage(&t);

    assert(a1 == base);
    assert(a2 == base + TEST_PAGE_SIZE);
    assert(a3 == base + 2 * TEST_PAGE_SIZE);

    free_test_table(&t);
    printf("PASS\n");
}

/*
When pageFree reaches the end-of-file boundary, allocPage must trigger a
new page stripe and return the boundary address as the first slot of that
stripe.
*/
void test_alloc_page_stripe(void) {
    printf("  test_alloc_page_stripe ... ");
    table t = make_test_table();

    uint64_t boundary = (uint64_t)t.metalen
        + (uint64_t)t.pageStripes * t.pageStripeLen * t.pageSize
        + (uint64_t)t.nodeStripes * t.nodeStripeLen * t.nodeSize;
    t.pageFree = boundary;
    int old_stripes = t.pageStripes;

    uint64_t addr = allocPage(&t);

    assert(addr           == boundary);
    assert(t.pageStripes  == old_stripes + 1);

    free_test_table(&t);
    printf("PASS\n");
}

/*
After a stripe boundary is crossed, the next allocPage should advance
pageFree by one more pageSize.
*/
void test_alloc_page_after_stripe(void) {
    printf("  test_alloc_page_after_stripe ... ");
    table t = make_test_table();

    uint64_t boundary = (uint64_t)t.metalen
        + (uint64_t)t.pageStripes * t.pageStripeLen * t.pageSize
        + (uint64_t)t.nodeStripes * t.nodeStripeLen * t.nodeSize;
    t.pageFree = boundary;

    allocPage(&t);                       // crosses boundary, pageStripes++
    uint64_t next = allocPage(&t);       // should be boundary + pageSize

    assert(next == boundary + TEST_PAGE_SIZE);

    free_test_table(&t);
    printf("PASS\n");
}

void test_alloc_node(void) {
    printf("  test_alloc_node ... ");
    table t = make_test_table();
    uint64_t base = t.nodeFree;

    uint64_t a1 = allocNode(&t);
    uint64_t a2 = allocNode(&t);
    uint64_t a3 = allocNode(&t);

    assert(a1 == base);
    assert(a2 == base + TEST_NODE_SIZE);
    assert(a3 == base + 2 * TEST_NODE_SIZE);

    free_test_table(&t);
    printf("PASS\n");
}

void test_alloc_node_stripe(void) {
    printf("  test_alloc_node_stripe ... ");
    table t = make_test_table();

    uint64_t boundary = (uint64_t)t.metalen
        + (uint64_t)t.pageStripes * t.pageStripeLen * t.pageSize
        + (uint64_t)t.nodeStripes * t.nodeStripeLen * t.nodeSize;
    t.nodeFree = boundary;
    int old_stripes = t.nodeStripes;

    uint64_t addr = allocNode(&t);

    assert(addr          == boundary);
    assert(t.nodeStripes == old_stripes + 1);

    free_test_table(&t);
    printf("PASS\n");
}

void test_alloc_node_after_stripe(void) {
    printf("  test_alloc_node_after_stripe ... ");
    table t = make_test_table();

    uint64_t boundary = (uint64_t)t.metalen
        + (uint64_t)t.pageStripes * t.pageStripeLen * t.pageSize
        + (uint64_t)t.nodeStripes * t.nodeStripeLen * t.nodeSize;
    t.nodeFree = boundary;

    allocNode(&t);
    uint64_t next = allocNode(&t);

    assert(next == boundary + TEST_NODE_SIZE);

    free_test_table(&t);
    printf("PASS\n");
}

// --- writeNextPage / readPage ---

/*
writeNextPage on an empty queue must be a no-op (no crash, count stays 0).
*/
void test_page_write_empty_queue(void) {
    printf("  test_page_write_empty_queue ... ");
    table t = make_test_table();

    assert(t.pageDirty.count == 0);
    writeNextPage(&t);
    assert(t.pageDirty.count == 0);

    free_test_table(&t);
    printf("PASS\n");
}

/*
Mark a page, write it, read it back, and verify every header field
survives the round-trip.
*/
void test_page_roundtrip(void) {
    printf("  test_page_roundtrip ... ");
    table t = make_test_table();
    slotted_page* p = make_io_page(42, 999);
    p->header.usedData   = 128;
    p->header.numRecords = 3;
    p->header.arrCap     = 200;
    p->header.maxEntries = 10;
    p->header.maxSlots   = 5;

    uint64_t addr = allocPage(&t);
    markPage(addr, p, &t);
    writeNextPage(&t);
    assert(t.pageDirty.count == 0);

    slotted_page r;
    memset(&r, 0, sizeof(r));
    bool ok = readPage(addr, &r, &t);
    assert(ok);
    assert(r.header.pageNum    == 42);
    assert(r.header.parent     == 999);
    assert(r.header.usedData   == 128);
    assert(r.header.numRecords == 3);
    assert(r.header.arrCap     == 200);
    assert(r.header.maxEntries == 10);
    assert(r.header.maxSlots   == 5);

    free(r.slots);
    free(r.entries);
    free_io_page(p);
    free_test_table(&t);
    printf("PASS\n");
}

/*
Mark 3 pages, then call writeNextPage 3 times. Verify the stack drains in
LIFO order — the last-marked page is written first — by reading each
address back and checking its pageNum.
NOTE: field value assertions require the && -> & fix.
*/
void test_page_write_lifo(void) {
    printf("  test_page_write_lifo ... ");
    table t = make_test_table();

    slotted_page* p1 = make_io_page(1, 0);
    slotted_page* p2 = make_io_page(2, 0);
    slotted_page* p3 = make_io_page(3, 0);
    uint64_t a1 = allocPage(&t);
    uint64_t a2 = allocPage(&t);
    uint64_t a3 = allocPage(&t);

    markPage(a1, p1, &t);
    markPage(a2, p2, &t);
    markPage(a3, p3, &t);
    assert(t.pageDirty.count == 3);

    writeNextPage(&t);   // should write p3 to a3
    assert(t.pageDirty.count == 2);
    writeNextPage(&t);   // should write p2 to a2
    assert(t.pageDirty.count == 1);
    writeNextPage(&t);   // should write p1 to a1
    assert(t.pageDirty.count == 0);

    // Each address should now contain its corresponding page
    slotted_page r1 = {0}, r2 = {0}, r3 = {0};
    assert(readPage(a1, &r1, &t) && r1.header.pageNum == 1);
    assert(readPage(a2, &r2, &t) && r2.header.pageNum == 2);
    assert(readPage(a3, &r3, &t) && r3.header.pageNum == 3);

    free(r1.slots); free(r1.entries);
    free(r2.slots); free(r2.entries);
    free(r3.slots); free(r3.entries);
    free_io_page(p1); free_io_page(p2); free_io_page(p3);
    free_test_table(&t);
    printf("PASS\n");
}

// --- writeNextNode / readNode ---

void test_node_write_empty_queue(void) {
    printf("  test_node_write_empty_queue ... ");
    table t = make_test_table();

    assert(t.nodeDirty.count == 0);
    writeNextNode(&t);
    assert(t.nodeDirty.count == 0);

    free_test_table(&t);
    printf("PASS\n");
}

/*
Mark a node, write it, read it back, and verify every field.
*/
void test_node_roundtrip(void) {
    printf("  test_node_roundtrip ... ");
    table t = make_test_table();

    node n = {0};
    n.childCount    = 3;
    n.maxPageNumber = 77;
    n.isLeaf        = true;
    n.parent        = 1000;
    n.prev          = 2000;
    n.next          = 3000;
    n.children[0]   = 100;
    n.children[1]   = 200;
    n.children[2]   = 300;
    n.keys[0]       = 10;
    n.keys[1]       = 20;
    n.keys[2]       = 30;

    uint64_t addr = allocNode(&t);
    markNode(addr, &n, &t);
    writeNextNode(&t);
    assert(t.nodeDirty.count == 0);

    node r = {0};
    bool ok = readNode(addr, &r, &t);
    assert(ok);
    assert(r.childCount    == 3);
    assert(r.maxPageNumber == 77);
    assert(r.isLeaf        == true);
    assert(r.parent        == 1000);
    assert(r.prev          == 2000);
    assert(r.next          == 3000);
    assert(r.children[0]   == 100);
    assert(r.children[1]   == 200);
    assert(r.children[2]   == 300);
    assert(r.keys[0]       == 10);
    assert(r.keys[1]       == 20);

    free_test_table(&t);
    printf("PASS\n");
}

/*
Mark 3 nodes LIFO, write them all, and verify each address holds its node.
NOTE: field value assertions require the && -> & fix.
*/
void test_node_write_lifo(void) {
    printf("  test_node_write_lifo ... ");
    table t = make_test_table();

    node n1 = {0}; n1.childCount = 1; n1.maxPageNumber = 10;
    node n2 = {0}; n2.childCount = 2; n2.maxPageNumber = 20;
    node n3 = {0}; n3.childCount = 3; n3.maxPageNumber = 30;
    uint64_t a1 = allocNode(&t);
    uint64_t a2 = allocNode(&t);
    uint64_t a3 = allocNode(&t);

    markNode(a1, &n1, &t);
    markNode(a2, &n2, &t);
    markNode(a3, &n3, &t);
    assert(t.nodeDirty.count == 3);

    writeNextNode(&t); assert(t.nodeDirty.count == 2);
    writeNextNode(&t); assert(t.nodeDirty.count == 1);
    writeNextNode(&t); assert(t.nodeDirty.count == 0);

    node r1 = {0}, r2 = {0}, r3 = {0};
    assert(readNode(a1, &r1, &t) && r1.maxPageNumber == 10);
    assert(readNode(a2, &r2, &t) && r2.maxPageNumber == 20);
    assert(readNode(a3, &r3, &t) && r3.maxPageNumber == 30);

    free_test_table(&t);
    printf("PASS\n");
}

/* Run all tableIO tests. */
void test_tableio(void) {
    printf("=== TableIO Tests ===\n");
    // writeMeta / loadMeta
    test_meta_roundtrip();
    test_meta_large_addr();
    test_meta_bad_magic();
    // markPage
    test_mark_page_dedup();
    test_mark_page_snapshot();
    test_mark_page_growth();
    // markNode
    test_mark_node_dedup();
    test_mark_node_snapshot();
    test_mark_node_growth();
    // allocPage
    test_alloc_page();
    test_alloc_page_stripe();
    test_alloc_page_after_stripe();
    // allocNode
    test_alloc_node();
    test_alloc_node_stripe();
    test_alloc_node_after_stripe();
    // writeNextPage / readPage
    test_page_write_empty_queue();
    test_page_roundtrip();
    test_page_write_lifo();
    // writeNextNode / readNode
    test_node_write_empty_queue();
    test_node_roundtrip();
    test_node_write_lifo();
    printf("=== All tableIO tests passed ===\n");
}
