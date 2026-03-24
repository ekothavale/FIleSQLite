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

void testTree(int n, node* t, page* p) {
    // B+ Tree Testing
    // current issue: there should be a way to find where a page should go, that way new entries can be put there
    node* q = generateTestBPlusTree();
    printTree(q, 1);
    int l = 0;
    int pageNum = 1;
    for (int i = 0; i < 10000; i++) {
        printf("I: %d, pageNum: %d\n", i, pageNum);
        if (i == 65) printTree(q, 1);
        l += i;
        pageNum = insertTuple(l, pageNum, q);
    }
    printf("HHHH\n");
    printTree(q, 1);
    freeTree(q);
}

int main(int argc, char** argv) {

}