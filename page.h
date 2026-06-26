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

/*
This file defines a bunch of constructs used to represent a slotted page in memory.
This representation is not the same used to store pages on disk; the translation algorithms are in tableIO.
*/

#ifndef PAGE_H
#define PAGE_H

#include "common.h"

typedef enum datatype {
	T_INT,
	T_STRING,
	T_DATE,
	T_TIME
}datatype;

/*
each record is a row of data in a table
each element of the record is proceeded by an escape character representing its c type
\i - int
\s - string
\d - date
\t = time
*/ 

static const char DATATYPE_CODES[] = {'i', 's', 'd', 't'};
#define NUM_DATATYPES 4

typedef struct header {
	uint64_t parent;
	uint32_t pageNum;
	uint32_t usedData; // total amount of data used by records
	uint32_t numRecords; // number of records in array at a time
	uint32_t numEntries; // number of entries in the the page, this should be a constant multiple of numRecords since row size is constant across a tbale
	uint32_t arrCap; // total size in bytes of slot array including both slots and records
	uint32_t maxEntries; // max number of entries in the page
	uint32_t maxSlots; // max number of slots in the page
}header;

typedef struct entry {
	char* data;
	uint32_t size; // size of entry data in bytes (NEEDS TO BE ADDED TO TESTING FUNCTIONS)
	datatype type;
}entry;

/* sp_record / sp_slot / slotted_page use the "sp_" prefix to avoid
   colliding with the identically-named types in types.h. */
typedef struct sp_record {
	entry* entries;
	uint32_t len; // number of entries in record (should be fixed for each table)
	uint32_t size; // size in bytes of record
}sp_record;

typedef struct sp_slot {
	uint32_t ID; // record identifier (offset key)
	uint32_t ptr; // index into the entries array where this record begins
	uint32_t len; // number of entries belonging to this record
	uint32_t size; // size in bytes of the corresponding record
}sp_slot;

/*
arr is the slot array and data
from 0 -> are the slot indexes that store the offset from the end of the corresponding record
from  <- len(arr) are the records
*/
typedef struct slotted_page {
	header header;
	entry* entries;
	sp_slot* slots;
}slotted_page;

int readIndex(int pos, char* arr, int arrlen);
bool hasSpace(slotted_page* p, uint32_t size);
slotted_page* makeSPage(uint32_t pageNum, uint32_t numSlots, uint32_t numEntries, uint32_t capacity);
void freeSPage(slotted_page* p);

bool addRecord(slotted_page* p, uint32_t offset, sp_record r);
bool deleteRecord(slotted_page* p, uint32_t offset);
bool updateRecord(slotted_page* p, uint32_t offset, sp_record r);
sp_record readRecord(slotted_page* p, uint32_t offset);

#endif