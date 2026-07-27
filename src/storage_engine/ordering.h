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

#ifndef ORDERING_H
#define ORDERING_H

#include "../common.h"
#include "value.h"

#define OFFSET_BITS 6
#define TEXT_KEY_LENGTH_MINIMUM 16
#define TEXT_KEY_MAX_LEN 24
#define TEXT_PAGE_NUM_LEN (TEXT_KEY_MAX_LEN - OFFSET_BITS)

// On-disk serialization sizes (fixed regardless of numeric vs string key type)
// page_num:    1 byte type + TEXT_PAGE_NUM_LEN bytes data  = 19 bytes
// page_offset: 1 byte type + 8 bytes data (max of u64/str) =  9 bytes
#define PAGE_NUM_DISK_SIZE    (1 + TEXT_PAGE_NUM_LEN)
#define PAGE_OFFSET_DISK_SIZE (1 + 8)

typedef enum {
	ORDERING_ULONG,
	ORDERING_STRING,
	ORDERING_DOUBLE
} ordering_type;

typedef struct {
	ordering_type type;
	union {
		uint64_t u64;
		char string[TEXT_PAGE_NUM_LEN + 1];
	} as;
} page_num;

typedef struct {
	ordering_type type;
	union {
		uint64_t u64;
		char string[OFFSET_BITS + 1];
	} as;
} page_offset;

typedef struct {
	page_num    pageNum;
	page_offset offset;
} ordering_key;

ordering_key pkToOk(value pk);
int comparePageNums(page_num a, page_num b);
int compareOffsets(page_offset a, page_offset b);

#endif // ORDERING_H
