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

#include "schema.h"

/*
reads the entries in the schema file and loads them into the given hash table
on-disk format per entry: [hash: u32] [col count: u32] [col 0 len: u32] [col 0 bytes] ...
*/
static void readEntries(hashtable* ht, FILE* file) {
	uint32_t entryCount;
	fread(&entryCount, sizeof(uint32_t), 1, file);

	for (uint32_t i = 0; i < entryCount; i++) {
		uint32_t hash;
		fread(&hash, sizeof(uint32_t), 1, file);

		uint32_t colCount;
		fread(&colCount, sizeof(uint32_t), 1, file);

		char** cols = malloc(colCount * sizeof(char*));
		for (uint32_t j = 0; j < colCount; j++) {
			uint32_t len;
			fread(&len, sizeof(uint32_t), 1, file);
			char* name = malloc(len + 1);
			fread(name, 1, len, file);
			name[len] = '\0';
			cols[j] = name;
		}

		ht_entry e = { .hash = hash, .cols = cols, .count = (int)colCount };
		insertHT(&e, ht);
	}
}

/*
writes all of the entries in the schema hash table to the schema file
on-disk format per entry: [hash: u32] [col count: u32] [col 0 len: u32] [col 0 bytes] ...
*/
static void writeEntries(hashtable* ht, FILE* file) {
	uint32_t count = 0;
	for (int i = 0; i < ht->capacity; i++) {
		if (ht->entries[i].hash != 0) count++;
	}
	fwrite(&count, sizeof(uint32_t), 1, file);

	for (int i = 0; i < ht->capacity; i++) {
		ht_entry* e = &ht->entries[i];
		if (e->hash == 0) continue;

		fwrite(&e->hash, sizeof(uint32_t), 1, file);
		uint32_t colCount = (uint32_t)e->count;
		fwrite(&colCount, sizeof(uint32_t), 1, file);

		for (int j = 0; j < e->count; j++) {
			uint32_t len = (uint32_t)strlen(e->cols[j]);
			fwrite(&len, sizeof(uint32_t), 1, file);
			fwrite(e->cols[j], 1, len, file);
		}
	}
}


/*
load schema data from tables/schema.scma
@return: hashtable pointer wiht schema data or NULL on failed read
mallocs (1 hashtable)
*/
hashtable* loadSchema() {
	const char* dir = SCHEMA_DIR;
	const char* ext = SCHEMA_EXT;
	// tables/schema.scma
	size_t lenFName = 7 + 6 + 5;
	char* fname = malloc(lenFName);
	snprintf(fname, lenFName, "%s%s%s", dir, "schema", ext);

	FILE* tfile = fopen(fname, "rb");
	if (!tfile) {
		printf("Error: failed to open database schema\n");
		free(fname);
		return NULL;
	}
	uint32_t magic;
	fread(&magic, 4, 1, tfile);
	if (magic != SCHEMA_MAGIC) {
		printf("Error: schema file is corrupted\n");
		free(fname);
		return NULL;
	}
	hashtable* ht = malloc(sizeof(hashtable));
	initHashTable(ht);
	readEntries(ht, tfile);
	fclose(tfile);
	free(fname);
	return ht;
}

/*
assumes the hashtable contains accurate data
*/
void saveSchema(hashtable* schema) {
	const char* dir = SCHEMA_DIR;
	const char* ext = SCHEMA_EXT;
	size_t lenFName = 7 + 6 + 5;
	char* fname = malloc(lenFName);
	snprintf(fname, lenFName, "%s%s%s", dir, "schema", ext);

	FILE* tfile = fopen(fname, "wb");
	free(fname);
	if (!tfile) {
		printf("Error: failed to open schema file for writing\n");
		return;
	}
	uint32_t magic = SCHEMA_MAGIC;
	fwrite(&magic, sizeof(uint32_t), 1, tfile);
	writeEntries(schema, tfile);
	fclose(tfile);
}
