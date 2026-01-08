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

void testn(int n) {
    // this doesn't work because it doesn't set up any trees

    // setting up testing tree
    node r, i1, i2, i3, l1, l2, l3, l4, l5, l6, l7, l8;
    node* leaves[8] = {&l1, &l2, &l3, &l4, &l5, &l6, &l7, &l8};

    r.children[0] = &i1;
    r.children[1] = &i2;
    r.children[2] = &i3;
    i1.parent = &r;
    i2.parent = &r;
    i3.parent = &r;
    r.childCount = 3;

    i1.children[0] = &l1;
    i1.children[1] = &l2;
    i1.children[2] = &l3;
    l1.parent = &i1;
    l2.parent = &i1;
    l3.parent = &i1;
    i1.childCount = 3;

    i2.children[0] = &l4;
    i2.children[1] = &l5;
    l4.parent = &i2;
    l5.parent = &i2;
    i2.childCount = 2;

    i3.children[0] = &l6;
    i3.children[1] = &l7;
    i3.children[2] = &l8;
    l6.parent = &i3;
    l7.parent = &i3;
    l8.parent = &i3;
    i3.childCount = 3;

    int a = 0;
    int b = 1;
    int c = 100;
    for (int i = 0; i < 8; i++) {
        leaves[i]->isLeaf = true;
        for (int j = 0; j < c % 3; j++) {
            leaves[i]->children[j] = malloc(sizeof(page));
            int tmp = a + b;
            ((page*) leaves[i]->children[j])->pageNum = tmp;
            a = b;
            b = tmp;
        }
        leaves[i]->childCount = c % 3;
        c += b;
    }

    if (findPage(n, &r) != NULL) printf("Page %d FOUND in tree\n", n);
    else printf("Page %d NOT found in tree\n", n);

    // freeing pages
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < leaves[i]->childCount; j++) {
            free(leaves[i]->children[j]);
        }
    }
    printf("\n");


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
    test(10000000);
    

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