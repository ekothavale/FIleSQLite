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

#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "value.h"
#include "../storage_engine/bplus.h"
#include "../storage_engine/tableIO.h"

#define STACK_MAX 256
#define MAX_CURSORS 4 // maximum amount of concurrent cursors

typedef struct cursor {
	table* tbl; // state representing a database table stored in a local file
	bool open; // if the cursor is connected to table
	// add more state (position, etc) here
} cursor;

typedef struct result_buffer {
	Value** rows;
	int count;
	int capacity;
	int cols;
} result_buffer;

// a virtual computer to manage a local database
typedef struct VM {
	Chunk* chunk;
	uint8_t* ip; // instruction pointer
	Value stack[STACK_MAX]; // where values are stored
	Value* stackTop;
	cursor cursors[MAX_CURSORS]; // concurrent database processes
	result_buffer results;
} VM;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} interpret_result;

void initVM();
void freeVM();
interpret_result interpret(const char* source);
void push(Value value);
Value pop();

#endif // VM_H