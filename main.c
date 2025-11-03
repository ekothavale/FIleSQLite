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

int main(int argc, char** argv) {
    // B+ Tree Testing
    

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