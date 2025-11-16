#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include "common.h"
#include "chunk.h"
#include "bplus.h"

void printTokenizedQuery(TokenizedQuery* tquery);
void printChunk(Chunk* chunk);
void printPage(page* p);

#endif // DEBUG_H