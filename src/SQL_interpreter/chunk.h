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

#ifndef CHUNK_H
#define CHUNK_H

#include "common.h"
#include "memory.h"
#include "value.h"

typedef struct chunk {
    uint8_t* code; // pointer to the start of bytecode dynamic array
    int capacity; // bytecode dynamic array size
    int count; // bytecode dynamic array count (grow when count >= capacity)
    int* lines; // line numbers corresponding to each opcode
    ValueArray constants;
} chunk;

/*
bytecode for VM
each opcode is one byte and can be followed by 1 or 2 byte optional arguments
*/
typedef enum opcode {
    // constants
    OP_CONSTANT,
    OP_NULL,
    OP_TRUE,
    OP_FALSE,
    // stack ops
    OP_POP,
    // arithmetic
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    // comparison & logic
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_LIKE,
    OP_IS_NULL,
    OP_NOT_NULL,
    OP_NOT,
    // control flow
    OP_JUMP,
    OP_JUMP_FALSE,
    OP_JUMP_TRUE,
    // scanner ops
    OP_OPEN_SCAN,
    OP_CLOSE_SCAN,
    OP_NEXT,
    OP_REWIND,
    OP_COLUMN,
    // database management language
    OP_EMIT_ROW,
    OP_INSERT_ROW,
    OP_UPDATE_COL,
    OP_DELETE_ROW,
    // database definition language
    OP_CREATE_TABLE,
    OP_DROP_TABLE,
    // result set ops
    OP_SORT,
    OP_LIMIT,
    // execution management
    OP_HALT
}opcode;

void initChunk(chunk* chunk);
void writeChunk(chunk* chunk, uint8_t byte, int line);
void freeChunk(chunk* chunk);
int addConstant(chunk* chunk, value value);


#endif // CHUNK_H