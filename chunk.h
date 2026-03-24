#ifndef CHUNK_H
#define CHUNK_H

#include "common.h"
#include "memory.h"

typedef struct Chunk {
    uint8_t* code;
    int capacity;
    int count;
} Chunk;

// bytecode
typedef enum opcode {
    OP_SELECT,
    OP_INSERT // OP_INSERT <page_num, int> <record, int>
}opcode;

void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte);
void freeChunk(Chunk* chunk);


#endif // CHUNK_H