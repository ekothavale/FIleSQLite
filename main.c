#include "common.h"
#include "scanner.h"
#include "debug.h"
#include "chunk.h"
#include "bplus.h"

void execute(char* input) {
    TokenizedQuery* tquery = scan(input);
    printTokenizedQuery(tquery);
    freeTokenizedQuery(tquery);

    //opcode* bytecode = compile(tokens);
    //run(bytecode);
}

void testn(int n, node* t, page* p) {
    printPage(p);
    prettyPrintTree(t, 0);
    page* l = findPage(n, t);
    if (p == NULL) printf("Page %d NOT FOUND in tree\n", n);
    else printf("Page %d FOUND in tree\n", p->pageNum);

    printf("\n");

    addPage(l->parent, p);

    printPage(p);
    prettyPrintTree(t, 0);
}

void test(int n) {
    page p;
    p.pageNum = n;
    p.parent = NULL;
    for (int i = 0; i < n; i++) {
        printf("%d\n", (writeVal(&p, i)));
    }
    printPage(&p);
}

int main(int argc, char** argv) {
    // B+ Tree Testing
    node* q = generateTestBPlusTree();
    page* p = malloc(sizeof(page));
    testn(1, q, p);
    freeTree(q);
    

    // VM Testing
    /*
    Chunk chunk;
    initChunk(&chunk);
    for (uint8_t i = 0; i < 160; i++){
        writeChunk(&chunk, i);
    }
    printChunk(&chunk);
    freeChunk(&chunk);
    const int ARRLEN = 2048;
    char* ex = ".quit";
    char input[2048];
    while (true) {
        fgets(input, ARRLEN, stdin);  // read input query
        if (strncmp(input, ex, 5) == 0) break;  // exit if input is exit command
        execute(input);  // execute the query
        memset(&input, 0, ARRLEN);  // reset buffer
    }
    return 0;
    */
}