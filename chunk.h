#ifndef CHUNK_H
#define CHUNK_H

#include "common.h"
#include "memory.h"

typedef struct Chunk {
    uint8_t* code;
    int capacity;
    int count;
} Chunk;

void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte);
void freeChunk(Chunk* chunk);


#endif // CHUNK_H