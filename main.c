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

#include "common.h"
#include "scanner.h"
#include "debug.h"
#include "chunk.h"
#include "bplus.h"
#include "testing.h"

void execute(char* input) {
    TokenizedQuery* tquery = scan(input);
    printTokenizedQuery(tquery);
    freeTokenizedQuery(tquery);

    //opcode* bytecode = compile(tokens);
    //run(bytecode);
}

/*void testInsertion() {
    // B+ Tree Testing
    // current issue: there should be a way to find where a page should go, that way new entries can be put there
    tree* t = newTree(100);
    //printTree(q, 1);
    int random[] = {3, 16, 7, 20, 90, 101, 88, 2, 76, 32, 10, 66, 71, 45, 22, 91};
    for (int j = 0; j < 16; j++) {
        int i = random[j];
        printf("I = %d\n", i);
        findAndInsert(i, t);
        printTree(t);
        //printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        //printTree(q, 1);
    }
    printTree(t);
    checkTreePointers(t);
    freeTree(t);
}

void testDeletion() {
    tree* q = newTree(1);
    for (int i = 19; i > 1; i-=2) {
        findAndInsert(i, q);
    }
    printTree(q);
    checkTreePointers(q);
    for (int i = 1; i < 20; i+=2) {
        bool d = findAndDelete(i, q);
        printf("Page %d deleted ", i);
        printf(d ? "successfully\n" : "unsuccessfully\n");
        printTree(q);
    }
    printTree(q);
    checkTreePointers(q);
    freeTree(q);
}*/

static entry makeEntry(const char* str, datatype t) {
    entry e;
    e.type = t;
    e.data = malloc(strlen(str) + 1);
    strcpy(e.data, str);
    return e;
}

static entry* makeRecord() {
    entry* out = malloc(5 * sizeof(entry));
    out[0] = makeEntry("Hello", T_STRING);
    out[1] = makeEntry("My", T_STRING);
    out[2] = makeEntry("Name", T_STRING);
    out[3] = makeEntry("IS", T_STRING);
    out[4] = makeEntry("40", T_INT);
    return out;
}

void testPages() {
    int numSlots = 40;
    int numEntries = 200;
    int pageCap = 10000;
    slotted_page* p = makeSPage(13, numSlots, numEntries, pageCap);
    for (int i = 0; i < 45; i++) {
        sp_record record = {
            .entries = makeRecord(),
            .len = 5
        };
        for (int i = 0; i < 5; i++) {
            record.size += strlen(record.entries[i].data) + 1;
        }
        addRecord(p, i, record);
    }
    printSlottedPage(p);
    for (int i = 0; i < 45; i++) {
        deleteRecord(p, i);
    }
    printSlottedPage(p);
}

void testPagesRandom() {
    int numSlots = 40;
    int numEntries = 200;
    int pageCap = 10000;
    slotted_page* p = makeSPage(13, numSlots, numEntries, pageCap);
    int insertionOrder[] = {4, 2, 7, 0, 8, 1, 3, 9, 5, 6};
    for (int i = 0; i < 10; i++) {
        sp_record record = {
            // new records don't have their size written
            .entries = makeRecord(),
            .len = 5
        };
        for (int i = 0; i < 5; i++) {
            record.size += strlen(record.entries[i].data) + 1;
        }
        addRecord(p, insertionOrder[i], record);
    }
    printSlottedPage(p);
    int deletionOrder[] = {9, 6, 1, 2, 7, 3, 4, 8, 0, 5};
    for (int i = 0; i < 10; i++) {
        deleteRecord(p, deletionOrder[i]);
    }
    printSlottedPage(p);
}

int main(int argc, char** argv) {
    //testInsertion();
    //testDeletion();
    testPages();
    testPagesRandom();
}