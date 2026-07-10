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

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "../common.h"

#define MAX_LOAD_FACTOR 0.8 // load factor at which the hash table is resized

typedef struct entry {
	char** cols; // names of columns
	uint32_t hash; // hash of key to id the entry
	int count; // number of rows in the entry
} entry;

typedef struct hashtable {
	int count;
	int capacity;
	entry* entries;
} hashtable;

void initHashTable(hashtable* table);
void freeHashTable(hashtable* table);
uint32_t hashString(const char* key, int len);
void insertHT(entry* e, hashtable* table);
entry* readHT(uint32_t, hashtable* table);
void deleteHT(uint32_t, hashtable* table);

#endif