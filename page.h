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
	datatype type;
}entry;

/* sp_record / sp_slot / slotted_page use the "sp_" prefix to avoid
   colliding with the identically-named types in types.h. */
typedef struct sp_record {
	entry* entries;
	uint32_t len;
	uint32_t size;
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