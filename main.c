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

void testTree() {
    // B+ Tree Testing
    // current issue: there should be a way to find where a page should go, that way new entries can be put there
    node* q = newTree(100);
    //printTree(q, 1);
    int random[] = {3, 16, 7, 20, 90, 101, 88, 2, 76, 32, 10, 66, 71, 45, 22, 91};
    for (int j = 0; j < 16; j++) {
        int i = random[j];
        printf("I = %d\n", i);
        findAndInsert(i, q);
        if (q->parent) q = q->parent;
        printTree(q, 1);
        //printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        //printTree(q, 1);
    }
    printTree(q, 1);
    checkTreePointers(q);
    freeTree(q);
}

int main(int argc, char** argv) {
    testTree();
}