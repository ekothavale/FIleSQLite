#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include "common.h"
#include "chunk.h"

void printTokenizedQuery(TokenizedQuery* tquery);
void printChunk(Chunk* chunk);

#endif // DEBUG_H