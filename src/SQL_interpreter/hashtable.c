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

#include "hashtable.h"
#include "memory.h"
#include "value.h"

void initHashTable(hashtable* table) {
	table->count = 0;
	table->capacity = 0;
	table->entries = NULL;
}

void freeHashTable(hashtable* table) {
	FREE_ARRAY(schema, table->entries, table->capacity);
	initHashTable(table);
}

/*
FNV-1a hash function
*/
static uint32_t FNV1_A(const char* key, int length) {
	uint32_t hash = 2166136261u;
	for (int i = 0; i < length; i++) {
		hash ^= (uint8_t)key[i];
		hash *= 16777619;
	}
	return hash;
}

/*
induction wrapper around hash function
*/
uint32_t hashString(const char* key, int length) {
	return FNV1_A(key, length);
}

/*
finds an entry in the given table
resolves collisions via quadratic probing
*/
static schema* findEntry(uint32_t hash, schema* entries, int capacity) {
	int velocity = 0;
	for (;;) {
		uint32_t index = (hash + velocity * velocity) % capacity;
		schema* found = &entries[index];
		if (found->hash == hash || found->hash == 0) {
			return found;
		}
		velocity++;
	}
}

/*
resize the given hash table by copying the data to a new table by rehashing each entry
frees the original table entries array, callocs a new one
*/
static void adjustCapacity(int capacity, hashtable* table) {
	schema* entries = calloc(sizeof(schema), capacity);
	for (int i = 0; i < table->capacity; i++) {
		schema* e = &table->entries[i];
		if (e->hash == 0) continue;

		schema* dest = findEntry(e->hash, entries, capacity);
		dest->hash = e->hash;
		dest->cols = e->cols;
		dest->count = e->count;
		dest->tablename = e->tablename;
	}
	free(table->entries);
	table->entries = entries;
	table->capacity = capacity;
}

/*
inserts e into the given table if it is not already present
overwrites the exisiting value if e is already in the table
*/
void insertHT(schema* e, hashtable* table) {
	if (table->count + 1 > table->capacity * MAX_LOAD_FACTOR) {
		int capacity = GROW_CAPACITY(table->capacity);
		adjustCapacity(capacity, table);
	}
	schema* found = findEntry(e->hash, table->entries, table->capacity);
	found->hash = e->hash;
	found->cols = e->cols;
	found->count = e->count;
	found->tablename = e->tablename;
	table->count++;
}

/*
read a value from a hash table given a key
return NULL if value is not found
*/
schema* readHT(uint32_t hash, hashtable* table) {
	schema* found = findEntry(hash, table->entries, table->capacity);
	return found->hash ? found : NULL;
}

/*
deletes a key-value pair from a hash table
*/
void deleteHT(uint32_t hash, hashtable* table) {
	schema* target = findEntry(hash, table->entries, table->capacity);
	target->cols = 0;
	target->count = 0;
	target->hash = 0;
}

