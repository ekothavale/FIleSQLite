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
#include "debug.h"
#include "SQL_interpreter/lexer.h"
#include "SQL_interpreter/chunk.h"
#include "SQL_interpreter/vm.h"
#include "storage_engine/bplus.h"
#include "storage_engine/testing.h"


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

/*
TO BE CONTINUED
FULL BACK END SIMULATION
*/
void testTree() {
    table* t = createTree("People", 300);
    address p = findPage(300, t);
}

static void repl() {
    char line[MAX_REPL_INPUT_LEN];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

/*
reads a file into a buffer
mallocs buffer
*/
static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \%s\".\n", path);
        exit(74);
    }

    // get file size to allocate correct buffer
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    // allocate buffer
    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    // read file
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"&s\".\n");
        exit(74);
    }
    buffer[bytesRead] = '\0';

    // clean up
    fclose(file);
    return buffer;
}

static void runFile(const char* path) {
    char* source = readFile(path);
    interpret_result result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, char** argv) {
    //testPages();
    //testPagesRandom();
    //test_tableio();
    //test_table_mgmt();
    //test_btree();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        printf("Usage: ./main [optional SQL file]");
    }
    
}