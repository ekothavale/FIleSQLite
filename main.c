#include "common.h"
#include "parser.h"


void execute(char* input) {
    token* tokens = parse(input);
    opcode* bytecode = compile(tokens);
    run(bytecode);
}

int main(int argc, char* argv) {
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
}