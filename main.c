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

void testInsertion() {
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
    node* q = newTree(1);
    for (int i = 3; i < 20; i+=2) {
        findAndInsert(i, q);
        if (q->parent) q = q->parent;
    }
    printTree(q);
    checkTreePointers(q);
    for (int i = 19; i > 0; i-=2) {
        bool d = findAndDelete(i, q);
        printf("Page %d deleted ", i);
        printf(i ? "successfully\n" : "unsuccessfully\n");
        printTree(q);
    }
    printTree(q);
    checkTreePointers(q);
    freeTree(q);
}

int main(int argc, char** argv) {
    testInsertion();
    testDeletion();
}