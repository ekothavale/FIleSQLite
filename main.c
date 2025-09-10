#include "common.h"
#include "parser.h"

void printTokens(Token* tokens) {
    for (int i = 0; i < 10; i++) {
        printf("Token Type: %d\n", tokens[i].type);
    }
}

void execute(char* input) {
    TokenizedQuery* tquery = parse(input);
    printTokens(tquery->tokens);
    freeTokenizedQuery(tquery);

    //opcode* bytecode = compile(tokens);
    //run(bytecode);
}

int main(int argc, char** argv) {
    const int ARRLEN = 2048;
    char* ex = ".quit";
    char input[2048];
    while (true) {
        fgets(input, ARRLEN, stdin);  // read input query
        if (strncmp(input, ex, 5) == 0) break;  // exit if input is exit command
        printf("Input: %s", input);
        execute(input);  // execute the query
        memset(&input, 0, ARRLEN);  // reset buffer
    }
    return 0;
}