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
	uint32_t out = (uint8_t)arr[pos];
	out = (out << 8) | (uint8_t)arr[pos+1];
	out = (out << 8) | (uint8_t)arr[pos+2];
	out = (out << 8) | (uint8_t)arr[pos+3];
	return out;
}

// UNTESTED
bool pageFull(slotted_page* p) {
	// TODO: implement once page capacity is tracked in the header
	(void)p;
	return false;
}

// UNTESTED
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

// UNTESTED
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

slotted_page* makePage(uint32_t pageNum, uint32_t numSlots, uint32_t numEntries) {
	slotted_page* out = calloc(1, sizeof(slotted_page));
	out->header.pageNum = pageNum;
	out->slots = calloc(numSlots, sizeof(sp_slot));
	out->entries = callo(numEntries, sizeof(entry));
	return out;
}

void freePage(slotted_page* p) {
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

// UNTESTED
/*
Currently doesn't account for full pages
*/
bool addRecord(slotted_page* p, uint32_t offset, sp_record r) {
	uint32_t index = searchSlotArray(p, offset);
	shiftSlotArrayR(p->slots, index, p->header.numRecords);
	p->slots[index].ID = offset;
	p->slots[index].len = r.len;
	p->slots[index].ptr = p->header.numEntries;
	p->header.numRecords++;
	for (int i = 0; i < r.len; i++) {
		p->entries[p->header.numEntries++] = r.entries[i];
	}
	return true;
}

//UNTESTED
/*
@return true if deletion was successful else return false
*/
bool deleteRecord(slotted_page* p, uint32_t offset) {
	uint32_t index = searchSlotArray(p, offset);
	if (index >= p->header.numRecords || p->slots[index].ID != offset) return false;
	sp_slot* s = p->slots + index;
	for (uint32_t i = 0; i < s->len; i++) {
		free((p->entries[s->ptr+i]).data);
		p->entries[s->ptr+i].data = NULL;
	}
	for (uint32_t i = s->ptr; i < p->header.numEntries - s->len; i++) {
		p->entries[i] = p->entries[i + s->len];
	}
	p->header.numEntries -= s->len;
	shiftSlotArrayL(p->slots, index, p->header.numRecords);
	p->header.numRecords--;
	return true;
}

// UNTESTED
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

// UNTESTED
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