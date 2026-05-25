/*
These pages use the most basic and inefficient method of compression which is to compress garbage cells on every deletion.
One way to upgrade this is to track which cells are garbage, but not delete them. Then on writes, garbage cells can potentially be overwritten.
As of now, on each disk write the entire page is rewritten each time. This is highly inefficient and in upgraded versions, only specific records
would need to be overwritten.
*/

#include "page.h"

// UNTESTED
/*
reads a 32bit big-endian unsigned integer from a page's slot array
pos is the MSB
UNSAFE FUNCTION - does not escape if parameters are unsafe
*/
int readIndex(int pos, char* arr, int arrlen) {
	if (pos + 3 > arrlen) {
		printf("Error: read an index from a slot array that extended past the defined slots\n");
	}
	if (pos % 4 != 0) {
		printf("Error: read an index from a slot array using a misaligned position\n");
	}
	uint32_t out = arr[pos];
	out << 4;
	out &= arr[pos+1];
	out << 4;
	out &= arr[pos+2];
	out << 4;
	out &= arr[pos+3];
	return out;
}

// UNTESTED
bool pageFull(page* p) {
	return p->header.arrlen < p->header.numRecords * 4 + p->header.usedData
}

/*
more complicated version:
	// if page is full:
	// 	if the size of the record plus the size of the slot is less than the page's wasted bytes:
	//	  compact the cells and proceed
	//  else:
	// 	  return false
	// find spot in slot array for record
	// shift slots down and put the slot there
	// iterate through deleted slots
	// if there is a deleted slot that is at least twice the size of the record:
	// 	shift all cells down 1
	// 	put record in that cell
	//	decrease size of deleted slot by the size of that record
	//	... | DELETED | ... -> ... | R | DELETED' | ... size(DELETED) = size(R) + size(DELETED)
	//	if the new size of the deleted cell is 4 bytes or less:
	//	  add the new size of the deleted cell to p->wastedBytes
	//	  remove the slot pointing to the cell
	//	  shift any other deleted slots down
	// else:
	// 	add the record to the end of the next open cell
	// return true
*/

// UNTESTED
bool addRecord(page* p, uint32_t offset, entry* record) {
	if (pageFull(p)) return false;
	p->entries[p->header.numRecords] = record;
}

//UNTESTED
/*
@return true if deletion was successful else return false
*/
bool deleteRecord(page* p, uint32_t offset) {
	;
}

// UNTESTED
bool updateRecord(page* p, entry* record) {
	;
}

// UNTESTED
/*
@return record
*/
entry* readRecord(page* p) {
	;
}

// UNTESTED
/*
binary search slot array
@return index of slot
*/
u_int32_t searchSlotArray(page* p, uint32_t offset) {
	u_int32_t hi = p->header.numRecords;
	u_int32_t lo = 0;
	while (hi - lo > 1) {
		uint32_t mid = (hi - lo) / 2;
		if (offset >= mid) lo = mid;
		else hi = mid;
	}
	return lo;
}