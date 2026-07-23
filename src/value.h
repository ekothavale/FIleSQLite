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

#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef enum {
	VAL_NULL,
	VAL_BOOL,
	VAL_INT,
	VAL_FLOAT,
	VAL_TEXT,
	VAL_U32,
} value_type;

/*
REPRESENTATION OF SQL DATATYPES
real sql types have size parameters that customize the maximum sizes of each type
this has not yet been implemented

*/
typedef enum {
	SQL_NULL, // null
	SQL_TEXT, // string
	SQL_BOOL, // boolean
	SQL_INT, // integer
	SQL_FLOAT, // float
	SQL_DOUBLE, // double
	SQL_DATE, // YYYY-MM-DD
	SQL_DATETIME, // YYYY-MM-DD hh:mm:ss
	SQL_TIME, // hh:mm:ss
} SQL_type;

// tagged union
typedef struct value {
	value_type type;
	union {
		bool boolean;
		int64_t integer;
		double floating;
		char* text; // heap allocated, VM responsible for lifetime
		uint32_t u32;
	} as;
} value;

#define NULL_VAL(v)   ((value){VAL_NULL, {.integer = 0}})
#define BOOL_VAL(v)   ((value){VAL_BOOL, {.boolean = v}})
#define INTEGER_VAL(v)   ((value){VAL_INT, {.integer = v}})
#define FLOAT_VAL(v)   ((value){VAL_FLOAT, {.floating = v}})
#define TEXT_VAL(v)   ((value){VAL_TEXT, {.text = v}})
#define UINT_VAL(v)   ((value){VAL_U32, {.u32 = v}})

typedef struct ValueArray {
	int capacity;
	int count;
	value* values;
} ValueArray;

// Public API — callable from outside this translation unit
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, value value);
void freeValueArray(ValueArray* array);
void printValue(value value);
SQL_type getSQLType(char byte);
char encodeSQLType(SQL_type t);

#endif