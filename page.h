/*
This file defines a bunch of constructs used to represent a slotted page in memory.
This representation is not the same used to store pages on disk; the translation algorithms are in tableIO.
*/

#ifndef PAGE_H
#define PAGE_H

#include <common.h>

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
	uint32_t numRecords; // number of records in array
	uint32_t numEntries; // number of entries in the the page, this should be a constant multiple of numRecords since row size is constant across a tbale
}header;

typedef struct entry {
	char* data;
	datatype type;
}entry;

typedef struct slot {
	uint32_t ID; // 
	uint32_t ptr; // location in array of slot
	uint32_t len; // number of entries corresponding to the record
}slot;

/*
arr is the slot array and data
from 0 -> are the slot indexes that store the offset from the end of the corresponding record
from  <- len(arr) are the records
*/
typedef struct page {
	header header;
	entry* entries;
	slot* slots;
}page;

int readIndex(int pos, char* arr, int arrlen);
bool pageFull(page*);
bool addRecord(page* p, uint32_t offset, entry* record, int recordLen);
bool deleteRecord(page* p, uint32_t offset);
bool updateRecord(page* p, entry* record);
entry* readRecord(page* p);

#endif