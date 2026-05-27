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
