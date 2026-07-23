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

#include "time.h"

#include "common.h"
#include "debug.h"
#include "const.h"
#include "SQL_interpreter/lexer.h"
#include "SQL_interpreter/chunk.h"
#include "SQL_interpreter/vm.h"
#include "storage_engine/bplus.h"
#include "storage_engine/testing.h"
#include "SQL_interpreter/testing.h"


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

/*
pretty prints the rows in the results of a query 
*/
static void printResult(result_buffer result) {
    for (int r = 0; r < result.count; r++) {
        for (int c = 0; c < result.cols; c++) {
            if (c > 0) printf(" | ");
            value v = result.rows[r][c];
            SQL_type type = (SQL_type) result.types[c];
            switch (type) {
                case SQL_NULL:  printf("NULL");              break;
                case SQL_BOOL:  printf("%s", v.as.boolean ? "true" : "false"); break;
                case SQL_INT:   printf("%lld", v.as.integer); break;
                case SQL_FLOAT: printf("%g",   v.as.floating); break;
                case SQL_TEXT:  printf("%s",   v.as.text);    break;
                default:        printf("N/A");               break;
            }
        }
        printf("\n");
    }
    printf("(%d row%s)\n", result.count, result.count == 1 ? "" : "s");
}

static void repl() {
    char line[MAX_REPL_INPUT_LEN];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        result_buffer result = interpret(line);
        if (result.print) printResult(result);
    }
}

/*
reads a file into a buffer
mallocs buffer
*/
static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
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
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytesRead] = '\0';

    // clean up
    fclose(file);
    return buffer;
}

static void runFile(const char* path) {
    char* source = readFile(path);
    result_buffer result = interpret(source);
    free(source);

    if (result.ir == INTERPRET_LOAD_ERROR) exit(60);
    if (result.ir == INTERPRET_COMPILE_ERROR) exit(65);
    if (result.ir == INTERPRET_RUNTIME_ERROR) exit(70);
    if (result.print) printResult(result);
}

int main(int argc, char** argv) {
    /*
    test_tableio();
    test_table_mgmt();
    test_btree();
    test_chunk();
    test_value();
    test_lexer();
    test_parser();
    test_hashtable();
    test_schema();
    test_generator();
    test_vm();
    */

    struct timespec start, end;

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        if (strncmp(argv[1], "-d", 2) == 0) {
            #define DEBUG_TRACE_EXECUTION
            repl();
        } else {
            clock_gettime(CLOCK_MONOTONIC, &start);
            runFile(argv[1]);
            clock_gettime(CLOCK_MONOTONIC, &end);
        }
    } else if (argc == 3) {
        if (strncmp(argv[2], "-d", 2) == 0) {
            #define DEBUG_TRACE_EXECUTION
        } else {
            printf("Usage: ./main [SQL file] [id]\n");
            return 0;
        }
        clock_gettime(CLOCK_MONOTONIC, &start);
        runFile(argv[1]);
        clock_gettime(CLOCK_MONOTONIC, &end);

    } else {
        printf("Usage: ./main [SQL file] [-d]\n");
    }

    double time_taken = ((end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1e9) * 1e3;
    printf(" %.3f ms\n", time_taken);
    return 0;
}