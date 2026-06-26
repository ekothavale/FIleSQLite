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
These pages use the most basic and inefficient method of compression which is to compress garbage cells on every deletion.
One way to upgrade this is to track which cells are garbage, but not delete them. Then on writes, garbage cells can potentially be overwritten.
As of now, on each disk write the entire page is rewritten each time. This is highly inefficient and in upgraded versions, only specific records
would need to be overwritten.
*/

#include "page.h"

// ##########################################################################################################################################
// ##########################################################################################################################################
// HELPER FUNCTIONS


bool hasSpace(slotted_page* p, uint32_t size) {
	if (p->header.usedData + size > p->header.arrCap) return false;
	return true;
}

/*
binary search slot array
@return index of slot
*/
static uint32_t searchSlotArray(slotted_page* p, uint32_t offset) {
	uint32_t hi = p->header.numRecords;
	uint32_t lo = 0;
	while (lo < hi) {
		uint32_t mid = lo + (hi - lo) / 2;
		if (p->slots[mid].ID < offset) lo = mid + 1;
		else hi = mid;
	}
	return lo;
}

/*
Does not zero out start
*/
static int shiftSlotArrayR(sp_slot* array, int start, int len) {
	if (start > len) {
		printf("Start index %d beyond length %d of array in shiftNodeArrayR\n", start, len);
		return -1;
	}
	for (int i = len-1; i > start; i--) {
		array[i] = array[i-1];
	}
	return 0;
}

/*
Does not zero out last element of array
*/
static int shiftSlotArrayL(sp_slot* array, int target, int len) {
	if (target > len-1) {
		printf("Start index %d beyond length %d of array in shiftPageArrayL\n", target, len);
		return -1;
	}
	for (int i = target; i < len-1; i++) {
		array[i] = array[i+1];
	}
	return 0;
}

/*
@param - numSlots - maximum number of slots for the page to hold
@param - numEntries - maximum number of entries for the page to hold
@param - capacity - maximum capacity of the slot array (including both entries and slots)
Callocs new memory
*/
slotted_page* makeSPage(uint32_t pageNum, uint32_t numSlots, uint32_t numEntries, uint32_t capacity) {
	slotted_page* out = calloc(1, sizeof(slotted_page));
	out->header.pageNum = pageNum;
	out->header.arrCap = capacity;
	out->header.maxSlots = numSlots;
	out->header.maxEntries = numEntries;
	out->slots = calloc(numSlots, sizeof(sp_slot));
	out->entries = calloc(numEntries, sizeof(entry));
	return out;
}

void freeSPage(slotted_page* p) {
	free(p->entries);
	free(p->slots);
	free(p);
}

// ##########################################################################################################################################
// ##########################################################################################################################################
// CRUD FUNCTIONS

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

/*
Currently doesn't account for full pages
*/
bool addRecord(slotted_page* p, uint32_t offset, sp_record r) {
	printf("Add size: %d\n", r.size);
	if (!hasSpace(p, r.size)) {
		printf("Tried to add record to page %d but it was full\n", p->header.pageNum);
		return false;
	}
	uint32_t index = searchSlotArray(p, offset);
	shiftSlotArrayR(p->slots, index, p->header.numRecords + 1);
	p->slots[index].ID = offset;
	p->slots[index].len = r.len;
	p->slots[index].ptr = p->header.numEntries;
	p->slots[index].size = r.size;
	p->header.numRecords++;
	for (int i = 0; i < r.len; i++) {
		p->entries[p->header.numEntries++] = r.entries[i];
	}
	p->header.usedData += r.size;
	return true;
}

/*
@return true if deletion was successful else return false
*/
bool deleteRecord(slotted_page* p, uint32_t offset) {
	uint32_t index = searchSlotArray(p, offset);
	if (index >= p->header.numRecords || p->slots[index].ID != offset) return false;
	sp_slot deleted = p->slots[index];
	p->header.usedData -= deleted.size;
	printf("Deleted size: %d\n", deleted.size);
	for (uint32_t i = 0; i < deleted.len; i++) {
		free((p->entries[deleted.ptr + i]).data);
		p->entries[deleted.ptr + i].data = NULL;
	}
	for (uint32_t i = deleted.ptr; i < p->header.numEntries - deleted.len; i++) {
		p->entries[i] = p->entries[i + deleted.len];
	}

	// Any slot that pointed after the deleted record must be rebased.
	for (uint32_t i = 0; i < p->header.numRecords; i++) {
		if (i == index) continue;
		if (p->slots[i].ptr > deleted.ptr) {
			p->slots[i].ptr -= deleted.len;
		}
	}

	uint32_t oldNumEntries = p->header.numEntries;
	p->header.numEntries -= deleted.len;

	// Clear trailing duplicate cells left behind by entry compaction.
	for (uint32_t i = p->header.numEntries; i < oldNumEntries; i++) {
		p->entries[i].data = NULL;
		p->entries[i].type = T_INT;
		p->entries[i].size = 0;
	}

	shiftSlotArrayL(p->slots, index, p->header.numRecords);
	p->header.numRecords--;
	return true;
}

bool updateRecord(slotted_page* p, uint32_t offset, sp_record r) {
	uint32_t index = searchSlotArray(p, offset);
	if (index >= p->header.numRecords) return false;
	sp_slot s = p->slots[index];
	if (s.ID != offset || s.len < r.len) return false;
	for (uint32_t i = 0; i < r.len; i++) {
		free(p->entries[s.ptr + i].data);
		p->entries[s.ptr + i] = r.entries[i];
	}
	return true;
}

/*
@return sp_record pointing into the page's entry array
@return {NULL, 0} if record not in page
*/
sp_record readRecord(slotted_page* p, uint32_t offset) {
	uint32_t index = searchSlotArray(p, offset);
	if (index >= p->header.numRecords) {
		sp_record out = {NULL, 0};
		return out;
	}
	sp_slot s = p->slots[index];
	if (s.ID != offset) {
		sp_record out = {NULL, 0};
		return out;
	}
	sp_record out = {p->entries + s.ptr, s.len};
	return out;
}